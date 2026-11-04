import CoreMedia
import CoreVideo
import Foundation
import VideoToolbox

final class EncoderCore: @unchecked Sendable {
    private let lock = NSLock()
    private var session: VTCompressionSession?
    private var sentConfiguration = false
    private var forceNextKeyframe = false
    private var peakMultiplier = 1
    private var encodeSamples: [Double] = []
    private var onPacket: (@Sendable (UInt64, Data) -> Void)?
    private let callback: VTCompressionOutputCallback = { refcon, sourceFrameRefcon, status, _, sampleBuffer in
        if let sourceFrameRefcon {
            let started = sourceFrameRefcon.assumingMemoryBound(to: Double.self)
            let elapsed = (CFAbsoluteTimeGetCurrent() - started.pointee) * 1000
            if let refcon {
                Unmanaged<EncoderCore>.fromOpaque(refcon).takeUnretainedValue().noteEncode(elapsed)
            }
            started.deinitialize(count: 1)
            started.deallocate()
        }
        guard status == noErr, let refcon, let sampleBuffer else { return }
        let core = Unmanaged<EncoderCore>.fromOpaque(refcon).takeUnretainedValue()
        core.emit(sampleBuffer)
    }

    func setHandler(_ handler: @escaping @Sendable (UInt64, Data) -> Void) {
        lock.lock()
        onPacket = handler
        lock.unlock()
    }

    func start(_ quality: StreamQuality, keyframeInterval: Int) throws {
        stop()
        var compression: VTCompressionSession?
        let specification = [
            kVTVideoEncoderSpecification_EnableLowLatencyRateControl: kCFBooleanTrue as Any
        ] as CFDictionary
        let status = VTCompressionSessionCreate(
            allocator: kCFAllocatorDefault,
            width: Int32(quality.width),
            height: Int32(quality.height),
            codecType: kCMVideoCodecType_H264,
            encoderSpecification: specification,
            imageBufferAttributes: nil,
            compressedDataAllocator: nil,
            outputCallback: callback,
            refcon: Unmanaged.passUnretained(self).toOpaque(),
            compressionSessionOut: &compression
        )
        guard status == noErr, let compression else { throw EncoderError.cannotCreate }
        let bitrate = quality.bitrateKbps * 1000
        let profile: CFString = quality.profile == .high
            ? kVTProfileLevel_H264_High_AutoLevel
            : kVTProfileLevel_H264_Baseline_AutoLevel
        let peak = max(quality.peakMultiplier, 1)
        set(kVTCompressionPropertyKey_RealTime, kCFBooleanTrue, on: compression)
        set(kVTCompressionPropertyKey_AllowFrameReordering, kCFBooleanFalse, on: compression)
        set(kVTCompressionPropertyKey_ProfileLevel, profile, on: compression)
        set(kVTCompressionPropertyKey_AverageBitRate, bitrate as CFNumber, on: compression)
        set(kVTCompressionPropertyKey_ExpectedFrameRate, quality.fps as CFNumber, on: compression)
        set(kVTCompressionPropertyKey_MaxKeyFrameInterval, (quality.fps * max(keyframeInterval, 1)) as CFNumber, on: compression)
        set(kVTCompressionPropertyKey_MaxKeyFrameIntervalDuration, keyframeInterval as CFNumber, on: compression)
        set(kVTCompressionPropertyKey_MaxFrameDelayCount, 0 as CFNumber, on: compression)
        let limits = [(bitrate / 8) * peak, 1] as CFArray
        set(kVTCompressionPropertyKey_DataRateLimits, limits, on: compression)
        set(kVTCompressionPropertyKey_ColorPrimaries, kCMFormatDescriptionColorPrimaries_ITU_R_709_2, on: compression)
        set(kVTCompressionPropertyKey_TransferFunction, kCMFormatDescriptionTransferFunction_ITU_R_709_2, on: compression)
        set(kVTCompressionPropertyKey_YCbCrMatrix, kCMFormatDescriptionYCbCrMatrix_ITU_R_709_2, on: compression)
        VTCompressionSessionPrepareToEncodeFrames(compression)
        lock.lock()
        session = compression
        peakMultiplier = peak
        encodeSamples.removeAll()
        sentConfiguration = false
        forceNextKeyframe = false
        lock.unlock()
    }

    func setBitrate(_ kbps: Int) {
        lock.lock()
        let current = session
        let peak = peakMultiplier
        lock.unlock()
        guard let current, kbps > 0 else { return }
        let bitrate = kbps * 1000
        set(kVTCompressionPropertyKey_AverageBitRate, bitrate as CFNumber, on: current)
        let limits = [(bitrate / 8) * max(peak, 1), 1] as CFArray
        set(kVTCompressionPropertyKey_DataRateLimits, limits, on: current)
    }

    func averageEncodeMs() -> Double {
        lock.lock()
        defer { lock.unlock() }
        guard !encodeSamples.isEmpty else { return 0 }
        return encodeSamples.reduce(0, +) / Double(encodeSamples.count)
    }

    func noteEncode(_ milliseconds: Double) {
        guard milliseconds.isFinite, milliseconds >= 0 else { return }
        lock.lock()
        encodeSamples.append(milliseconds)
        if encodeSamples.count > 30 {
            encodeSamples.removeFirst(encodeSamples.count - 30)
        }
        lock.unlock()
    }

    func forceKeyframe() {
        lock.lock()
        forceNextKeyframe = true
        lock.unlock()
    }

    func stop() {
        lock.lock()
        let current = session
        session = nil
        encodeSamples.removeAll()
        sentConfiguration = false
        forceNextKeyframe = false
        lock.unlock()
        guard let current else { return }
        VTCompressionSessionCompleteFrames(current, untilPresentationTimeStamp: .invalid)
        VTCompressionSessionInvalidate(current)
    }

    func submit(_ pixelBuffer: CVPixelBuffer, time: CMTime) {
        lock.lock()
        let current = session
        let force = forceNextKeyframe
        forceNextKeyframe = false
        lock.unlock()
        guard let current else { return }
        let properties: CFDictionary? = force
            ? [kVTEncodeFrameOptionKey_ForceKeyFrame: kCFBooleanTrue] as CFDictionary
            : nil
        let started = UnsafeMutablePointer<Double>.allocate(capacity: 1)
        started.initialize(to: CFAbsoluteTimeGetCurrent())
        let status = VTCompressionSessionEncodeFrame(
            current,
            imageBuffer: pixelBuffer,
            presentationTimeStamp: time,
            duration: .invalid,
            frameProperties: properties,
            sourceFrameRefcon: UnsafeMutableRawPointer(started),
            infoFlagsOut: nil
        )
        if status != noErr {
            started.deinitialize(count: 1)
            started.deallocate()
        }
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
