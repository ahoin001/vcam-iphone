import Foundation

struct SessionSnapshot: Equatable, Sendable {
    var status: String
    var isStreaming: Bool
    var width: Int
    var height: Int
    var fps: Int
    var format: StreamFormat
    var lens: Lens
    var batteryPercent: Int
    var tally: TallyState
    var panelClient = false
    /// Host clock, in milliseconds, using the same clock as each frame's capture time.
    var clockMs: UInt64 = 0
    var droppedFrames: Int = 0
    var thermal: String = "nominal"
    var captureDrops: Int = 0
    var portrait = false
    var manualExposure = false
    var shutter: Double = 0
    var torch = false
    var lowPower = false
    var charging = false

    static let waiting = SessionSnapshot(
        status: "waiting",
        isStreaming: false,
        width: 1920,
        height: 1080,
        fps: 30,
        format: .avc,
        lens: .back,
        batteryPercent: 100,
        tally: .idle
    )

    func json(success: Bool? = nil) -> String {
        var fields = ["\"status\":\"\(status)\""]
        if let success {
            fields.append("\"success\":\(success ? "true" : "false")")
        }
        fields.append("\"streaming\":\(isStreaming ? "true" : "false")")
        fields.append("\"width\":\(width)")
        fields.append("\"height\":\(height)")
        fields.append("\"fps\":\(fps)")
        fields.append("\"format\":\"\(format.rawValue)\"")
        fields.append("\"lens\":\"\(lens.rawValue)\"")
        fields.append("\"battery\":\(batteryPercent)")
        fields.append("\"tally\":\"\(tally.rawValue)\"")
        fields.append("\"clockMs\":\(clockMs)")
        fields.append("\"droppedFrames\":\(droppedFrames)")
        fields.append("\"thermal\":\"\(thermal)\"")
        fields.append("\"captureDrops\":\(captureDrops)")
        fields.append("\"portrait\":\(portrait ? "true" : "false")")
        fields.append("\"manualExposure\":\(manualExposure ? "true" : "false")")
        fields.append("\"shutter\":\(Self.number(shutter))")
        fields.append("\"torch\":\(torch ? "true" : "false")")
        fields.append("\"lowPower\":\(lowPower ? "true" : "false")")
        fields.append("\"charging\":\(charging ? "true" : "false")")
        if panelClient {
            fields.append("\"panelClient\":true")
        }
        return "{\(fields.joined(separator: ","))}"
    }

    private static func number(_ value: Double) -> String {
        var text = String(format: "%.4f", locale: Locale(identifier: "en_US_POSIX"), value)
        if text.contains(".") {
            while text.last == "0" {
                text.removeLast()
            }
            if text.last == "." {
                text.removeLast()
            }
        }
        if text == "-0" {
            return "0"
        }
        return text
    }

    var videoMode: VideoMode {
        VideoMode(width: width, height: height, fps: fps)
    }
}
