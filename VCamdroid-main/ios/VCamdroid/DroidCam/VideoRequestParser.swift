import Foundation

enum VideoRequestParser {
    private static let sizePattern = /(?<width>\d{3,4})x(?<height>\d{3,4})/

    static func parse(line: String, fallback: VideoMode) -> VideoRequest {
        let lowered = line.lowercased()
        let format: StreamFormat = (lowered.contains("hevc") || lowered.contains("h265")) ? .hevc : .avc
        if let match = try? sizePattern.firstMatch(in: line),
           let width = Int(match.width),
           let height = Int(match.height) {
            return VideoRequest(format: format, width: width, height: height)
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
}
