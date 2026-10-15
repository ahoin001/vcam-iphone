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
    #expect(snapshot.json() == "{\"status\":\"streaming\",\"streaming\":true,\"width\":1920,\"height\":1080,\"fps\":30,\"format\":\"avc\",\"lens\":\"back\",\"battery\":80,\"tally\":\"program\"}")
    #expect(snapshot.json(success: true).contains("\"success\":true"))
}

@Test func controlJSONMatchesTheStudio() {
    let json = ControlState(settings: StudioSettings()).json()
    #expect(json == "{\"lens\":\"back\",\"zoom\":1,\"manualExposure\":false,\"exposureBias\":0,\"iso\":100,\"shutter\":0.0167,\"whiteBalance\":\"auto\",\"manualFocus\":false,\"focus\":1,\"torch\":false,\"flip\":false}")
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
    #expect(settings.sanitized().exposureBias == -3)
    #expect(settings.sanitized().zoom == 1)
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
