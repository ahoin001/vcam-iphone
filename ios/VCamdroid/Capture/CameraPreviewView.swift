import AVFoundation
import UIKit

final class CameraPreviewView: UIView {
    let previewLayer = AVCaptureVideoPreviewLayer()
    private let processedLayer = AVSampleBufferDisplayLayer()

    var processedRenderer: AVSampleBufferVideoRenderer {
        processedLayer.sampleBufferRenderer
    }

    var showsProcessedFrames = false {
        didSet {
            previewLayer.isHidden = showsProcessedFrames
            processedLayer.isHidden = !showsProcessedFrames
        }
    }

    var onFocus: ((CGPoint) -> Void)?

    override init(frame: CGRect) {
        super.init(frame: frame)
        backgroundColor = .black
        clipsToBounds = true
        isAccessibilityElement = false
        previewLayer.videoGravity = .resizeAspect
        processedLayer.videoGravity = .resizeAspect
        processedLayer.isHidden = true
        layer.addSublayer(previewLayer)
        layer.addSublayer(processedLayer)
        let tap = UITapGestureRecognizer(target: self, action: #selector(handleTap(_:)))
        addGestureRecognizer(tap)
    }

    override func layoutSubviews() {
        super.layoutSubviews()
        CATransaction.begin()
        CATransaction.setDisableActions(true)
        previewLayer.frame = bounds
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
