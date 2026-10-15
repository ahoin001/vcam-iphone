import Foundation

enum DroidCamHTTP {
    static func parse(buffer: Data, fallback: VideoMode) -> DroidCamRequest {
        let text = String(decoding: buffer, as: UTF8.self)
        let firstLine = text.split(whereSeparator: \.isNewline).first.map(String.init) ?? ""
        return parse(firstLine: firstLine, fallback: fallback)
    }

    static func parse(firstLine: String, fallback: VideoMode) -> DroidCamRequest {
        let line = firstLine.trimmingCharacters(in: .whitespacesAndNewlines)
        if line.hasPrefix("GET /ping") {
            return .ping
        }
        if line.hasPrefix("GET /resolutions") || line.hasPrefix("GET /probe") || line.hasPrefix("GET /v1/resolutions") {
            return .resolutions
        }
        if line.hasPrefix("GET /capabilities") || line.hasPrefix("GET /v1/capabilities") || line.hasPrefix("GET /hal") {
            return .capabilities
        }
        if line.hasPrefix("GET /v1/session") {
            return .session
        }
        if line.hasPrefix("PUT /v1/session") || line.hasPrefix("POST /v1/session") {
            return .update(sessionUpdate(from: line))
        }
        if line.hasPrefix("GET /battery") {
            return .battery
        }
        if line.hasPrefix("PUT /v1/tally/") {
            return .tally(tally(from: line))
        }
        if line.contains("/video") {
            return .video(VideoRequestParser.parse(line: line, fallback: fallback))
        }
        if line.hasPrefix("GET /v2/audio") {
            return .audio
        }
        return .unknown
    }

    static func reply(status: Int, body: Data, contentType: String?) -> Data {
        var header = "HTTP/1.1 \(status) \(reason(status))\r\n"
        if let contentType {
            header += "Content-Type: \(contentType)\r\n"
        }
        header += "Content-Length: \(body.count)\r\nConnection: close\r\n\r\n"
        var data = Data(header.utf8)
        data.append(body)
        return data
    }

    static func jsonReply(_ json: String) -> Data {
        reply(status: 200, body: Data(json.utf8), contentType: "application/json")
    }

    private static func reason(_ status: Int) -> String {
        switch status {
        case 200: "OK"
        case 404: "Not Found"
        default: "OK"
        }
    }

    private static func sessionUpdate(from line: String) -> SessionUpdate {
        let parts = line.split(separator: " ")
        let path = parts.count >= 2 ? String(parts[1]) : line
        var update = SessionUpdate()
        if let queryIndex = path.firstIndex(of: "?") {
            let query = path[path.index(after: queryIndex)...]
            for pair in query.split(separator: "&") {
                let keyValue = pair.split(separator: "=", maxSplits: 1).map(String.init)
                guard keyValue.count == 2 else { continue }
                let key = keyValue[0].lowercased()
                let value = keyValue[1]
                switch key {
                case "width", "w":
                    update.width = Int(value)
                case "height", "h":
                    update.height = Int(value)
                case "fps":
                    update.fps = Int(value)
                case "format", "fmt":
                    update.format = StreamFormat(rawValue: value.lowercased())
                case "lens", "camera":
                    update.lens = Lens(rawValue: value.lowercased())
                default:
                    break
                }
            }
        }
        if update.width == nil && update.height == nil {
            let parsed = VideoRequestParser.parse(line: path, fallback: VideoMode(width: 0, height: 0, fps: 0))
            if parsed.width > 0 && parsed.height > 0 && (path.contains("x") || path.lowercased().contains("p") || path.lowercased().contains("4k")) {
                update.width = parsed.width
                update.height = parsed.height
            }
        }
        return update
    }

    private static func tally(from line: String) -> TallyState {
        let parts = line.split(separator: " ")
        let path = parts.count >= 2 ? String(parts[1]) : ""
        let raw = path.replacing("/v1/tally/", with: "").trimmingCharacters(in: CharacterSet(charactersIn: "/"))
        return TallyState(rawValue: raw.lowercased()) ?? .idle
    }
}
