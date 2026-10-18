import CoreMedia
import Foundation

/// Capture-queue flags for work the computer never sees.
final class StreamGate: @unchecked Sendable {
    private let lock = NSLock()
    private var skipDesk = false
    private var frameSeconds = 1.0 / 30.0
    private var lateDrops = 0

    func setSkipDesk(_ skip: Bool) {
        lock.lock()
        skipDesk = skip
        lock.unlock()
    }

    func setFrameSeconds(_ seconds: Double) {
        guard seconds > 0 else { return }
        lock.lock()
        frameSeconds = seconds
        lock.unlock()
    }

    var skipsDesk: Bool {
        lock.lock()
        defer { lock.unlock() }
        return skipDesk
    }

    var drops: Int {
        lock.lock()
        defer { lock.unlock() }
        return lateDrops
    }

    func shouldEncode(_ time: CMTime) -> Bool {
        lock.lock()
        let frame = frameSeconds
        lock.unlock()
        let now = CMClockGetTime(CMClockGetHostTimeClock())
        guard ThermalBudget.frameIsLate(time, frameSeconds: frame, now: now) else { return true }
        lock.lock()
        lateDrops += 1
        lock.unlock()
        return false
    }
}
