struct StudioSettings: Equatable, Sendable {
    var width: Int = 1920
    var height: Int = 1080
    var fps: Int = 30
    var bitrateKbps: Int = 4000
    var keyframeInterval: Int = 1
    var port: Int = 4747
    var curtainMode: CurtainMode = .automatic
    var confirmStop: Bool = true
    var showTally: Bool = true
    var discoveryEnabled: Bool = true
    var allowsHighFrameRate: Bool = false
    var explainedLocalNetwork: Bool = false
    var lens: Lens = .back
    var zoom: Double = 1
    var exposureBias: Double = 0
    var manualExposure: Bool = false
    var iso: Double = 100
    var shutterSeconds: Double = 1.0 / 60.0
    var manualWhiteBalance: Bool = false
    var kelvin: Double = 5200
    var manualFocus: Bool = false
    var lensPosition: Double = 1
    var flip180: Bool = false
    var torch: Bool = false
    var portraitEnabled: Bool = false
    var portraitStrength: Double = 0.65
    var portraitPhonePreview: Bool = false
    var onboardingCompleted: Bool = false

    init() {}

    var mode: VideoMode {
        VideoMode(width: width, height: height, fps: fps)
    }

    func sanitized() -> StudioSettings {
        var copy = self
        let allowed = VideoMode.allowed(allowsHighFrameRate: allowsHighFrameRate)
        if !allowed.contains(copy.mode) {
            copy.width = 1920
            copy.height = 1080
            copy.fps = 30
        }
        copy.port = min(max(copy.port, 1), 65535)
        copy.bitrateKbps = min(max(copy.bitrateKbps, 2500), 16000)
        copy.keyframeInterval = max(copy.keyframeInterval, 1)
        copy.zoom = min(max(copy.zoom, 1), 10)
        copy.exposureBias = min(max(copy.exposureBias, -3), 3)
        copy.iso = min(max(copy.iso, 50), 6400)
        copy.shutterSeconds = min(max(copy.shutterSeconds, 1.0 / 10000), 0.5)
        copy.kelvin = min(max(copy.kelvin, 2000), 10000)
        copy.lensPosition = min(max(copy.lensPosition, 0), 1)
        copy.portraitStrength = min(max(copy.portraitStrength, 0), 1)
        return copy
    }
}

extension StudioSettings: Codable {
    private enum CodingKeys: String, CodingKey {
        case width, height, fps, bitrateKbps, keyframeInterval, port, curtainMode, confirmStop, showTally
        case discoveryEnabled, allowsHighFrameRate, explainedLocalNetwork, lens, zoom, exposureBias
        case manualExposure, iso, shutterSeconds, manualWhiteBalance, kelvin, manualFocus, lensPosition
        case flip180, torch, portraitEnabled, portraitStrength, portraitPhonePreview, onboardingCompleted
    }

    init(from decoder: Swift.Decoder) throws {
        self.init()
        let container = try decoder.container(keyedBy: CodingKeys.self)
        width = try container.decodeIfPresent(Int.self, forKey: .width) ?? width
        height = try container.decodeIfPresent(Int.self, forKey: .height) ?? height
        fps = try container.decodeIfPresent(Int.self, forKey: .fps) ?? fps
        bitrateKbps = try container.decodeIfPresent(Int.self, forKey: .bitrateKbps) ?? bitrateKbps
        keyframeInterval = try container.decodeIfPresent(Int.self, forKey: .keyframeInterval) ?? keyframeInterval
        port = try container.decodeIfPresent(Int.self, forKey: .port) ?? port
        curtainMode = try container.decodeIfPresent(CurtainMode.self, forKey: .curtainMode) ?? curtainMode
        confirmStop = try container.decodeIfPresent(Bool.self, forKey: .confirmStop) ?? confirmStop
        showTally = try container.decodeIfPresent(Bool.self, forKey: .showTally) ?? showTally
        discoveryEnabled = try container.decodeIfPresent(Bool.self, forKey: .discoveryEnabled) ?? discoveryEnabled
        allowsHighFrameRate = try container.decodeIfPresent(Bool.self, forKey: .allowsHighFrameRate) ?? allowsHighFrameRate
        explainedLocalNetwork = try container.decodeIfPresent(Bool.self, forKey: .explainedLocalNetwork) ?? explainedLocalNetwork
        lens = try container.decodeIfPresent(Lens.self, forKey: .lens) ?? lens
        zoom = try container.decodeIfPresent(Double.self, forKey: .zoom) ?? zoom
        exposureBias = try container.decodeIfPresent(Double.self, forKey: .exposureBias) ?? exposureBias
        manualExposure = try container.decodeIfPresent(Bool.self, forKey: .manualExposure) ?? manualExposure
        iso = try container.decodeIfPresent(Double.self, forKey: .iso) ?? iso
        shutterSeconds = try container.decodeIfPresent(Double.self, forKey: .shutterSeconds) ?? shutterSeconds
        manualWhiteBalance = try container.decodeIfPresent(Bool.self, forKey: .manualWhiteBalance) ?? manualWhiteBalance
        kelvin = try container.decodeIfPresent(Double.self, forKey: .kelvin) ?? kelvin
        manualFocus = try container.decodeIfPresent(Bool.self, forKey: .manualFocus) ?? manualFocus
        lensPosition = try container.decodeIfPresent(Double.self, forKey: .lensPosition) ?? lensPosition
        flip180 = try container.decodeIfPresent(Bool.self, forKey: .flip180) ?? flip180
        torch = try container.decodeIfPresent(Bool.self, forKey: .torch) ?? torch
        portraitEnabled = try container.decodeIfPresent(Bool.self, forKey: .portraitEnabled) ?? portraitEnabled
        portraitStrength = try container.decodeIfPresent(Double.self, forKey: .portraitStrength) ?? portraitStrength
        portraitPhonePreview = try container.decodeIfPresent(Bool.self, forKey: .portraitPhonePreview) ?? portraitPhonePreview
        onboardingCompleted = try container.decodeIfPresent(Bool.self, forKey: .onboardingCompleted) ?? onboardingCompleted
    }

    func encode(to encoder: Swift.Encoder) throws {
        var container = encoder.container(keyedBy: CodingKeys.self)
        try container.encode(width, forKey: .width)
        try container.encode(height, forKey: .height)
        try container.encode(fps, forKey: .fps)
        try container.encode(bitrateKbps, forKey: .bitrateKbps)
        try container.encode(keyframeInterval, forKey: .keyframeInterval)
        try container.encode(port, forKey: .port)
        try container.encode(curtainMode, forKey: .curtainMode)
        try container.encode(confirmStop, forKey: .confirmStop)
        try container.encode(showTally, forKey: .showTally)
        try container.encode(discoveryEnabled, forKey: .discoveryEnabled)
        try container.encode(allowsHighFrameRate, forKey: .allowsHighFrameRate)
        try container.encode(explainedLocalNetwork, forKey: .explainedLocalNetwork)
        try container.encode(lens, forKey: .lens)
        try container.encode(zoom, forKey: .zoom)
        try container.encode(exposureBias, forKey: .exposureBias)
        try container.encode(manualExposure, forKey: .manualExposure)
        try container.encode(iso, forKey: .iso)
        try container.encode(shutterSeconds, forKey: .shutterSeconds)
        try container.encode(manualWhiteBalance, forKey: .manualWhiteBalance)
        try container.encode(kelvin, forKey: .kelvin)
        try container.encode(manualFocus, forKey: .manualFocus)
        try container.encode(lensPosition, forKey: .lensPosition)
        try container.encode(flip180, forKey: .flip180)
        try container.encode(torch, forKey: .torch)
        try container.encode(portraitEnabled, forKey: .portraitEnabled)
        try container.encode(portraitStrength, forKey: .portraitStrength)
        try container.encode(portraitPhonePreview, forKey: .portraitPhonePreview)
        try container.encode(onboardingCompleted, forKey: .onboardingCompleted)
    }
}

extension StudioSettings {
    /// Panel video is 1080p30 and must not replace the saved DroidCam OBS size.
    func adopting(_ quality: StreamQuality) -> StudioSettings {
        guard quality.profile == .baseline else { return self }
        var copy = self
        copy.width = quality.width
        copy.height = quality.height
        copy.fps = quality.fps
        return copy
    }
}
