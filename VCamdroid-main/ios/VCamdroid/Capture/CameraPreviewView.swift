import AVFoundation
import UIKit

final class CameraPreviewView: UIView {
    override class var layerClass: AnyClass { AVCaptureVideoPreviewLayer.self }

    private let processedLayer = AVSampleBufferDisplayLayer()

    var previewLayer: AVCaptureVideoPreviewLayer {
        guard let preview = layer as? AVCaptureVideoPreviewLayer else {
            preconditionFailure("CameraPreviewView requires an AVCaptureVideoPreviewLayer")
        }
        return preview
    }

    var processedRenderer: AVSampleBufferVideoRenderer {
        processedLayer.sampleBufferRenderer
    }

    var showsProcessedFrames = false {
        didSet {
            previewLayer.opacity = showsProcessedFrames ? 0 : 1
            processedLayer.isHidden = !showsProcessedFrames
        }
    }

    var onFocus: ((CGPoint) -> Void)?

    override init(frame: CGRect) {
        super.init(frame: frame)
        backgroundColor = .black
        isAccessibilityElement = false
        processedLayer.videoGravity = .resizeAspect
        processedLayer.isHidden = true
        layer.addSublayer(processedLayer)
        let tap = UITapGestureRecognizer(target: self, action: #selector(handleTap(_:)))
        addGestureRecognizer(tap)
    }

    override func layoutSubviews() {
        super.layoutSubviews()
        CATransaction.begin()
        CATransaction.setDisableActions(true)
        processedLayer.frame = bounds
        CATransaction.commit()
    }

    required init?(coder: NSCoder) {
        nil
    }

    @objc private func handleTap(_ recognizer: UITapGestureRecognizer) {
        let point = recognizer.location(in: self)
        onFocus?(previewLayer.captureDevicePointConverted(fromLayerPoint: point))
    }
}
