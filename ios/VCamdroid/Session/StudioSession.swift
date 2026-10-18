import AVFoundation
import CoreMedia
import Foundation
import Observation
import SwiftUI
import UIKit

@MainActor
@Observable
final class StudioSession {
    let previewSession: PreviewSession
    private let capture: CaptureEngine
    private let encoder: Encoder
    private let server: DroidCamServer
    private let curtainController = ScreenCurtainController()

    var settings: StudioSettings {
        didSet { noteSettingsSaved(from: oldValue) }
    }
    private(set) var snapshot: SessionSnapshot
    private(set) var logs: [LogEntry] = []
    private(set) var clientAddress: String?
    private(set) var isUSBClient = false
    private(set) var statusMessage: LocalizedStringResource = .waitingForOBS
    private(set) var isRunning = false
    private(set) var isThermalHot = false
    var curtainPresented = false
    private(set) var portraitCare: PortraitCare = .hidden
    private(set) var portraitPreviewActive = false
    private(set) var interfaceLandscapeLeft = false

    var portraitPreviewDeferred: Bool {
        settings.portraitEnabled && !settings.portraitPhonePreview && portraitCare != .hidden && portraitCare != .paused
    }
    let portraitPreview = PortraitPreviewSink()
    private let deskPreview = PreviewJPEGSink()
    private let streamGate = StreamGate()
    private var budget = ThermalBudget()
    private var budgetTask: Task<Void, Never>?
    private var pacedBitrateKbps: Int?
    private var curtainTask: Task<Void, Never>?
    private var quietActivityUntil = Date.distantPast
    private var careTasks: [Task<Void, Never>] = []
    private var lastPortraitCommand = PortraitCommand.off
    private var encoderActive = false
    private var panelStream = false {
        didSet {
            snapshot.panelClient = panelStream
            streamGate.setSkipDesk(panelStream)
            if panelStream {
                deskPreview.clear()
            }
        }
    }
    private var activeBitrateKbps: Int
    private(set) var shotPresets: [ShotPreset] = ShotPresetStore.load()
    private(set) var stoppedByUser = false
    private(set) var customWhiteBalanceSupported = true
    private(set) var customFocusSupported = true
    private var hasAnnouncedLive = false
    private var packetForwarder: OrderedPacketForwarder?
    private var packetTask: Task<Void, Never>?
    private var listenerRestart: Task<Void, Never>?
    private var pausedForBackground = false
    private var listenFailed = false
    private var lastCurtainArm = Date.distantPast

    init() {
        let preview = PreviewSession()
        previewSession = preview
        capture = CaptureEngine(previewSession: preview)
        encoder = Encoder()
        server = DroidCamServer()
        let stored = StudioSettingsStore.load()
        settings = stored
        activeBitrateKbps = stored.bitrateKbps
        snapshot = SessionSnapshot(
            status: "waiting",
            isStreaming: false,
            width: stored.width,
            height: stored.height,
            fps: stored.fps,
            format: .avc,
            lens: stored.lens,
            batteryPercent: 100,
            tally: .idle
        )
        statusMessage = .waitingForOBS
    }

    var addressLine: String {
        "\(LocalAddress.ipv4()):\(settings.port)"
    }

    var panelLine: String {
        let host = isUSBClient ? "127.0.0.1" : LocalAddress.ipv4()
        return "http://\(host):\(settings.port)/panel"
    }

    /// Angle 0 is upright with the interface in landscape right. Flip adds a half turn on top.
    var streamRotation: CGFloat {
        settings.flip180 != interfaceLandscapeLeft ? 180 : 0
    }

    private var captureConfiguration: CaptureConfiguration {
        CaptureConfiguration(settings: settings, rotationAngle: streamRotation)
    }

    func setInterfaceOrientation(_ orientation: UIInterfaceOrientation) {
        guard orientation.isLandscape else { return }
        let left = orientation == .landscapeLeft
        guard left != interfaceLandscapeLeft else { return }
        interfaceLandscapeLeft = left
        guard isRunning else { return }
        Task {
            try? await capture.update(captureConfiguration)
        }
    }

    func start() async {
        guard !isRunning else { return }
        stoppedByUser = false
        listenFailed = false
        snapshot.batteryPercent = BatteryPercent.current()
        curtainController.disableIdleTimer()
        await server.configure(
            snapshot: { await self.currentSnapshot() },
            catalog: { await self.catalog() },
            preview: { await self.deskPreview.jpegData() },
            onVideo: { request in await self.beginVideo(request) },
            onVideoStopped: { await self.endVideo() },
            onUpdate: { update in await self.apply(update: update) },
            controls: { await self.currentControls() },
            onControls: { update in await self.apply(controls: update) },
            presets: { await self.presetListJSON() },
            onPreset: { command in await self.perform(command) },
            onTally: { tally in await self.setTally(tally) },
            onClient: { address, usb in await self.setClient(address, usb: usb) },
            onListenFailed: { await self.noteListenFailed() }
        )
        let forwarder = OrderedPacketForwarder()
        let videoEncoder = encoder
        forwarder.setOverflowHandler {
            videoEncoder.requestKeyframe()
        }
        let videoServer = server
        packetForwarder?.finish()
        packetForwarder = forwarder
        packetTask?.cancel()
        packetTask = Task {
            await forwarder.run { packet in
                await videoServer.send(pts: packet.pts, payload: packet.payload)
            }
        }
        await encoder.setHandler { [forwarder] pts, payload in
            forwarder.enqueue(pts: pts, payload: payload)
        }
        let preview = portraitPreview
        let desk = deskPreview
        let gate = streamGate
        await capture.setFrameHandler { [encoder, preview, desk, gate] pixelBuffer, time in
            if gate.shouldEncode(time) {
                encoder.submit(pixelBuffer, time: time)
            }
            preview.enqueue(pixelBuffer, time: time)
            if !gate.skipsDesk {
                desk.enqueue(pixelBuffer)
            }
        }
        let command = updatePortraitCare()
        lastPortraitCommand = command
        await capture.setPortrait(command)
        do {
            try await server.start(
                port: settings.port,
                advertise: settings.discoveryEnabled,
                deviceName: DeviceName.current
            )
            try await capture.start(captureConfiguration)
            if listenFailed {
                await encoder.stop()
                await capture.stop()
                await server.stop()
                return
            }
            isRunning = true
            statusMessage = .waitingForOBS
            await noteDeviceLimits()
            appendLog(String(localized: .waitingForOBS))
            armCurtain()
            watchCare()
            watchBudget()
        } catch DroidCamServerError.invalidPort, DroidCamServerError.portInUse {
            statusMessage = .portInUse
            appendLog(String(localized: .portInUse))
        } catch {
            statusMessage = .startFailed
            appendLog(String(localized: .startFailed))
        }
    }

    func stop() async {
        stoppedByUser = true
        listenerRestart?.cancel()
        packetForwarder?.finish()
        packetTask?.cancel()
        packetForwarder = nil
        packetTask = nil
        curtainTask?.cancel()
        budgetTask?.cancel()
        budgetTask = nil
        budget = ThermalBudget()
        pacedBitrateKbps = nil
        careTasks.forEach { $0.cancel() }
        careTasks = []
        curtainPresented = false
        portraitPreviewActive = false
        portraitCare = .hidden
        lastPortraitCommand = .off
        curtainController.enableIdleTimer()
        encoderActive = false
        await capture.setPortrait(.off)
        await encoder.stop()
        await capture.stop()
        await server.stop()
        deskPreview.clear()
        isRunning = false
        isThermalHot = false
        snapshot.isStreaming = false
        snapshot.status = "waiting"
        clientAddress = nil
        isUSBClient = false
        panelStream = false
        activeBitrateKbps = settings.bitrateKbps
        statusMessage = .cameraStopped
    }

    func replaySetup() {
        Task { await leaveForSetup() }
    }

    private func leaveForSetup() async {
        if isRunning {
            await stop()
        }
        stoppedByUser = false
        settings.onboardingCompleted = false
    }

    func confirmStopIfNeeded() -> Bool {
        settings.confirmStop
    }

    func updateScenePhase(_ phase: ScenePhase) {
        switch phase {
        case .background:
            guard isRunning else { return }
            pausedForBackground = true
            statusMessage = .streamPausedLocked
            appendLog(String(localized: .streamPausedLocked))
            Task { await pauseForBackground() }
        case .active:
            guard isRunning, pausedForBackground else { return }
            pausedForBackground = false
            Task { await resumeFromBackground() }
        default:
            break
        }
    }

    func noteActivity() {
        guard settings.curtainMode == .automatic else { return }
        guard Date() >= quietActivityUntil else { return }
        if curtainPresented {
            hideCurtain()
            armCurtain()
            return
        }
        guard Date().timeIntervalSince(lastCurtainArm) >= 1 else { return }
        armCurtain()
    }

    func setCurtainMode(_ mode: CurtainMode) {
        settings.curtainMode = mode
        switch mode {
        case .on:
            curtainTask?.cancel()
            showCurtain()
        case .off:
            curtainTask?.cancel()
            hideCurtain()
        case .automatic:
            hideCurtain()
            armCurtain()
        }
    }

    func dismissCurtain() {
        hideCurtain()
        if settings.curtainMode != .off {
            armCurtain()
        }
    }

    func engageCurtain() {
        curtainTask?.cancel()
        quietActivityUntil = Date().addingTimeInterval(0.6)
        curtainTask = Task { [weak self] in
            try? await Task.sleep(for: .milliseconds(80))
            guard !Task.isCancelled else { return }
            self?.showCurtain()
        }
    }

    func applyControls() {
        noteActivity()
        pushPortrait()
        guard isRunning else { return }
        Task {
            try? await capture.update(captureConfiguration)
            await noteDeviceLimits()
        }
    }

    private func noteDeviceLimits() async {
        let whiteBalance = await capture.supportsCustomWhiteBalance()
        let focus = await capture.supportsCustomFocus()
        customWhiteBalanceSupported = whiteBalance
        customFocusSupported = focus
        var next = settings
        var changed = false
        if !whiteBalance, next.manualWhiteBalance {
            next.manualWhiteBalance = false
            changed = true
        }
        if !focus, next.manualFocus {
            next.manualFocus = false
            changed = true
        }
        if changed {
            settings = next
        }
    }

    func applyPortrait() {
        let clean = settings.sanitized()
        if clean != settings {
            settings = clean
        }
        noteActivity()
        pushPortrait()
    }

    func setBitrate(_ kbps: Int) {
        var copy = settings
        copy.bitrateKbps = kbps
        let clamped = copy.sanitized().bitrateKbps
        guard clamped != settings.bitrateKbps else { return }
        settings.bitrateKbps = clamped
        if !panelStream {
            activeBitrateKbps = clamped
        }
        noteActivity()
        guard encoderActive, !panelStream else { return }
        Task { await restartEncoder() }
    }

    func selectLens(_ lens: Lens) {
        settings.lens = lens
        snapshot.lens = lens
        applyControls()
    }

    func setMode(_ mode: VideoMode) {
        guard !panelStream else { return }
        var next = settings
        next.width = mode.width
        next.height = mode.height
        next.fps = mode.fps
        settings = next.sanitized()
        snapshot.width = mode.width
        snapshot.height = mode.height
        snapshot.fps = mode.fps
        applyControls()
        if encoderActive {
            Task { await restartEncoder() }
        }
    }

    func updateFrameRate(_ fps: Int) {
        guard !panelStream else { return }
        settings.fps = fps
        snapshot.fps = fps
        applyControls()
        if encoderActive {
            Task { await restartEncoder() }
        }
    }

    func focus(at point: CGPoint) {
        guard !settings.manualFocus else { return }
        Task { await capture.focus(at: point) }
    }

    func apply(update: SessionUpdate) -> Bool {
        if let link = update.link {
            apply(link: link)
        }
        let hasMode = update.width != nil || update.height != nil || update.fps != nil || update.format != nil || update.lens != nil
        guard hasMode else { return update.link != nil }
        if let format = update.format, format != .avc { return false }
        if panelStream {
            if let lens = update.lens {
                settings.lens = lens
                snapshot.lens = lens
                Task {
                    try? await capture.update(captureConfiguration)
                }
            }
            return true
        }
        var next = settings
        if let width = update.width { next.width = width }
        if let height = update.height { next.height = height }
        if let fps = update.fps { next.fps = fps }
        if let lens = update.lens { next.lens = lens }
        let allowed = VideoMode.allowed(allowsHighFrameRate: next.allowsHighFrameRate)
        guard allowed.contains(next.mode) else { return false }
        settings = next.sanitized()
        snapshot.width = settings.width
        snapshot.height = settings.height
        snapshot.fps = settings.fps
        snapshot.lens = settings.lens
        snapshot.format = .avc
        Task {
            try? await capture.update(captureConfiguration)
            if encoderActive {
                await restartEncoder()
            }
        }
        return true
    }

    func presetListJSON() -> String {
        ShotPresetStore.json(shotPresets)
    }

    func perform(_ command: PresetCommand) -> String {
        switch command {
        case .save(let name):
            shotPresets.append(ShotPreset(name: name, settings: settings))
            ShotPresetStore.save(shotPresets)
        case .apply(let id):
            if let preset = shotPresets.first(where: { $0.id == id }) {
                _ = apply(controls: preset.asControlUpdate())
            }
        case .delete(let id):
            shotPresets.removeAll { $0.id == id }
            ShotPresetStore.save(shotPresets)
        case .rename(let id, let name):
            if let index = shotPresets.firstIndex(where: { $0.id == id }) {
                shotPresets[index].name = name
                ShotPresetStore.save(shotPresets)
            }
        }
        return presetListJSON()
    }

    func apply(controls update: ControlUpdate) -> Bool {
        var next = settings
        if let lens = update.lens {
            next.lens = lens
        }
        if let zoom = update.zoom {
            next.zoom = zoom
        }
        if let manualExposure = update.manualExposure {
            next.manualExposure = manualExposure
        }
        if let exposureBias = update.exposureBias {
            next.exposureBias = exposureBias
        }
        if let iso = update.iso {
            next.iso = iso
        }
        if let shutter = update.shutter {
            next.shutterSeconds = shutter
        }
        if let whiteBalance = update.whiteBalance {
            if let kelvin = whiteBalance.kelvin {
                next.manualWhiteBalance = true
                next.kelvin = kelvin
            } else {
                next.manualWhiteBalance = false
            }
        }
        if let manualFocus = update.manualFocus {
            next.manualFocus = manualFocus
        }
        if let focus = update.focus {
            next.lensPosition = focus
        }
        if let torch = update.torch {
            next.torch = torch
        }
        if let flip = update.flip {
            next.flip180 = flip
        }
        if let portrait = update.portrait {
            next.portraitEnabled = portrait
        }
        if let portraitStrength = update.portraitStrength {
            next.portraitStrength = portraitStrength
        }
        var restartBitrate = false
        if let bitrateKbps = update.bitrateKbps, bitrateKbps != next.bitrateKbps {
            next.bitrateKbps = bitrateKbps
            restartBitrate = !panelStream
        }
        settings = next.sanitized()
        if !panelStream {
            activeBitrateKbps = settings.bitrateKbps
        }
        snapshot.lens = settings.lens
        applyControls()
        if restartBitrate, encoderActive {
            Task { await restartEncoder() }
        }
        return true
    }

    private func apply(link: LinkHealth) {
        switch link {
        case .cannotDecode:
            statusMessage = .pcCannotDecode
        case .live:
            guard statusMessage == .pcCannotDecode else { return }
            statusMessage = snapshot.isStreaming ? .streaming : .waitingForOBS
        }
    }

    private func currentControls() -> ControlState {
        ControlState(settings: settings, activeBitrateKbps: activeBitrateKbps)
    }

    private func currentSnapshot() -> SessionSnapshot {
        var copy = snapshot
        copy.batteryPercent = BatteryPercent.current()
        copy.clockMs = Self.hostClockMs()
        copy.droppedFrames = packetForwarder?.dropped() ?? 0
        copy.thermal = Self.thermalName()
        copy.captureDrops = capture.droppedFrames() + streamGate.drops
        copy.portrait = settings.portraitEnabled
        copy.manualExposure = settings.manualExposure
        copy.shutter = settings.shutterSeconds
        copy.torch = settings.torch
        copy.lowPower = ProcessInfo.processInfo.isLowPowerModeEnabled
        let battery = UIDevice.current.batteryState
        copy.charging = battery == .charging || battery == .full
        return copy
    }

    private static func hostClockMs() -> UInt64 {
        let seconds = CMTimeGetSeconds(CMClockGetTime(CMClockGetHostTimeClock()))
        guard seconds.isFinite, seconds > 0 else { return 0 }
        return UInt64(seconds * 1000)
    }

    private static func thermalName() -> String {
        switch ProcessInfo.processInfo.thermalState {
        case .nominal: return "nominal"
        case .fair: return "fair"
        case .serious: return "serious"
        case .critical: return "critical"
        @unknown default: return "nominal"
        }
    }

    private func catalog() -> ResolutionCatalog {
        let modes = VideoMode.allowed(allowsHighFrameRate: settings.allowsHighFrameRate)
        return ResolutionCatalog(deviceName: DeviceName.current, back: modes, front: modes)
    }

    private func beginVideo(_ request: VideoRequest) async {
        guard request.format == .avc else {
            appendLog(String(localized: .unsupportedMode))
            return
        }
        let quality: StreamQuality
        if request.client == .panel {
            quality = .panel(usb: isUSBClient)
        } else {
            let allowed = VideoMode.allowed(allowsHighFrameRate: settings.allowsHighFrameRate)
            let matches = allowed.contains { $0.width == request.width && $0.height == request.height }
            guard matches else {
                appendLog(String(localized: .unsupportedMode))
                return
            }
            quality = .legacy(
                width: request.width,
                height: request.height,
                fps: settings.fps,
                bitrateKbps: settings.bitrateKbps,
                keyframeInterval: settings.keyframeInterval
            )
        }
        panelStream = request.client == .panel
        budget = ThermalBudget()
        pacedBitrateKbps = quality.bitrateKbps
        streamGate.setFrameSeconds(1.0 / Double(max(quality.fps, 1)))
        adopt(quality)
        pushPortrait()
        do {
            try await capture.update(captureConfiguration)
            try await encoder.start(quality, keyframeInterval: quality.keyframeInterval)
            encoderActive = true
            snapshot.isStreaming = true
            snapshot.status = "streaming"
            if hasAnnouncedLive {
                statusMessage = .streaming
            } else {
                hasAnnouncedLive = true
                statusMessage = .cameraLive
            }
            appendLog(String(localized: .clientConnected))
        } catch {
            encoderActive = false
            panelStream = false
            budget = ThermalBudget()
            pacedBitrateKbps = nil
            activeBitrateKbps = settings.bitrateKbps
            snapshot.isStreaming = false
            snapshot.status = "waiting"
            statusMessage = .waitingForOBS
            appendLog(String(localized: .startFailed))
        }
    }

    private func endVideo() async {
        encoderActive = false
        panelStream = false
        budget = ThermalBudget()
        pacedBitrateKbps = nil
        activeBitrateKbps = settings.bitrateKbps
        await encoder.stop()
        snapshot.isStreaming = false
        snapshot.status = "waiting"
        statusMessage = .waitingForOBS
        appendLog(String(localized: .clientDisconnected))
    }

    private func restartEncoder() async {
        let quality = panelStream
            ? StreamQuality.panel(usb: isUSBClient)
            : StreamQuality.legacy(
                width: settings.width,
                height: settings.height,
                fps: settings.fps,
                bitrateKbps: settings.bitrateKbps,
                keyframeInterval: settings.keyframeInterval
            )
        adopt(quality)
        await encoder.stop()
        do {
            try await encoder.start(quality, keyframeInterval: quality.keyframeInterval)
            encoderActive = true
        } catch {
            encoderActive = false
            appendLog(String(localized: .startFailed))
        }
    }

    private func adopt(_ quality: StreamQuality) {
        settings = settings.adopting(quality)
        activeBitrateKbps = quality.bitrateKbps
        snapshot.width = quality.width
        snapshot.height = quality.height
        snapshot.fps = quality.fps
        snapshot.format = .avc
    }

    private func setTally(_ tally: TallyState) {
        guard settings.showTally else { return }
        snapshot.tally = tally
    }

    private func noteListenFailed() {
        listenFailed = true
        statusMessage = .portInUse
        appendLog(String(localized: .portInUse))
        guard isRunning else { return }
        isRunning = false
        encoderActive = false
        Task {
            await encoder.stop()
            await capture.stop()
            await server.stop()
        }
    }

    private func setClient(_ address: String?, usb: Bool) {
        clientAddress = address
        isUSBClient = usb && address != nil
    }

    func markLocalNetworkExplained() {
        settings.explainedLocalNetwork = true
    }

    private func pauseForBackground() async {
        await server.dropVideo()
        encoderActive = false
        panelStream = false
        await encoder.stop()
        await capture.stop()
        snapshot.isStreaming = false
        snapshot.status = "waiting"
        curtainPresented = false
    }

    private func resumeFromBackground() async {
        do {
            try await capture.start(captureConfiguration)
        } catch {
            appendLog(String(localized: .startFailed))
            return
        }
        statusMessage = .waitingForOBS
    }

    private func showCurtain() {
        guard isRunning else { return }
        curtainPresented = true
    }

    private func hideCurtain() {
        curtainPresented = false
    }

    private func armCurtain() {
        curtainTask?.cancel()
        guard isRunning, settings.curtainMode != .off else { return }
        if settings.curtainMode == .on {
            showCurtain()
            return
        }
        lastCurtainArm = Date()
        curtainTask = Task { [weak self] in
            try? await Task.sleep(for: .seconds(8))
            guard !Task.isCancelled else { return }
            self?.showCurtain()
        }
    }

    private func watchCare() {
        careTasks.forEach { $0.cancel() }
        let names = [
            UIDevice.batteryLevelDidChangeNotification,
            UIDevice.batteryStateDidChangeNotification,
            ProcessInfo.thermalStateDidChangeNotification
        ]
        careTasks = names.map { name in
            Task { [weak self] in
                for await _ in NotificationCenter.default.notifications(named: name) {
                    self?.noteCareSignal(thermalChanged: name == ProcessInfo.thermalStateDidChangeNotification)
                }
            }
        }
    }

    private func noteCareSignal(thermalChanged: Bool) {
        let thermal = ProcessInfo.processInfo.thermalState
        let hot = thermal == .serious || thermal == .critical
        pushPortrait()
        guard thermalChanged else { return }
        if hot {
            if !isThermalHot {
                appendLog(String(localized: .thermalWarning))
            }
            isThermalHot = true
            statusMessage = .thermalWarning
        } else if isThermalHot {
            isThermalHot = false
            statusMessage = budget.step > 0 ? warmStatus : (snapshot.isStreaming ? .streaming : .waitingForOBS)
        }
    }

    private func watchBudget() {
        budgetTask?.cancel()
        budgetTask = Task { [weak self] in
            while !Task.isCancelled {
                try? await Task.sleep(for: .seconds(1))
                guard !Task.isCancelled else { return }
                await self?.noteBudgetSample()
            }
        }
    }

    private func noteBudgetSample() async {
        guard isRunning, encoderActive, panelStream else { return }
        let encodeMs = await encoder.averageEncodeMs()
        guard encodeMs > 0 else { return }
        let pace = ThermalBudget.pace(encodeMs: encodeMs, thermal: ProcessInfo.processInfo.thermalState)
        guard let step = budget.note(pace: pace, now: Date()) else { return }
        pushPortrait()
        let base = StreamQuality.panel(usb: isUSBClient).bitrateKbps
        let next = ThermalBudget.bitrateKbps(base: base, step: step)
        guard next != pacedBitrateKbps else { return }
        pacedBitrateKbps = next
        activeBitrateKbps = next
        await encoder.setBitrate(next)
        encoder.requestKeyframe()
        guard statusMessage != .pcCannotDecode else { return }
        statusMessage = step > 0 ? warmStatus : (snapshot.isStreaming ? .streaming : .waitingForOBS)
    }

    private var warmStatus: LocalizedStringResource {
        batteryIsCharging ? .phoneWarmCharging : .phoneWarmLighter
    }

    private var batteryIsCharging: Bool {
        let battery = UIDevice.current.batteryState
        return battery == .charging || battery == .full
    }

    private func pushPortrait() {
        let command = updatePortraitCare()
        guard command != lastPortraitCommand else { return }
        lastPortraitCommand = command
        Task { await capture.setPortrait(command) }
    }

    @discardableResult
    private func updatePortraitCare() -> PortraitCommand {
        let thermal = ProcessInfo.processInfo.thermalState
        let care = PortraitPolicy.care(
            enabled: settings.portraitEnabled,
            thermal: thermal,
            fps: settings.fps,
            height: settings.height,
            batteryPercent: BatteryPercent.current(),
            unplugged: UIDevice.current.batteryState == .unplugged
        )
        if care != portraitCare, care == .eased || care == .paused || care == .streamHot, let message = care.message {
            appendLog(String(localized: message))
        }
        portraitCare = care
        let grade = ThermalBudget.applying(
            step: budget.step,
            to: PortraitPolicy.grade(enabled: settings.portraitEnabled, thermal: thermal)
        )
        portraitPreviewActive = settings.portraitPhonePreview && (grade == .balanced || grade == .fast)
        return PortraitCommand(grade: grade, strength: settings.portraitStrength)
    }

    private func noteSettingsSaved(from old: StudioSettings) {
        guard settings != old else { return }
        StudioSettingsStore.save(settings)
        guard isRunning, old.port != settings.port || old.discoveryEnabled != settings.discoveryEnabled else { return }
        listenerRestart?.cancel()
        listenerRestart = Task { [weak self] in
            try? await Task.sleep(for: .milliseconds(400))
            guard !Task.isCancelled else { return }
            await self?.restartListener()
        }
    }

    private func restartListener() async {
        await server.stop()
        guard isRunning else { return }
        do {
            try await server.start(
                port: settings.port,
                advertise: settings.discoveryEnabled,
                deviceName: DeviceName.current
            )
        } catch {
            noteListenFailed()
        }
    }

    private func appendLog(_ message: String) {
        logs.append(LogEntry(message: message))
        if logs.count > 200 {
            logs.removeFirst(logs.count - 200)
        }
    }
}
