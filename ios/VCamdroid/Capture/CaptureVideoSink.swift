import AVFoundation
import CoreMedia
import Foundation

final class CaptureVideoSink: NSObject, AVCaptureVideoDataOutputSampleBufferDelegate, @unchecked Sendable {
    var onFrame: (@Sendable (CVPixelBuffer, CMTime) -> Void)?
    private let lock = NSLock()
    private var droppedCount = 0

    func dropped() -> Int {
        lock.lock()
        defer { lock.unlock() }
        return droppedCount
    }

    func resetDropped() {
        lock.lock()
        droppedCount = 0
        lock.unlock()
    }

    func noteDropped() {
        lock.lock()
        droppedCount += 1
        lock.unlock()
    }

    func captureOutput(
        _ output: AVCaptureOutput,
        didOutput sampleBuffer: CMSampleBuffer,
        from connection: AVCaptureConnection
    ) {
        guard let pixelBuffer = CMSampleBufferGetImageBuffer(sampleBuffer) else { return }
        let time = CMSampleBufferGetPresentationTimeStamp(sampleBuffer)
        onFrame?(pixelBuffer, time)
    }

    func captureOutput(
        _ output: AVCaptureOutput,
        didDrop sampleBuffer: CMSampleBuffer,
        from connection: AVCaptureConnection
    ) {
        _ = output
        _ = sampleBuffer
        _ = connection
        noteDropped()
    }
}
