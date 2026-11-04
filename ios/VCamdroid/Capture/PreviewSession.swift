import AVFoundation

final class PreviewSession: @unchecked Sendable {
    let session: AVCaptureSession

    init() {
        session = AVCaptureSession()
        session.automaticallyConfiguresCaptureDeviceForWideColor = false
    }
}
