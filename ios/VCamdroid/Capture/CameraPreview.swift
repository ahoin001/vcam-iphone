import AVFoundation
import SwiftUI

struct CameraPreview: UIViewRepresentable {
    var session: AVCaptureSession
    var rotation: CGFloat
    var showsProcessed: Bool
    var attachPreview: Bool
    var sink: PortraitPreviewSink
    var onFocus: (CGPoint) -> Void

    func makeUIView(context: Context) -> CameraPreviewView {
        let view = CameraPreviewView()
        view.previewLayer.videoGravity = .resizeAspect
        view.previewLayer.session = attachPreview ? session : nil
        view.onFocus = onFocus
        sink.attach(view.processedRenderer)
        let showProcessed = showsProcessed && attachPreview
        sink.setActive(showProcessed)
        view.showsProcessedFrames = showProcessed
        return view
    }

    func updateUIView(_ view: CameraPreviewView, context: Context) {
        if attachPreview {
            if view.previewLayer.session !== session {
                view.previewLayer.session = session
            }
        } else if view.previewLayer.session != nil {
            view.previewLayer.session = nil
        }
        view.onFocus = onFocus
        sink.attach(view.processedRenderer)
        let showProcessed = showsProcessed && attachPreview
        sink.setActive(showProcessed)
        view.showsProcessedFrames = showProcessed
        guard attachPreview, let connection = view.previewLayer.connection else { return }
        if connection.isVideoRotationAngleSupported(rotation) {
            connection.videoRotationAngle = rotation
        }
        if connection.isVideoMirroringSupported {
            connection.automaticallyAdjustsVideoMirroring = false
            connection.isVideoMirrored = false
        }
    }
}
