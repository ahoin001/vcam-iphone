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
        if line.hasPrefix("GET /v1/controls") {
            return .controls
        }
        if line.hasPrefix("PUT /v1/controls") || line.hasPrefix("POST /v1/controls") {
            return .applyControls(controlUpdate(from: line))
        }
        if line.hasPrefix("GET /panel") {
            return .panel
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

    static func htmlReply(_ html: String) -> Data {
        let body = Data(html.utf8)
        var header = "HTTP/1.1 200 OK\r\n"
        header += "Content-Type: text/html; charset=utf-8\r\n"
        header += "Content-Length: \(body.count)\r\n"
        header += "Cache-Control: no-store\r\n"
        header += "Connection: close\r\n\r\n"
        var data = Data(header.utf8)
        data.append(body)
        return data
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

    private static func controlUpdate(from line: String) -> ControlUpdate {
        let query = queryItems(from: line)
        var update = ControlUpdate()
        if let value = query["lens"] ?? query["camera"] {
            update.lens = Lens(rawValue: value.lowercased())
        }
        if let value = query["zoom"], let number = finite(value) {
            update.zoom = number
        }
        if let value = query["manualexposure"], let flag = flag(value) {
            update.manualExposure = flag
        }
        if let value = query["exposurebias"] ?? query["ev"] ?? query["exposure"], let number = finite(value) {
            update.exposureBias = number
        }
        if let value = query["iso"], let number = finite(value) {
            update.iso = number
        }
        if let value = query["shutter"], let number = finite(value) {
            update.shutter = number
        }
        if let value = query["whitebalance"] ?? query["wb"] {
            update.whiteBalance = WhiteBalancePreset(rawValue: value.lowercased())
        }
        if let value = query["manualfocus"], let flag = flag(value) {
            update.manualFocus = flag
        }
        if let value = query["focus"], let number = finite(value) {
            update.focus = number
        }
        if let value = query["torch"], let flag = flag(value) {
            update.torch = flag
        }
        if let value = query["flip"], let flag = flag(value) {
            update.flip = flag
        }
        if let value = query["portrait"], let flag = flag(value) {
            update.portrait = flag
        }
        if let value = query["portraitstrength"], let number = finite(value) {
            update.portraitStrength = number
        }
        return update
    }

    private static func queryItems(from line: String) -> [String: String] {
        let parts = line.split(separator: " ")
        let path = parts.count >= 2 ? String(parts[1]) : line
        guard let queryIndex = path.firstIndex(of: "?") else { return [:] }
        let query = path[path.index(after: queryIndex)...]
        var items: [String: String] = [:]
        for pair in query.split(separator: "&") {
            let keyValue = pair.split(separator: "=", maxSplits: 1).map(String.init)
            guard keyValue.count == 2 else { continue }
            let value = keyValue[1].removingPercentEncoding ?? keyValue[1]
            items[keyValue[0].lowercased()] = value
        }
        return items
    }

    private static func finite(_ value: String) -> Double? {
        guard let number = Double(value), number.isFinite else { return nil }
        return number
    }

    private static func flag(_ value: String) -> Bool? {
        switch value.lowercased() {
        case "1", "true", "on", "yes":
            true
        case "0", "false", "off", "no":
            false
        default:
            nil
        }
    }

    private static func tally(from line: String) -> TallyState {
        let parts = line.split(separator: " ")
        let path = parts.count >= 2 ? String(parts[1]) : ""
        let raw = path.replacing("/v1/tally/", with: "").trimmingCharacters(in: CharacterSet(charactersIn: "/"))
        return TallyState(rawValue: raw.lowercased()) ?? .idle
    }
}
