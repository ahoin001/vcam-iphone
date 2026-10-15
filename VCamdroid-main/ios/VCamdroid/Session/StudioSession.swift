import AVFoundation
import CoreMedia
import Foundation
import Observation
import SwiftUI

@MainActor
@Observable
final class StudioSession {
    let previewSession: PreviewSession
    private let capture: CaptureEngine
    private let encoder: Encoder
    private let server: DroidCamServer
    private let curtainController = ScreenCurtainController()

    var settings: StudioSettings
    private(set) var snapshot: SessionSnapshot
    private(set) var logs: [LogEntry] = []
    private(set) var clientAddress: String?
    private(set) var isUSBClient = false
    private(set) var statusMessage: LocalizedStringResource = .waitingForOBS
    private(set) var isRunning = false
    var curtainPresented = false
    private(set) var portraitCare: PortraitCare = .hidden
    private(set) var portraitPreviewActive = false
    let portraitPreview = PortraitPreviewSink()
    private var curtainTask: Task<Void, Never>?
    private var careTasks: [Task<Void, Never>] = []
    private var lastPortraitCommand = PortraitCommand.off
    private var encoderActive = false
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

    func start() async {
        guard !isRunning else { return }
        persist()
        listenFailed = false
        snapshot.batteryPercent = BatteryPercent.current()
        curtainController.disableIdleTimer()
        await server.configure(
            snapshot: { await self.currentSnapshot() },
            catalog: { await self.catalog() },
            onVideo: { request in await self.beginVideo(request) },
            onVideoStopped: { await self.endVideo() },
            onUpdate: { update in await self.apply(update: update) },
            controls: { await self.currentControls() },
            onControls: { update in await self.apply(controls: update) },
            onTally: { tally in await self.setTally(tally) },
            onClient: { address, usb in await self.setClient(address, usb: usb) },
            onListenFailed: { await self.noteListenFailed() }
        )
        await encoder.setHandler { [server] pts, payload in
            Task { await server.send(pts: pts, payload: payload) }
        }
        let preview = portraitPreview
        await capture.setFrameHandler { [encoder, preview] pixelBuffer, time in
            encoder.submit(pixelBuffer, time: time)
            preview.enqueue(pixelBuffer, time: time)
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
            try await capture.start(CaptureConfiguration(settings: settings))
            if listenFailed {
                await encoder.stop()
                await capture.stop()
                await server.stop()
                return
            }
            isRunning = true
            statusMessage = .waitingForOBS
            appendLog(String(localized: .waitingForOBS))
            armCurtain()
            watchCare()
        } catch DroidCamServerError.invalidPort, DroidCamServerError.portInUse {
            statusMessage = .portInUse
            appendLog(String(localized: .portInUse))
        } catch {
            statusMessage = .startFailed
            appendLog(String(localized: .startFailed))
        }
    }

    func stop() async {
        curtainTask?.cancel()
        careTasks.forEach { $0.cancel() }
        careTasks = []
        curtainPresented = false
        portraitPreviewActive = false
        portraitCare = .hidden
        lastPortraitCommand = .off
        curtainController.restore()
        curtainController.enableIdleTimer()
        encoderActive = false
        await capture.setPortrait(.off)
        await encoder.stop()
        await capture.stop()
        await server.stop()
        isRunning = false
        snapshot.isStreaming = false
        snapshot.status = "waiting"
        clientAddress = nil
        isUSBClient = false
        statusMessage = .waitingForOBS
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
        persist()
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
        showCurtain()
    }

    func applyControls() {
        persist()
        noteActivity()
        pushPortrait()
        guard isRunning else { return }
        Task {
            try? await capture.update(CaptureConfiguration(settings: settings))
        }
    }

    func applyPortrait() {
        let clean = settings.sanitized()
        if clean != settings {
            settings = clean
        }
        persist()
        noteActivity()
        pushPortrait()
    }

    func selectLens(_ lens: Lens) {
        settings.lens = lens
        snapshot.lens = lens
        applyControls()
    }

    func setMode(_ mode: VideoMode) {
        settings.width = mode.width
        settings.height = mode.height
        settings.fps = mode.fps
        snapshot.width = mode.width
        snapshot.height = mode.height
        snapshot.fps = mode.fps
        persist()
        applyControls()
        if encoderActive {
            Task { await restartEncoder() }
        }
    }

    func updateFrameRate(_ fps: Int) {
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
        var next = settings
        if let width = update.width { next.width = width }
        if let height = update.height { next.height = height }
        if let fps = update.fps { next.fps = fps }
        if let lens = update.lens { next.lens = lens }
        if let format = update.format, format != .avc { return false }
        let allowed = VideoMode.allowed(allowsHighFrameRate: next.allowsHighFrameRate)
        guard allowed.contains(next.mode) else { return false }
        settings = next.sanitized()
        snapshot.width = settings.width
        snapshot.height = settings.height
        snapshot.fps = settings.fps
        snapshot.lens = settings.lens
        snapshot.format = .avc
        persist()
        Task {
            try? await capture.update(CaptureConfiguration(settings: settings))
            if encoderActive {
                await restartEncoder()
            }
        }
        return true
    }

    func apply(controls update: ControlUpdate) -> Bool {
        if let lens = update.lens {
            settings.lens = lens
        }
        if let zoom = update.zoom {
            settings.zoom = zoom
        }
        if let manualExposure = update.manualExposure {
            settings.manualExposure = manualExposure
        }
        if let exposureBias = update.exposureBias {
            settings.exposureBias = exposureBias
        }
        if let iso = update.iso {
            settings.iso = iso
        }
        if let shutter = update.shutter {
            settings.shutterSeconds = shutter
        }
        if let whiteBalance = update.whiteBalance {
            if let kelvin = whiteBalance.kelvin {
                settings.manualWhiteBalance = true
                settings.kelvin = kelvin
            } else {
                settings.manualWhiteBalance = false
            }
        }
        if let manualFocus = update.manualFocus {
            settings.manualFocus = manualFocus
        }
        if let focus = update.focus {
            settings.lensPosition = focus
        }
        if let torch = update.torch {
            settings.torch = torch
        }
        if let flip = update.flip {
            settings.flip180 = flip
        }
        if let portrait = update.portrait {
            settings.portraitEnabled = portrait
        }
        if let portraitStrength = update.portraitStrength {
            settings.portraitStrength = portraitStrength
        }
        settings = settings.sanitized()
        snapshot.lens = settings.lens
        applyControls()
        return true
    }

    private func currentControls() -> ControlState {
        ControlState(settings: settings)
    }

    private func currentSnapshot() -> SessionSnapshot {
        var copy = snapshot
        copy.batteryPercent = BatteryPercent.current()
        return copy
    }

    private func catalog() -> ResolutionCatalog {
        let modes = VideoMode.allowed(allowsHighFrameRate: settings.allowsHighFrameRate)
        return ResolutionCatalog(deviceName: DeviceName.current, back: modes, front: modes)
    }

    private func beginVideo(_ request: VideoRequest) async {
        let allowed = VideoMode.allowed(allowsHighFrameRate: settings.allowsHighFrameRate)
        let requested = VideoMode(width: request.width, height: request.height, fps: settings.fps)
        guard request.format == .avc, allowed.contains(where: { $0.width == requested.width && $0.height == requested.height }) else {
            appendLog(String(localized: .unsupportedMode))
            return
        }
        settings.width = request.width
        settings.height = request.height
        snapshot.width = request.width
        snapshot.height = request.height
        snapshot.format = .avc
        snapshot.isStreaming = true
        snapshot.status = "streaming"
        persist()
        pushPortrait()
        do {
            try await capture.update(CaptureConfiguration(settings: settings))
            try await encoder.start(
                width: request.width,
                height: request.height,
                fps: settings.fps,
                bitrateKbps: settings.bitrateKbps,
                keyframeInterval: settings.keyframeInterval
            )
            encoderActive = true
            statusMessage = .streaming
            appendLog(String(localized: .clientConnected))
        } catch {
            encoderActive = false
            appendLog(String(localized: .startFailed))
        }
    }

    private func endVideo() async {
        encoderActive = false
        await encoder.stop()
        snapshot.isStreaming = false
        snapshot.status = "waiting"
        statusMessage = .waitingForOBS
        appendLog(String(localized: .clientDisconnected))
    }

    private func restartEncoder() async {
        await encoder.stop()
        do {
            try await encoder.start(
                width: settings.width,
                height: settings.height,
                fps: settings.fps,
                bitrateKbps: settings.bitrateKbps,
                keyframeInterval: settings.keyframeInterval
            )
            encoderActive = true
        } catch {
            encoderActive = false
            appendLog(String(localized: .startFailed))
        }
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
        persist()
    }

    private func pauseForBackground() async {
        encoderActive = false
        await encoder.stop()
        await capture.stop()
        snapshot.isStreaming = false
        snapshot.status = "waiting"
        curtainPresented = false
        curtainController.restore()
    }

    private func resumeFromBackground() async {
        do {
            try await capture.start(CaptureConfiguration(settings: settings))
        } catch {
            appendLog(String(localized: .startFailed))
            return
        }
        guard clientAddress != nil else {
            statusMessage = .waitingForOBS
            return
        }
        await encoder.markConfigurationUnsent()
        await restartEncoder()
        snapshot.isStreaming = true
        snapshot.status = "streaming"
        statusMessage = .streaming
        pushPortrait()
    }

    private func showCurtain() {
        curtainPresented = true
        curtainController.dim()
    }

    private func hideCurtain() {
        curtainPresented = false
        curtainController.restore()
    }

    private func armCurtain() {
        curtainTask?.cancel()
        guard isRunning, settings.curtainMode != .off else { return }
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
        let portraitWasOff = !settings.portraitEnabled
        let thermal = ProcessInfo.processInfo.thermalState
        pushPortrait()
        if thermalChanged, portraitWasOff, thermal == .serious || thermal == .critical {
            statusMessage = .thermalWarning
            appendLog(String(localized: .thermalWarning))
        }
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
        if care != portraitCare, care == .eased || care == .paused, let message = care.message {
            appendLog(String(localized: message))
        }
        portraitCare = care
        let grade = PortraitPolicy.grade(enabled: settings.portraitEnabled, thermal: thermal)
        portraitPreviewActive = grade == .balanced || grade == .fast
        return PortraitCommand(grade: grade, strength: settings.portraitStrength)
    }

    private func persist() {
        StudioSettingsStore.save(settings)
    }

    private func appendLog(_ message: String) {
        logs.append(LogEntry(message: message))
        if logs.count > 200 {
            logs.removeFirst(logs.count - 200)
        }
    }
}
