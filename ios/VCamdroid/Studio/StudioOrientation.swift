import SwiftUI
import UIKit

@MainActor
enum OrientationLock {
    private(set) static var mask: UIInterfaceOrientationMask = .allButUpsideDown

    static func set(_ mask: UIInterfaceOrientationMask, in scene: UIWindowScene) {
        guard mask != self.mask else { return }
        self.mask = mask
        scene.keyWindow?.rootViewController?.setNeedsUpdateOfSupportedInterfaceOrientations()
        scene.requestGeometryUpdate(.iOS(interfaceOrientations: mask))
    }
}

final class AppDelegate: NSObject, UIApplicationDelegate {
    func application(
        _ application: UIApplication,
        supportedInterfaceOrientationsFor window: UIWindow?
    ) -> UIInterfaceOrientationMask {
        OrientationLock.mask
    }
}

/// Holds the studio in landscape, the shape of the stream, and reports which landscape side is up.
struct StudioOrientation: UIViewRepresentable {
    var onChange: (UIInterfaceOrientation) -> Void

    func makeUIView(context: Context) -> StudioOrientationView {
        let view = StudioOrientationView()
        view.onChange = onChange
        return view
    }

    func updateUIView(_ view: StudioOrientationView, context: Context) {
        view.onChange = onChange
    }
}

final class StudioOrientationView: UIView {
    var onChange: ((UIInterfaceOrientation) -> Void)?
    private var observation: NSKeyValueObservation?
    private weak var lockedScene: UIWindowScene?

    override init(frame: CGRect) {
        super.init(frame: frame)
        isUserInteractionEnabled = false
        isAccessibilityElement = false
    }

    required init?(coder: NSCoder) {
        nil
    }

    override func didMoveToWindow() {
        super.didMoveToWindow()
        observation = nil
        if let scene = window?.windowScene {
            lockedScene = scene
            OrientationLock.set(.landscape, in: scene)
            observation = scene.observe(\.effectiveGeometry, options: [.initial, .new]) { [weak self] scene, _ in
                MainActor.assumeIsolated {
                    self?.onChange?(scene.effectiveGeometry.interfaceOrientation)
                }
            }
        } else if let scene = lockedScene {
            OrientationLock.set(.allButUpsideDown, in: scene)
            lockedScene = nil
        }
    }
}
