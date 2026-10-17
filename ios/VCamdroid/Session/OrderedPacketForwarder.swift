import Foundation

struct VideoPacket: Sendable, Equatable {
    var pts: UInt64
    var payload: Data
}

/// Sends encoded packets one at a time, in the order they were enqueued.
/// A config packet therefore always leaves before the IDR that was queued after it.
final class OrderedPacketForwarder: @unchecked Sendable {
    private let stream: AsyncStream<VideoPacket>
    private let continuation: AsyncStream<VideoPacket>.Continuation

    init() {
        let pair = AsyncStream<VideoPacket>.makeStream()
        stream = pair.stream
        continuation = pair.continuation
    }

    func enqueue(pts: UInt64, payload: Data) {
        continuation.yield(VideoPacket(pts: pts, payload: payload))
    }

    func finish() {
        continuation.finish()
    }

    func run(_ send: @Sendable (VideoPacket) async -> Void) async {
        for await packet in stream {
            await send(packet)
        }
    }
}
