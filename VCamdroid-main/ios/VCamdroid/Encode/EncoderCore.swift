import CoreMedia
import CoreVideo
import Foundation
import VideoToolbox

final class EncoderCore: @unchecked Sendable {
    private let lock = NSLock()
    private var session: VTCompressionSession?
    private var sentConfiguration = false
    private var onPacket: (@Sendable (UInt64, Data) -> Void)?
    private let callback: VTCompressionOutputCallback = { refcon, _, status, _, sampleBuffer in
        guard status == noErr, let refcon, let sampleBuffer else { return }
        let core = Unmanaged<EncoderCore>.fromOpaque(refcon).takeUnretainedValue()
        core.emit(sampleBuffer)
    }

    func setHandler(_ handler: @escaping @Sendable (UInt64, Data) -> Void) {
        lock.lock()
        onPacket = handler
        lock.unlock()
    }

    func start(width: Int, height: Int, fps: Int, bitrateKbps: Int, keyframeInterval: Int) throws {
        stop()
        var compression: VTCompressionSession?
        let status = VTCompressionSessionCreate(
            allocator: kCFAllocatorDefault,
            width: Int32(width),
            height: Int32(height),
            codecType: kCMVideoCodecType_H264,
            encoderSpecification: nil,
            imageBufferAttributes: nil,
            compressedDataAllocator: nil,
            outputCallback: callback,
            refcon: Unmanaged.passUnretained(self).toOpaque(),
            compressionSessionOut: &compression
        )
        guard status == noErr, let compression else { throw EncoderError.cannotCreate }
        let bitrate = bitrateKbps * 1000
        set(kVTCompressionPropertyKey_RealTime, kCFBooleanTrue, on: compression)
        set(kVTCompressionPropertyKey_AllowFrameReordering, kCFBooleanFalse, on: compression)
        set(kVTCompressionPropertyKey_ProfileLevel, kVTProfileLevel_H264_Baseline_AutoLevel, on: compression)
        set(kVTCompressionPropertyKey_AverageBitRate, bitrate as CFNumber, on: compression)
        set(kVTCompressionPropertyKey_ExpectedFrameRate, fps as CFNumber, on: compression)
        set(kVTCompressionPropertyKey_MaxKeyFrameInterval, (fps * max(keyframeInterval, 1)) as CFNumber, on: compression)
        set(kVTCompressionPropertyKey_MaxKeyFrameIntervalDuration, keyframeInterval as CFNumber, on: compression)
        set(kVTCompressionPropertyKey_MaxFrameDelayCount, 0 as CFNumber, on: compression)
        let limits = [bitrate / 8, 1] as CFArray
        set(kVTCompressionPropertyKey_DataRateLimits, limits, on: compression)
        set(kVTCompressionPropertyKey_ColorPrimaries, kCMFormatDescriptionColorPrimaries_ITU_R_709_2, on: compression)
        set(kVTCompressionPropertyKey_TransferFunction, kCMFormatDescriptionTransferFunction_ITU_R_709_2, on: compression)
        set(kVTCompressionPropertyKey_YCbCrMatrix, kCMFormatDescriptionYCbCrMatrix_ITU_R_709_2, on: compression)
        VTCompressionSessionPrepareToEncodeFrames(compression)
        lock.lock()
        session = compression
        sentConfiguration = false
        lock.unlock()
    }

    func stop() {
        lock.lock()
        let current = session
        session = nil
        sentConfiguration = false
        lock.unlock()
        guard let current else { return }
        VTCompressionSessionCompleteFrames(current, untilPresentationTimeStamp: .invalid)
        VTCompressionSessionInvalidate(current)
    }

    func submit(_ pixelBuffer: CVPixelBuffer, time: CMTime) {
        lock.lock()
        let current = session
        lock.unlock()
        guard let current else { return }
        VTCompressionSessionEncodeFrame(
            current,
            imageBuffer: pixelBuffer,
            presentationTimeStamp: time,
            duration: .invalid,
            frameProperties: nil,
            sourceFrameRefcon: nil,
            infoFlagsOut: nil
        )
    }

    func resetConfigurationFlag() {
        lock.lock()
        sentConfiguration = false
        lock.unlock()
    }

    private func emit(_ sampleBuffer: CMSampleBuffer) {
        guard let format = CMSampleBufferGetFormatDescription(sampleBuffer),
              let block = CMSampleBufferGetDataBuffer(sampleBuffer) else { return }
        var total = 0
        var pointer: UnsafeMutablePointer<Int8>?
        guard CMBlockBufferGetDataPointer(block, atOffset: 0, lengthAtOffsetOut: nil, totalLengthOut: &total, dataPointerOut: &pointer) == kCMBlockBufferNoErr,
              let pointer else { return }
        let avcc = Data(bytes: pointer, count: total)
        let picture = AnnexB.fromAVCC(avcc)
        let pts = UInt64(max(0, CMTimeGetSeconds(CMSampleBufferGetPresentationTimeStamp(sampleBuffer)) * 1000))

        lock.lock()
        let handler = onPacket
        let needsConfig = !sentConfiguration
        if needsConfig { sentConfiguration = true }
        lock.unlock()

        if needsConfig {
            let config = AnnexB.parameterSets(Self.parameterSets(format))
            if !config.isEmpty {
                handler?(DroidCamPacketFramer.noPTS, config)
            }
        }
        if !picture.isEmpty {
            handler?(pts, picture)
        }
    }

    private static func parameterSets(_ format: CMFormatDescription) -> [Data] {
        var count = 0
        CMVideoFormatDescriptionGetH264ParameterSetAtIndex(
            format,
            parameterSetIndex: 0,
            parameterSetPointerOut: nil,
            parameterSetSizeOut: nil,
            parameterSetCountOut: &count,
            nalUnitHeaderLengthOut: nil
        )
        var sets: [Data] = []
        for index in 0..<count {
            var pointer: UnsafePointer<UInt8>?
            var size = 0
            let status = CMVideoFormatDescriptionGetH264ParameterSetAtIndex(
                format,
                parameterSetIndex: index,
                parameterSetPointerOut: &pointer,
                parameterSetSizeOut: &size,
                parameterSetCountOut: nil,
                nalUnitHeaderLengthOut: nil
            )
            if status == noErr, let pointer, size > 0 {
                sets.append(Data(bytes: pointer, count: size))
            }
        }
        return sets
    }

    private func set(_ key: CFString, _ value: CFTypeRef, on session: VTCompressionSession) {
        VTSessionSetProperty(session, key: key, value: value)
    }
}
