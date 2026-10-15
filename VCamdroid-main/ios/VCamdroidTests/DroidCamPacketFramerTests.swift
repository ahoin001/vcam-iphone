import Foundation
import Testing
@testable import VCamdroid

@Test func configPacketUsesNoPTS() {
    let payload = Data([0x00, 0x00, 0x00, 0x01, 0x67])
    let framed = DroidCamPacketFramer.frame(pts: DroidCamPacketFramer.noPTS, payload: payload)
    let parsed = DroidCamPacketFramer.parse(framed)
    #expect(framed.count == 17)
    #expect(framed.prefix(8) == Data(repeating: 0xFF, count: 8))
    #expect(parsed?.pts == DroidCamPacketFramer.noPTS)
    #expect(parsed?.payload == payload)
}

@Test func videoPacketRoundTrip() {
    let payload = Data([0x00, 0x00, 0x00, 0x01, 0x65, 0x88])
    let framed = DroidCamPacketFramer.frame(pts: 1_500, payload: payload)
    let parsed = DroidCamPacketFramer.parse(framed)
    #expect(parsed?.pts == 1_500)
    #expect(parsed?.payload == payload)
    #expect(framed[8] == 0)
    #expect(framed[11] == 6)
}

@Test func annexBRewritesLengthPrefixes() {
    var avcc = Data([0x00, 0x00, 0x00, 0x02, 0x65, 0x88])
    avcc.append(contentsOf: [0x00, 0x00, 0x00, 0x01, 0x41])
    let annex = AnnexB.fromAVCC(avcc)
    #expect(annex == Data([0x00, 0x00, 0x00, 0x01, 0x65, 0x88, 0x00, 0x00, 0x00, 0x01, 0x41]))
}
