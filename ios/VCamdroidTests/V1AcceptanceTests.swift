import Foundation
import Testing
@testable import VCamdroid

@Test func sessionJSONFieldOrder() {
    let snapshot = SessionSnapshot(
        status: "streaming",
        isStreaming: true,
        width: 1920,
        height: 1080,
        fps: 30,
        format: .avc,
        lens: .back,
        batteryPercent: 80,
        tally: .program
    )
    #expect(snapshot.json() == "{\"status\":\"streaming\",\"streaming\":true,\"width\":1920,\"height\":1080,\"fps\":30,\"format\":\"avc\",\"lens\":\"back\",\"battery\":80,\"tally\":\"program\",\"clockMs\":0,\"droppedFrames\":0,\"thermal\":\"nominal\"}")
    #expect(snapshot.json(success: true).contains("\"success\":true"))
}

@Test func controlJSONMatchesTheStudio() {
    let json = ControlState(settings: StudioSettings()).json()
    #expect(json == "{\"lens\":\"back\",\"zoom\":1,\"manualExposure\":false,\"exposureBias\":0,\"iso\":100,\"shutter\":0.0167,\"whiteBalance\":\"auto\",\"manualFocus\":false,\"focus\":1,\"torch\":false,\"flip\":false,\"portrait\":false,\"portraitStrength\":0.65,\"bitrateKbps\":4000,\"activeBitrateKbps\":4000}")
    #expect(ControlState(settings: StudioSettings()).json(success: true).hasPrefix("{\"success\":true,"))
}

@Test func controlJSONReportsWhiteBalancePreset() {
    var settings = StudioSettings()
    settings.manualWhiteBalance = true
    settings.kelvin = 6000
    settings.lens = .front
    settings.zoom = 2.5
    settings.torch = true
    #expect(ControlState(settings: settings).whiteBalance == .cloudy)
    let json = ControlState(settings: settings).json()
    #expect(json.contains("\"lens\":\"front\""))
    #expect(json.contains("\"zoom\":2.5"))
    #expect(json.contains("\"whiteBalance\":\"cloudy\""))
    #expect(json.contains("\"torch\":true"))
}

@Test func controlJSONReportsTheRateInUse() {
    let state = ControlState(settings: StudioSettings(), activeBitrateKbps: 12_000)
    #expect(state.bitrateKbps == 4000)
    #expect(state.json().contains("\"bitrateKbps\":4000"))
    #expect(state.json().contains("\"activeBitrateKbps\":12000"))
}

@Test func panelQualityFollowsTheTransport() {
    let usb = StreamQuality.panel(usb: true)
    #expect(usb == StreamQuality(width: 1920, height: 1080, fps: 30, bitrateKbps: 30_000, peakMultiplier: 2, profile: .high, keyframeInterval: 2))
    #expect(usb.keyframeInterval == 2)
    let wifi = StreamQuality.panel(usb: false)
    #expect(wifi.bitrateKbps == 12_000)
    #expect(wifi.fps == 30)
    #expect(wifi.profile == .high)
    #expect(wifi.peakMultiplier == 2)
    let legacy = StreamQuality.legacy(width: 1280, height: 720, fps: 30, bitrateKbps: 6000)
    #expect(legacy.profile == .baseline)
    #expect(legacy.peakMultiplier == 1)
    var stored = StudioSettings()
    stored.bitrateKbps = usb.bitrateKbps
    #expect(stored.sanitized().bitrateKbps == 16_000)
}

@Test func studioControlRangesClamp() {
    var settings = StudioSettings()
    settings.zoom = 20
    settings.exposureBias = 9
    settings.shutterSeconds = 2
    settings.iso = 10
    let clamped = settings.sanitized()
    #expect(clamped.zoom == 10)
    #expect(clamped.exposureBias == 3)
    #expect(clamped.shutterSeconds == 0.5)
    #expect(clamped.iso == 50)
    settings.exposureBias = -9
    settings.zoom = 0.2
    settings.portraitStrength = 4
    #expect(settings.sanitized().exposureBias == -3)
    #expect(settings.sanitized().zoom == 1)
    #expect(settings.sanitized().portraitStrength == 1)
    settings.portraitStrength = -2
    #expect(settings.sanitized().portraitStrength == 0)
}

@Test func defaultsMatchTheSpec() {
    let settings = StudioSettings()
    #expect(settings.width == 1920)
    #expect(settings.height == 1080)
    #expect(settings.fps == 30)
    #expect(settings.bitrateKbps == 4000)
    #expect(settings.keyframeInterval == 1)
    #expect(settings.port == 4747)
    #expect(settings.curtainMode == .automatic)
    #expect(settings.allowsHighFrameRate == false)
}

@Test func highFrameRateStaysOutOfThePicker() {
    #expect(!VideoMode.v1Modes.contains(VideoMode.highFrameRateCandidate))
    var settings = StudioSettings()
    settings.fps = 60
    #expect(settings.sanitized().fps == 30)
}

@Test func resolutionsJSONListsBothLenses() {
    let catalog = ResolutionCatalog(deviceName: "Apple iPhone 16 Pro", back: VideoMode.v1Modes, front: VideoMode.v1Modes)
    let json = catalog.json()
    #expect(json.contains("\"back\""))
    #expect(json.contains("\"front\""))
    #expect(json.contains("1920x1080@30fps"))
    #expect(json.contains("\"fullReport\":true"))
}

@Test func lockedCopyDoesNotPromisePowerButtonStreaming() {
    let text = String(localized: .streamPausedLocked)
    #expect(text.localizedStandardContains("Unlock"))
    #expect(text.localizedStandardContains("side button"))
    #expect(!text.localizedStandardContains("keeps streaming"))
}

@Test func v1AcceptanceChecklistCoversTheDevicePass() {
    #expect(DeviceAcceptance.checks.count == 10)
    #expect(DeviceAcceptance.checks.contains("1080p60 stays out of the picker until a 30-minute device run passes"))
}

@Test func controlUpdateReadsPortrait() {
    let request = DroidCamHTTP.parse(
        firstLine: "PUT /v1/controls?portrait=1&portraitStrength=0.4 HTTP/1.1",
        fallback: VideoMode(width: 1920, height: 1080, fps: 30)
    )
    #expect(request == .applyControls(ControlUpdate(portrait: true, portraitStrength: 0.4)))
}

@Test func oldSettingsLeaveOnboardingIncomplete() throws {
    var settings = StudioSettings()
    settings.width = 1280
    settings.height = 720
    settings.fps = 30
    settings.bitrateKbps = 6000
    settings.lens = .front
    settings.onboardingCompleted = true
    let data = try JSONEncoder().encode(settings)
    var object = try #require(JSONSerialization.jsonObject(with: data) as? [String: Any])
    object.removeValue(forKey: "onboardingCompleted")
    let stripped = try JSONSerialization.data(withJSONObject: object)
    let decoded = try JSONDecoder().decode(StudioSettings.self, from: stripped)
    #expect(decoded.onboardingCompleted == false)
    #expect(decoded.width == 1280)
    #expect(decoded.height == 720)
    #expect(decoded.fps == 30)
    #expect(decoded.bitrateKbps == 6000)
    #expect(decoded.lens == .front)
}

@Test func panelConnectionDoesNotReplaceSavedMode() {
    var settings = StudioSettings()
    settings.width = 1280
    settings.height = 720
    settings.fps = 30
    let panel = settings.adopting(.panel(usb: true))
    #expect(panel.width == 1280)
    #expect(panel.height == 720)
    #expect(panel.fps == 30)
    let legacy = settings.adopting(.legacy(width: 1920, height: 1080, fps: 60, bitrateKbps: 8000))
    #expect(legacy.width == 1920)
    #expect(legacy.height == 1080)
    #expect(legacy.fps == 60)
}

@Test func packetsLeaveInArrivalOrder() async {
    let forwarder = OrderedPacketForwarder()
    let sent = SentPackets()
    let task = Task {
        await forwarder.run { packet in
            if packet.pts == UInt64.max {
                try? await Task.sleep(for: .milliseconds(30))
            }
            await sent.append(packet.pts)
        }
    }
    forwarder.enqueue(pts: UInt64.max, payload: Data([7]))
    forwarder.enqueue(pts: 1, payload: Data([5]))
    forwarder.finish()
    await task.value
    #expect(await sent.values == [UInt64.max, 1])
}

@Test func fullQueueDropsUntilTheNextKeyframe() {
    let forwarder = OrderedPacketForwarder()
    let picture = Data([0, 0, 0, 1, 0x41])
    let keyframe = Data([0, 0, 0, 1, 0x65])
    let requests = OverflowCount()
    forwarder.setOverflowHandler {
        requests.add()
    }
    forwarder.enqueue(pts: 1, payload: picture)
    forwarder.enqueue(pts: 2, payload: picture)
    forwarder.enqueue(pts: 3, payload: picture)
    forwarder.enqueue(pts: 4, payload: picture)
    forwarder.enqueue(pts: 5, payload: picture)
    #expect(forwarder.pendingCount() == OrderedPacketForwarder.capacity)
    #expect(forwarder.dropped() == 2)
    #expect(requests.value == 1)
    forwarder.enqueue(pts: UInt64.max, payload: Data([7]))
    #expect(forwarder.pendingCount() == OrderedPacketForwarder.capacity)
    #expect(forwarder.dropped() == 3)
    forwarder.enqueue(pts: 6, payload: keyframe)
    #expect(forwarder.dropped() == 4)
    #expect(forwarder.pendingCount() == OrderedPacketForwarder.capacity)
    #expect(requests.value == 1)
}

private final class OverflowCount: @unchecked Sendable {
    private let lock = NSLock()
    private var count = 0

    func add() {
        lock.lock()
        count += 1
        lock.unlock()
    }

    var value: Int {
        lock.lock()
        defer { lock.unlock() }
        return count
    }
}

private actor SentPackets {
    private(set) var values: [UInt64] = []

    func append(_ pts: UInt64) {
        values.append(pts)
    }
}

@Test func storedSettingsKeepPortraitOptional() throws {
    var settings = StudioSettings()
    settings.lens = .front
    settings.zoom = 2
    let data = try JSONEncoder().encode(settings)
    var object = try #require(JSONSerialization.jsonObject(with: data) as? [String: Any])
    object.removeValue(forKey: "portraitEnabled")
    object.removeValue(forKey: "portraitStrength")
    object.removeValue(forKey: "portraitPhonePreview")
    let stripped = try JSONSerialization.data(withJSONObject: object)
    let decoded = try JSONDecoder().decode(StudioSettings.self, from: stripped)
    #expect(decoded.lens == .front)
    #expect(decoded.zoom == 2)
    #expect(decoded.portraitEnabled == false)
    #expect(decoded.portraitStrength == 0.65)
    #expect(decoded.portraitPhonePreview == false)
}

@Test func portraitPausesWhenThePhoneIsTooHot() {
    #expect(PortraitPolicy.grade(enabled: true, thermal: .critical) == .paused)
    #expect(PortraitPolicy.grade(enabled: true, thermal: .serious) == .fast)
    #expect(PortraitPolicy.grade(enabled: true, thermal: .nominal) == .balanced)
    #expect(PortraitPolicy.grade(enabled: false, thermal: .critical) == .off)
}

@Test func portraitCareNamesHeatBatteryAndResolution() {
    #expect(PortraitPolicy.care(enabled: false, thermal: .nominal, fps: 60, height: 2160, batteryPercent: 10, unplugged: true) == .hidden)
    #expect(PortraitPolicy.care(enabled: false, thermal: .serious, fps: 30, height: 1080, batteryPercent: 100, unplugged: false) == .streamHot)
    #expect(PortraitPolicy.care(enabled: true, thermal: .critical, fps: 30, height: 1080, batteryPercent: 100, unplugged: false) == .paused)
    #expect(PortraitPolicy.care(enabled: true, thermal: .serious, fps: 60, height: 2160, batteryPercent: 10, unplugged: true) == .eased)
    #expect(PortraitPolicy.care(enabled: true, thermal: .nominal, fps: 60, height: 1080, batteryPercent: 10, unplugged: true) == .heavierAndBattery)
    #expect(PortraitPolicy.care(enabled: true, thermal: .fair, fps: 30, height: 1080, batteryPercent: 15, unplugged: true) == .lowBattery)
    #expect(PortraitPolicy.care(enabled: true, thermal: .nominal, fps: 30, height: 1440, batteryPercent: 80, unplugged: false) == .heavierMode)
    #expect(PortraitPolicy.care(enabled: true, thermal: .nominal, fps: 30, height: 1080, batteryPercent: 80, unplugged: true) == .steady)
    #expect(PortraitPolicy.care(enabled: true, thermal: .nominal, fps: 30, height: 1080, batteryPercent: 19, unplugged: false) == .steady)
}
