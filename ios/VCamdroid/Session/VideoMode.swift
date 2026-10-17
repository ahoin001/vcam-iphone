struct VideoMode: Codable, Equatable, Hashable, Sendable, Identifiable {
    var width: Int
    var height: Int
    var fps: Int

    var id: String { "\(width)x\(height)@\(fps)" }

    var label: String { "\(width)×\(height)" }

    /// Modes the picker offers until a 30-minute 1080p60 device run passes.
    static let v1Modes: [VideoMode] = [
        VideoMode(width: 1280, height: 720, fps: 30),
        VideoMode(width: 1920, height: 1080, fps: 30)
    ]

    /// Kept out of the picker until device acceptance enables it.
    static let highFrameRateCandidate = VideoMode(width: 1920, height: 1080, fps: 60)

    static func allowed(allowsHighFrameRate: Bool) -> [VideoMode] {
        allowsHighFrameRate ? v1Modes + [highFrameRateCandidate] : v1Modes
    }
}
