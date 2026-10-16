import CoreGraphics
import CoreImage
import CoreVideo
import Foundation

/// Holds the latest camera frame and encodes a throttled JPEG for the desk preview.
/// Encoding runs only when `jpegData()` is called, so idle desks cost almost nothing.
final class PreviewJPEGSink: @unchecked Sendable {
    private let lock = NSLock()
    private var latest: CVPixelBuffer?
    private var cachedJPEG: Data?
    private var lastEncode = Date.distantPast
    /// Desk glance feed only. Keep this well under the live stream rate so the phone stays free for encode.
    private let minInterval: TimeInterval = 1.0 / 12.0
    private let maxLongEdge = 480
    private let context = CIContext(options: [.cacheIntermediates: false])

    func enqueue(_ pixelBuffer: CVPixelBuffer) {
        lock.lock()
        latest = pixelBuffer
        lock.unlock()
    }

    func clear() {
        lock.lock()
        latest = nil
        cachedJPEG = nil
        lastEncode = .distantPast
        lock.unlock()
    }

    func jpegData() -> Data? {
        lock.lock()
        let buffer = latest
        let cached = cachedJPEG
        let elapsed = Date().timeIntervalSince(lastEncode)
        lock.unlock()

        if let cached, elapsed < minInterval {
            return cached
        }
        guard let buffer else { return cached }

        guard let encoded = encode(buffer) else { return cached }
        lock.lock()
        cachedJPEG = encoded
        lastEncode = Date()
        lock.unlock()
        return encoded
    }

    private func encode(_ pixelBuffer: CVPixelBuffer) -> Data? {
        let width = CVPixelBufferGetWidth(pixelBuffer)
        let height = CVPixelBufferGetHeight(pixelBuffer)
        guard width > 0, height > 0 else { return nil }

        var image = CIImage(cvPixelBuffer: pixelBuffer)
        let longEdge = max(width, height)
        if longEdge > maxLongEdge {
            let scale = CGFloat(maxLongEdge) / CGFloat(longEdge)
            image = image.transformed(by: CGAffineTransform(scaleX: scale, y: scale))
        }

        let extent = image.extent.integral
        guard extent.width > 1, extent.height > 1 else { return nil }
        return context.jpegRepresentation(
            of: image,
            colorSpace: CGColorSpaceCreateDeviceRGB(),
            options: [kCGImageDestinationLossyCompressionQuality as CIImageRepresentationOption: 0.55]
        )
    }
}
