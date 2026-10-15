import AVFoundation
import UIKit

final class CameraPreviewView: UIView {
    override class var layerClass: AnyClass { AVCaptureVideoPreviewLayer.self }

    var previewLayer: AVCaptureVideoPreviewLayer {
        guard let preview = layer as? AVCaptureVideoPreviewLayer else {
            preconditionFailure("CameraPreviewView requires an AVCaptureVideoPreviewLayer")
        }
        return preview
    }

    var onFocus: ((CGPoint) -> Void)?

    override init(frame: CGRect) {
        super.init(frame: frame)
        backgroundColor = .black
        isAccessibilityElement = false
        let tap = UITapGestureRecognizer(target: self, action: #selector(handleTap(_:)))
        addGestureRecognizer(tap)
    }

    required init?(coder: NSCoder) {
        nil
    }

    @objc private func handleTap(_ recognizer: UITapGestureRecognizer) {
        let point = recognizer.location(in: self)
        onFocus?(previewLayer.captureDevicePointConverted(fromLayerPoint: point))
    }
}
