import AVFoundation
import SwiftUI

struct CameraPreview: UIViewRepresentable {
    var session: AVCaptureSession
    var rotation: CGFloat
    var showsProcessed: Bool
    var sink: PortraitPreviewSink
    var onFocus: (CGPoint) -> Void

    func makeUIView(context: Context) -> CameraPreviewView {
        let view = CameraPreviewView()
        view.previewLayer.session = session
        view.previewLayer.videoGravity = .resizeAspect
        view.onFocus = onFocus
        sink.attach(view.processedRenderer)
        sink.setActive(showsProcessed)
        view.showsProcessedFrames = showsProcessed
        return view
    }

    func updateUIView(_ view: CameraPreviewView, context: Context) {
        if view.previewLayer.session !== session {
            view.previewLayer.session = session
        }
        view.onFocus = onFocus
        sink.attach(view.processedRenderer)
        sink.setActive(showsProcessed)
        view.showsProcessedFrames = showsProcessed
        guard let connection = view.previewLayer.connection else { return }
        if connection.isVideoRotationAngleSupported(rotation) {
            connection.videoRotationAngle = rotation
        }
        if connection.isVideoMirroringSupported {
            connection.automaticallyAdjustsVideoMirroring = false
            connection.isVideoMirrored = false
        }
    }
}
