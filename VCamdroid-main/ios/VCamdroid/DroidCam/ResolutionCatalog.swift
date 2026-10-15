import Foundation

struct ResolutionCatalog: Equatable, Sendable {
    var deviceName: String
    var back: [VideoMode]
    var front: [VideoMode]

    func json() -> String {
        let backList = back.map { "\"\($0.width)x\($0.height)@\($0.fps)fps\"" }.joined(separator: ",")
        let frontList = front.map { "\"\($0.width)x\($0.height)@\($0.fps)fps\"" }.joined(separator: ",")
        let cameras = (back.isEmpty ? 0 : 1) + (front.isEmpty ? 0 : 1)
        return "{\"back\":[\(backList)],\"front\":[\(frontList)],\"device\":\"\(deviceName)\",\"cameras\":\(cameras),\"fullReport\":true}"
    }

    func capabilitiesJSON() -> String {
        func modes(_ lens: Lens, _ list: [VideoMode]) -> String {
            list.map { mode in
                "{\"lens\":\"\(lens.rawValue)\",\"width\":\(mode.width),\"height\":\(mode.height),\"fps\":\(mode.fps)}"
            }.joined(separator: ",")
        }
        let all = modes(.back, back) + (back.isEmpty || front.isEmpty ? "" : ",") + modes(.front, front)
        let cameras = (back.isEmpty ? 0 : 1) + (front.isEmpty ? 0 : 1)
        return "{\"platform\":\"ios\",\"device\":\"\(deviceName)\",\"cameras\":\(cameras),\"formats\":[\"avc\"],\"modes\":[\(all)]}"
    }
}
