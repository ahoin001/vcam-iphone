import Foundation

struct VideoPacket: Sendable, Equatable {
    var pts: UInt64
    var payload: Data
}

/// Sends encoded packets one at a time, in the order they were enqueued.
/// A config packet therefore always leaves before the IDR that was queued after it.
/// The waiting queue holds at most three packets. Past that, pictures are dropped until the next keyframe so a stall cannot grow the delay.
final class OrderedPacketForwarder: @unchecked Sendable {
    static let capacity = 3

    private let lock = NSLock()
    private var pending: [VideoPacket] = []
    private var waiters: [CheckedContinuation<VideoPacket?, Never>] = []
    private var finished = false
    private var droppingUntilKeyframe = false
    private var requestedKeyframe = false
    private var droppedCount = 0
    private var onOverflow: (@Sendable () -> Void)?

    func setOverflowHandler(_ handler: @escaping @Sendable () -> Void) {
        lock.lock()
        onOverflow = handler
        lock.unlock()
    }

    func dropped() -> Int {
        lock.lock()
        defer { lock.unlock() }
        return droppedCount
    }

    func pendingCount() -> Int {
        lock.lock()
        defer { lock.unlock() }
        return pending.count
    }

    func enqueue(pts: UInt64, payload: Data) {
        let packet = VideoPacket(pts: pts, payload: payload)
        lock.lock()
        if finished {
            lock.unlock()
            return
        }
        let config = pts == UInt64.max
        let keyframe = Self.isKeyframe(payload)
        if !config && droppingUntilKeyframe && !keyframe {
            noteDropLocked()
            let handler = takeOverflowLocked()
            lock.unlock()
            handler?()
            return
        }
        if !config && pending.count >= Self.capacity && !keyframe {
            droppingUntilKeyframe = true
            noteDropLocked()
            let handler = takeOverflowLocked()
            lock.unlock()
            handler?()
            return
        }
        if !config && keyframe {
            droppingUntilKeyframe = false
            requestedKeyframe = false
            while pending.count >= Self.capacity {
                guard discardPictureLocked() else { break }
            }
        }
        if config {
            while pending.count >= Self.capacity {
                guard discardPictureLocked() else { break }
            }
        }
        if let waiter = waiters.first {
            waiters.removeFirst()
            lock.unlock()
            waiter.resume(returning: packet)
            return
        }
        pending.append(packet)
        lock.unlock()
    }

    func finish() {
        lock.lock()
        finished = true
        let parked = waiters
        waiters.removeAll()
        lock.unlock()
        for waiter in parked {
            waiter.resume(returning: nil)
        }
    }

    func run(_ send: @Sendable (VideoPacket) async -> Void) async {
        while let packet = await next() {
            await send(packet)
        }
    }

    private enum NextStep {
        case packet(VideoPacket)
        case finished
        case parked
    }

    /// Locking stays in this synchronous function. `NSLock` cannot be taken from an async function.
    private func step(_ continuation: CheckedContinuation<VideoPacket?, Never>?) -> NextStep {
        lock.lock()
        defer { lock.unlock() }
        if !pending.isEmpty {
            return .packet(pending.removeFirst())
        }
        if finished {
            return .finished
        }
        if let continuation {
            waiters.append(continuation)
        }
        return .parked
    }

    private func next() async -> VideoPacket? {
        switch step(nil) {
        case .packet(let packet):
            return packet
        case .finished:
            return nil
        case .parked:
            break
        }
        return await withCheckedContinuation { continuation in
            switch step(continuation) {
            case .packet(let packet):
                continuation.resume(returning: packet)
            case .finished:
                continuation.resume(returning: nil)
            case .parked:
                break
            }
        }
    }

    private func noteDropLocked() {
        droppedCount += 1
    }

    /// Returns the overflow handler the first time a stall asks for a new keyframe.
    private func takeOverflowLocked() -> (@Sendable () -> Void)? {
        guard !requestedKeyframe else { return nil }
        requestedKeyframe = true
        return onOverflow
    }

    @discardableResult
    private func discardPictureLocked() -> Bool {
        guard let index = pending.firstIndex(where: { $0.pts != UInt64.max }) else { return false }
        pending.remove(at: index)
        droppedCount += 1
        return true
    }

    static func isKeyframe(_ payload: Data) -> Bool {
        let bytes = [UInt8](payload)
        var index = 0
        while index + 3 < bytes.count {
            let longCode = index + 4 < bytes.count && bytes[index] == 0 && bytes[index + 1] == 0 && bytes[index + 2] == 0 && bytes[index + 3] == 1
            let shortCode = bytes[index] == 0 && bytes[index + 1] == 0 && bytes[index + 2] == 1
            if longCode || shortCode {
                let header = index + (longCode ? 4 : 3)
                if header < bytes.count && (bytes[header] & 0x1f) == 5 {
                    return true
                }
                index = header + 1
            } else {
                index += 1
            }
        }
        return false
    }
}
