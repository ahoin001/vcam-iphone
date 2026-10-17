import Foundation

enum DroidCamPacketFramer {
    static let headerSize = 12
    static let noPTS: UInt64 = .max

    static func frame(pts: UInt64, payload: Data) -> Data {
        var data = Data(capacity: headerSize + payload.count)
        var ptsBE = pts.bigEndian
        var lengthBE = UInt32(payload.count).bigEndian
        withUnsafeBytes(of: &ptsBE) { data.append(contentsOf: $0) }
        withUnsafeBytes(of: &lengthBE) { data.append(contentsOf: $0) }
        data.append(payload)
        return data
    }

    static func parse(_ data: Data) -> (pts: UInt64, payload: Data)? {
        guard data.count >= headerSize else { return nil }
        let pts = data.prefix(8).reduce(UInt64(0)) { ($0 << 8) | UInt64($1) }
        let length = data.dropFirst(8).prefix(4).reduce(UInt32(0)) { ($0 << 8) | UInt32($1) }
        let payloadEnd = headerSize + Int(length)
        guard payloadEnd <= data.count else { return nil }
        return (pts, data.subdata(in: headerSize..<payloadEnd))
    }
}
