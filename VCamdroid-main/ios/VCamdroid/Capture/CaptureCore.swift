import AVFoundation
import CoreMedia
import Foundation

final class CaptureCore: @unchecked Sendable {
    let previewSession: PreviewSession
    private let sessionQueue = DispatchQueue(label: "com.darusc.vcamdroid.capture")
    private let outputQueue = DispatchQueue(label: "com.darusc.vcamdroid.frames")
    private let videoOutput = AVCaptureVideoDataOutput()
    private let sink = CaptureVideoSink()

    init(previewSession: PreviewSession) {
        self.previewSession = previewSession
        videoOutput.alwaysDiscardsLateVideoFrames = true
        videoOutput.videoSettings = [
            kCVPixelBufferPixelFormatTypeKey as String: kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange
        ]
    }

    func setFrameHandler(_ handler: @escaping @Sendable (CVPixelBuffer, CMTime) -> Void) {
        sink.onFrame = handler
    }

    func perform(_ work: @escaping @Sendable () throws -> Void) async throws {
        try await withCheckedThrowingContinuation { (continuation: CheckedContinuation<Void, Error>) in
            sessionQueue.async {
                do {
                    try work()
                    continuation.resume()
                } catch {
                    continuation.resume(throwing: error)
                }
            }
        }
    }

    func start(_ configuration: CaptureConfiguration) throws {
        try apply(configuration, rebuild: true)
        if !previewSession.session.isRunning {
            previewSession.session.startRunning()
        }
    }

    func stop() {
        if previewSession.session.isRunning {
            previewSession.session.stopRunning()
        }
    }

    func update(_ configuration: CaptureConfiguration, rebuild: Bool) throws {
        try apply(configuration, rebuild: rebuild)
    }

    func focus(at devicePoint: CGPoint) {
        guard let device = activeDevice() else { return }
        guard (try? device.lockForConfiguration()) != nil else { return }
        defer { device.unlockForConfiguration() }
        guard device.isFocusPointOfInterestSupported else { return }
        device.focusPointOfInterest = devicePoint
        if device.isFocusModeSupported(.autoFocus) {
            device.focusMode = .autoFocus
        }
    }

    private func apply(_ configuration: CaptureConfiguration, rebuild: Bool) throws {
        let session = previewSession.session
        session.beginConfiguration()
        defer { session.commitConfiguration() }

        if rebuild || session.inputs.isEmpty {
            session.sessionPreset = .inputPriority
            for input in session.inputs {
                session.removeInput(input)
            }
            if !session.outputs.contains(videoOutput) {
                videoOutput.setSampleBufferDelegate(sink, queue: outputQueue)
                if session.canAddOutput(videoOutput) {
                    session.addOutput(videoOutput)
                }
            }
            let device = try camera(for: configuration.lens)
            let input = try AVCaptureDeviceInput(device: device)
            guard session.canAddInput(input) else { throw CaptureEngineError.cannotAddInput }
            session.addInput(input)
            try device.lockForConfiguration()
            do {
                try applyFormat(configuration, on: device)
                applyControls(configuration, on: device)
            } catch {
                device.unlockForConfiguration()
                throw error
            }
            device.unlockForConfiguration()
        } else if let device = activeDevice() {
            try device.lockForConfiguration()
            applyControls(configuration, on: device)
            device.unlockForConfiguration()
        }

        if let connection = videoOutput.connection(with: .video) {
            let angle: CGFloat = configuration.flip180 ? 180 : 0
            if connection.isVideoRotationAngleSupported(angle) {
                connection.videoRotationAngle = angle
            }
            if connection.isVideoStabilizationSupported {
                connection.preferredVideoStabilizationMode = .off
            }
            if connection.isVideoMirroringSupported {
                connection.automaticallyAdjustsVideoMirroring = false
                connection.isVideoMirrored = false
            }
        }
    }

    private func camera(for lens: Lens) throws -> AVCaptureDevice {
        let device: AVCaptureDevice?
        switch lens {
        case .back:
            device = AVCaptureDevice.default(.builtInTripleCamera, for: .video, position: .back)
                ?? AVCaptureDevice.default(.builtInDualWideCamera, for: .video, position: .back)
                ?? AVCaptureDevice.default(.builtInWideAngleCamera, for: .video, position: .back)
        case .front:
            device = AVCaptureDevice.default(.builtInWideAngleCamera, for: .video, position: .front)
        }
        guard let device else { throw CaptureEngineError.cameraUnavailable }
        return device
    }

    private func applyFormat(_ configuration: CaptureConfiguration, on device: AVCaptureDevice) throws {
        guard let format = matchingFormat(on: device, width: configuration.width, height: configuration.height, fps: configuration.fps) else {
            throw CaptureEngineError.formatUnavailable
        }
        device.activeFormat = format
        let frameDuration = CMTime(value: 1, timescale: CMTimeScale(configuration.fps))
        device.activeVideoMinFrameDuration = frameDuration
        device.activeVideoMaxFrameDuration = frameDuration
        device.automaticallyAdjustsVideoHDREnabled = false
        if format.isVideoHDRSupported {
            device.isVideoHDREnabled = false
        }
        if device.activeFormat.supportedColorSpaces.contains(.sRGB) {
            device.activeColorSpace = .sRGB
        }
    }

    private func applyControls(_ configuration: CaptureConfiguration, on device: AVCaptureDevice) {
        let maximumZoom = min(device.activeFormat.videoMaxZoomFactor, device.maxAvailableVideoZoomFactor)
        device.videoZoomFactor = min(max(CGFloat(configuration.zoom), 1), max(maximumZoom, 1))

        if device.hasTorch {
            device.torchMode = configuration.torch && device.isTorchAvailable ? .on : .off
        }

        if configuration.manualExposure {
            let iso = min(max(Float(configuration.iso), device.activeFormat.minISO), device.activeFormat.maxISO)
            var duration = CMTime(seconds: configuration.shutterSeconds, preferredTimescale: 1_000_000)
            if duration < device.activeFormat.minExposureDuration { duration = device.activeFormat.minExposureDuration }
            if duration > device.activeFormat.maxExposureDuration { duration = device.activeFormat.maxExposureDuration }
            device.setExposureModeCustom(duration: duration, iso: iso, completionHandler: nil)
        } else if device.isExposureModeSupported(.continuousAutoExposure) {
            device.exposureMode = .continuousAutoExposure
            let bias = min(max(Float(configuration.exposureBias), device.minExposureTargetBias), device.maxExposureTargetBias)
            device.setExposureTargetBias(bias, completionHandler: nil)
        }

        if configuration.manualWhiteBalance {
            let temperature = AVCaptureDevice.WhiteBalanceTemperatureAndTintValues(temperature: Float(configuration.kelvin), tint: 0)
            device.setWhiteBalanceModeLocked(with: clampedGains(device.deviceWhiteBalanceGains(for: temperature), on: device), completionHandler: nil)
        } else if device.isWhiteBalanceModeSupported(.continuousAutoWhiteBalance) {
            device.whiteBalanceMode = .continuousAutoWhiteBalance
        }

        if configuration.manualFocus {
            device.setFocusModeLocked(lensPosition: min(max(Float(configuration.lensPosition), 0), 1), completionHandler: nil)
        } else if device.isFocusModeSupported(.continuousAutoFocus) {
            device.focusMode = .continuousAutoFocus
        }
    }

    private func matchingFormat(on device: AVCaptureDevice, width: Int, height: Int, fps: Int) -> AVCaptureDevice.Format? {
        let target = Double(fps)
        return device.formats.first { format in
            let dimensions = CMVideoFormatDescriptionGetDimensions(format.formatDescription)
            let sizeMatches = Int(dimensions.width) == width && Int(dimensions.height) == height
            let rateMatches = format.videoSupportedFrameRateRanges.contains { $0.minFrameRate <= target && target <= $0.maxFrameRate }
            return sizeMatches && rateMatches
        }
    }

    private func clampedGains(_ gains: AVCaptureDevice.WhiteBalanceGains, on device: AVCaptureDevice) -> AVCaptureDevice.WhiteBalanceGains {
        let limit = device.maxWhiteBalanceGain
        func clamp(_ value: Float) -> Float { min(max(value, 1), limit) }
        return AVCaptureDevice.WhiteBalanceGains(redGain: clamp(gains.redGain), greenGain: clamp(gains.greenGain), blueGain: clamp(gains.blueGain))
    }

    private func activeDevice() -> AVCaptureDevice? {
        (previewSession.session.inputs.first as? AVCaptureDeviceInput)?.device
    }
}
