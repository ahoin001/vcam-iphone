import CoreMedia
import CoreVideo
import Foundation

actor Encoder {
    nonisolated(unsafe) private let core = EncoderCore()

    func setHandler(_ handler: @escaping @Sendable (UInt64, Data) -> Void) {
        core.setHandler(handler)
    }

    func start(_ quality: StreamQuality, keyframeInterval: Int) throws {
        try core.start(quality, keyframeInterval: keyframeInterval)
    }

    func stop() {
        core.stop()
    }

    func setBitrate(_ kbps: Int) {
        core.setBitrate(kbps)
    }

    func averageEncodeMs() -> Double {
        core.averageEncodeMs()
    }

    func markConfigurationUnsent() {
        core.resetConfigurationFlag()
    }

    nonisolated func submit(_ pixelBuffer: CVPixelBuffer, time: CMTime) {
        core.submit(pixelBuffer, time: time)
    }

    nonisolated func requestKeyframe() {
        core.forceKeyframe()
    }
}
