import CoreMedia
import Foundation

enum StreamPace: Equatable, Sendable {
    case cool
    case steady
    case warm
}

/// Decides when a long session should spend fewer bits. It never changes picture size or frame rate.
struct ThermalBudget: Equatable, Sendable {
    var step = 0
    private var warmSamples = 0
    private var coolSince: Date?

    static let warmEncodeMs = 28.0
    static let steadyEncodeMs = 20.0
    static let warmSamplesBeforeStep = 3
    static let coolHold: TimeInterval = 20
    static let maxStep = 2

    static func pace(encodeMs: Double, thermal: ProcessInfo.ThermalState) -> StreamPace {
        switch thermal {
        case .serious, .critical:
            return .warm
        case .fair:
            return encodeMs >= warmEncodeMs ? .warm : .steady
        default:
            if encodeMs >= warmEncodeMs { return .warm }
            if encodeMs >= steadyEncodeMs { return .steady }
            return .cool
        }
    }

    static func bitrateKbps(base: Int, step: Int) -> Int {
        let steps: [Int]
        if base >= 30_000 {
            steps = [30_000, 16_000, 8_000]
        } else if base >= 12_000 {
            steps = [12_000, 8_000, 5_000]
        } else {
            steps = [max(base, 5_000)]
        }
        let index = min(max(step, 0), steps.count - 1)
        return max(steps[index], 5_000)
    }

    static func applying(step: Int, to grade: PortraitGrade) -> PortraitGrade {
        switch grade {
        case .off:
            return .off
        case .paused:
            return .paused
        case .balanced, .fast:
            if step >= 2 { return .paused }
            if step >= 1 { return .fast }
            return grade
        }
    }

    static func frameIsLate(_ time: CMTime, frameSeconds: Double, now: CMTime) -> Bool {
        let pts = CMTimeGetSeconds(time)
        let host = CMTimeGetSeconds(now)
        guard pts.isFinite, host.isFinite, frameSeconds > 0 else { return false }
        return host - pts > frameSeconds * 2
    }

    /// Returns the new step when the rate should change.
    mutating func note(pace: StreamPace, now: Date) -> Int? {
        switch pace {
        case .warm:
            coolSince = nil
            warmSamples += 1
            guard warmSamples >= Self.warmSamplesBeforeStep, step < Self.maxStep else { return nil }
            warmSamples = 0
            step += 1
            return step
        case .steady:
            warmSamples = 0
            coolSince = nil
            return nil
        case .cool:
            warmSamples = 0
            if coolSince == nil {
                coolSince = now
            }
            guard let since = coolSince, now.timeIntervalSince(since) >= Self.coolHold, step > 0 else { return nil }
            coolSince = now
            step -= 1
            return step
        }
    }
}
