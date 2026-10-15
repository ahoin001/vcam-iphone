import Foundation

enum VideoRequestParser {
    static func parse(line: String, fallback: VideoMode) -> VideoRequest {
        let lowered = line.lowercased()
        let format: StreamFormat = (lowered.contains("hevc") || lowered.contains("h265")) ? .hevc : .avc
        if let size = firstSize(in: line) {
            return VideoRequest(format: format, width: size.width, height: size.height)
        }
        if lowered.contains("4k") || lowered.contains("2160p") {
            return VideoRequest(format: format, width: 3840, height: 2160)
        }
        if lowered.contains("1440p") {
            return VideoRequest(format: format, width: 2560, height: 1440)
        }
        if lowered.contains("1080p") {
            return VideoRequest(format: format, width: 1920, height: 1080)
        }
        if lowered.contains("720p") {
            return VideoRequest(format: format, width: 1280, height: 720)
        }
        if lowered.contains("480p") {
            return VideoRequest(format: format, width: 640, height: 480)
        }
        return VideoRequest(format: format, width: fallback.width, height: fallback.height)
    }

    /// Each call owns its own `Regex`. A stored pattern is not concurrency-safe: `Regex` is not `Sendable`.
    private static func firstSize(in line: String) -> (width: Int, height: Int)? {
        let sizePattern = /(?<width>\d{3,4})x(?<height>\d{3,4})/
        guard let match = try? sizePattern.firstMatch(in: line),
              let width = Int(match.width),
              let height = Int(match.height) else {
            return nil
        }
        return (width, height)
    }
}
