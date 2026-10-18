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
        if panelClient {
            fields.append("\"panelClient\":true")
        }
        return "{\(fields.joined(separator: ","))}"
    }

    var videoMode: VideoMode {
        VideoMode(width: width, height: height, fps: fps)
    }
}
