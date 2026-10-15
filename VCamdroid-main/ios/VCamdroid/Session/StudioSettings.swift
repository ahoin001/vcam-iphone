struct StudioSettings: Codable, Equatable, Sendable {
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
        copy.zoom = min(max(copy.zoom, 1), 16)
        copy.iso = min(max(copy.iso, 50), 6400)
        copy.shutterSeconds = min(max(copy.shutterSeconds, 1.0 / 10000), 1)
        copy.kelvin = min(max(copy.kelvin, 2000), 10000)
        copy.lensPosition = min(max(copy.lensPosition, 0), 1)
        return copy
    }
}
