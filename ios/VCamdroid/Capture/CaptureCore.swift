import AVFoundation
import CoreMedia
import Foundation

final class CaptureCore: @unchecked Sendable {
    let previewSession: PreviewSession
    private let sessionQueue = DispatchQueue(label: "com.darusc.vcamdroid.capture")
    private let outputQueue = DispatchQueue(label: "com.darusc.vcamdroid.frames")
    private let videoOutput = AVCaptureVideoDataOutput()
    private let sink = CaptureVideoSink()
    private let portrait = PortraitProcessor()

    init(previewSession: PreviewSession) {
        self.previewSession = previewSession
        videoOutput.alwaysDiscardsLateVideoFrames = true
        videoOutput.videoSettings = [
            kCVPixelBufferPixelFormatTypeKey as String: kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange
        ]
    }

    func setFrameHandler(_ handler: @escaping @Sendable (CVPixelBuffer, CMTime) -> Void) {
        let portrait = portrait
        sink.onFrame = { pixelBuffer, time in
            handler(portrait.process(pixelBuffer), time)
        }
    }

    func setPortrait(_ command: PortraitCommand) {
        portrait.set(command)
    }

    func droppedFrames() -> Int {
        sink.dropped()
    }

    func perform<T: Sendable>(_ work: @escaping @Sendable () throws -> T) async throws -> T {
        try await withCheckedThrowingContinuation { (continuation: CheckedContinuation<T, Error>) in
            sessionQueue.async {
                do {
                    continuation.resume(returning: try work())
                } catch {
                    continuation.resume(throwing: error)
                }
            }
        }
    }

    func start(_ configuration: CaptureConfiguration) throws {
        sink.resetDropped()
        try apply(configuration, rebuild: true)
        applyRotation(configuration.rotationAngle)
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
        applyRotation(configuration.rotationAngle)
    }

    /// Covers the preview layer's connection too, which is recreated whenever the input changes.
    private func applyRotation(_ angle: CGFloat) {
        for connection in previewSession.session.connections where connection.isVideoRotationAngleSupported(angle) {
            connection.videoRotationAngle = angle
        }
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
        let device = try camera(for: configuration)
        let session = previewSession.session
        session.beginConfiguration()
        defer { session.commitConfiguration() }

        let current = (session.inputs.first as? AVCaptureDeviceInput)?.device
        if rebuild || session.inputs.isEmpty || current?.uniqueID != device.uniqueID {
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
        } else if let current {
            try current.lockForConfiguration()
            applyControls(configuration, on: current)
            current.unlockForConfiguration()
        }

        if let connection = videoOutput.connection(with: .video) {
            if connection.isVideoStabilizationSupported {
                connection.preferredVideoStabilizationMode = .off
            }
            if connection.isVideoMirroringSupported {
                connection.automaticallyAdjustsVideoMirroring = false
                connection.isVideoMirrored = false
            }
        }
    }

    private func camera(for configuration: CaptureConfiguration) throws -> AVCaptureDevice {
        let candidates = devices(for: configuration.lens)
        if configuration.manualExposure, let supported = candidates.first(where: { $0.isExposureModeSupported(.custom) }) {
            return supported
        }
        guard let device = candidates.first else { throw CaptureEngineError.cameraUnavailable }
        return device
    }

    private func devices(for lens: Lens) -> [AVCaptureDevice] {
        let types: [AVCaptureDevice.DeviceType]
        let position: AVCaptureDevice.Position
        switch lens {
        case .back:
            types = [.builtInTripleCamera, .builtInDualWideCamera, .builtInDualCamera, .builtInWideAngleCamera]
            position = .back
        case .front:
            types = [.builtInWideAngleCamera]
            position = .front
        }
        var seen: Set<String> = []
        return types.compactMap { AVCaptureDevice.default($0, for: .video, position: position) }.filter { seen.insert($0.uniqueID).inserted }
    }

    private func applyFormat(_ configuration: CaptureConfiguration, on device: AVCaptureDevice) throws {
        guard let format = matchingFormat(on: device, width: configuration.width, height: configuration.height, fps: configuration.fps) else {
            throw CaptureEngineError.formatUnavailable
        }
        device.activeFormat = format
        if device.isAutoVideoFrameRateEnabled, device.activeFormat.isAutoVideoFrameRateSupported {
            device.isAutoVideoFrameRateEnabled = false
        }
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
        applyZoom(configuration.zoom, on: device)
        applyTorch(configuration.torch, on: device)
        applyExposure(configuration, on: device)
        applyWhiteBalance(configuration, on: device)
        applyFocus(configuration, on: device)
    }

    private func applyZoom(_ zoom: Double, on device: AVCaptureDevice) {
        let upper = min(device.activeFormat.videoMaxZoomFactor, device.maxAvailableVideoZoomFactor)
        let lower = min(max(device.minAvailableVideoZoomFactor, 1), upper)
        guard upper > 0, upper >= lower else { return }
        device.videoZoomFactor = min(max(CGFloat(zoom), lower), upper)
    }

    private func applyTorch(_ on: Bool, on device: AVCaptureDevice) {
        if on, device.isTorchModeSupported(.on), device.isTorchAvailable {
            device.torchMode = .on
        } else if device.isTorchModeSupported(.off) {
            device.torchMode = .off
        }
    }

    private func applyExposure(_ configuration: CaptureConfiguration, on device: AVCaptureDevice) {
        if configuration.manualExposure, device.isExposureModeSupported(.custom) {
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
    }

    private func applyWhiteBalance(_ configuration: CaptureConfiguration, on device: AVCaptureDevice) {
        if configuration.manualWhiteBalance, canLockWhiteBalance(device, kelvin: configuration.kelvin) {
            let temperature = AVCaptureDevice.WhiteBalanceTemperatureAndTintValues(temperature: Float(configuration.kelvin), tint: 0)
            device.setWhiteBalanceModeLocked(whiteBalanceTemperatureAndTintValues: temperature, handler: nil)
        } else if device.isWhiteBalanceModeSupported(.continuousAutoWhiteBalance) {
            device.whiteBalanceMode = .continuousAutoWhiteBalance
        }
    }

    private func canLockWhiteBalance(_ device: AVCaptureDevice, kelvin: Double) -> Bool {
        guard device.isWhiteBalanceModeSupported(.locked), device.isLockingWhiteBalanceWithCustomDeviceGainsSupported else { return false }
        let temperature = AVCaptureDevice.WhiteBalanceTemperatureAndTintValues(temperature: Float(kelvin), tint: 0)
        let gains = device.deviceWhiteBalanceGains(for: temperature)
        let maximum = device.maxWhiteBalanceGain
        return [gains.redGain, gains.greenGain, gains.blueGain].allSatisfy { $0 >= 1 && $0 <= maximum }
    }

    private func applyFocus(_ configuration: CaptureConfiguration, on device: AVCaptureDevice) {
        if configuration.manualFocus, device.isFocusModeSupported(.locked), device.isLockingFocusWithCustomLensPositionSupported {
            let position = min(max(Float(configuration.lensPosition), 0), 1)
            device.setFocusModeLocked(lensPosition: position, completionHandler: nil)
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

    func controlLimits() -> DeviceControlLimits {
        guard let device = activeDevice() else { return .unknown }
        return DeviceControlLimits(
            customWhiteBalance: device.isLockingWhiteBalanceWithCustomDeviceGainsSupported,
            customFocus: device.isLockingFocusWithCustomLensPositionSupported,
            customExposure: device.isExposureModeSupported(.custom),
            torch: device.isTorchModeSupported(.on)
        )
    }

    private func activeDevice() -> AVCaptureDevice? {
        (previewSession.session.inputs.first as? AVCaptureDeviceInput)?.device
    }
}
