import AVFoundation
import CoreMedia
import Foundation

final class PortraitPreviewSink: @unchecked Sendable {
    private let lock = NSLock()
    private weak var renderer: AVSampleBufferVideoRenderer?
    private var active = false

    func attach(_ renderer: AVSampleBufferVideoRenderer) {
        lock.lock()
        self.renderer = renderer
        lock.unlock()
    }

    func setActive(_ active: Bool) {
        lock.lock()
        let changed = self.active != active
        self.active = active
        let renderer = renderer
        lock.unlock()
        guard changed, !active else { return }
        renderer?.flush(removingDisplayedImage: true, completionHandler: nil)
    }

    func enqueue(_ pixelBuffer: CVPixelBuffer, time: CMTime) {
        lock.lock()
        let active = active
        let renderer = renderer
        lock.unlock()
        guard active, let renderer, renderer.isReadyForMoreMediaData else { return }
        if renderer.status == .failed {
            renderer.flush(removingDisplayedImage: false, completionHandler: nil)
        }
        guard let sample = Self.sampleBuffer(from: pixelBuffer, time: time) else { return }
        renderer.enqueue(sample)
    }

    private static func sampleBuffer(from pixelBuffer: CVPixelBuffer, time: CMTime) -> CMSampleBuffer? {
        var format: CMVideoFormatDescription?
        guard CMVideoFormatDescriptionCreateForImageBuffer(
            allocator: kCFAllocatorDefault,
            imageBuffer: pixelBuffer,
            formatDescriptionOut: &format
        ) == noErr, let format else { return nil }
        var timing = CMSampleTimingInfo(duration: .invalid, presentationTimeStamp: time, decodeTimeStamp: .invalid)
        var sample: CMSampleBuffer?
        guard CMSampleBufferCreateReadyWithImageBuffer(
            allocator: kCFAllocatorDefault,
            imageBuffer: pixelBuffer,
            formatDescription: format,
            sampleTiming: &timing,
            sampleBufferOut: &sample
        ) == noErr, let sample else { return nil }
        if let attachments = CMSampleBufferGetSampleAttachmentsArray(sample, createIfNecessary: true),
           CFArrayGetCount(attachments) > 0 {
            let dictionary = unsafeBitCast(CFArrayGetValueAtIndex(attachments, 0), to: CFMutableDictionary.self)
            CFDictionarySetValue(
                dictionary,
                Unmanaged.passUnretained(kCMSampleAttachmentKey_DisplayImmediately).toOpaque(),
                Unmanaged.passUnretained(kCFBooleanTrue).toOpaque()
            )
        }
        return sample
    }
}
