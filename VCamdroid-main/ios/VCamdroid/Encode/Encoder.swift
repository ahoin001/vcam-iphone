import CoreMedia
import CoreVideo
import Foundation

actor Encoder {
    nonisolated(unsafe) private let core = EncoderCore()

    func setHandler(_ handler: @escaping @Sendable (UInt64, Data) -> Void) {
        core.setHandler(handler)
    }

    func start(width: Int, height: Int, fps: Int, bitrateKbps: Int, keyframeInterval: Int) throws {
        try core.start(
            width: width,
            height: height,
            fps: fps,
            bitrateKbps: bitrateKbps,
            keyframeInterval: keyframeInterval
        )
    }

    func stop() {
        core.stop()
    }

    func markConfigurationUnsent() {
        core.resetConfigurationFlag()
    }

    nonisolated func submit(_ pixelBuffer: CVPixelBuffer, time: CMTime) {
        core.submit(pixelBuffer, time: time)
    }
}
