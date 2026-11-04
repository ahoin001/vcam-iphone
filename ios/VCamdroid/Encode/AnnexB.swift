import Foundation

enum AnnexB {
    static let startCode = Data([0x00, 0x00, 0x00, 0x01])

    static func fromAVCC(_ data: Data) -> Data {
        var output = Data()
        var offset = 0
        while offset + 4 <= data.count {
            let length = data[offset..<offset + 4].reduce(0) { ($0 << 8) | Int($1) }
            offset += 4
            guard length > 0, offset + length <= data.count else { break }
            output.append(startCode)
            output.append(data[offset..<offset + length])
            offset += length
        }
        return output
    }

    static func parameterSets(_ sets: [Data]) -> Data {
        var output = Data()
        for set in sets where !set.isEmpty {
            output.append(startCode)
            output.append(set)
        }
        return output
    }
}
