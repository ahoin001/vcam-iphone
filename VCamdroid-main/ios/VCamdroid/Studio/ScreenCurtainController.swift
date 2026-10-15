import UIKit

@MainActor
final class ScreenCurtainController {
    private var savedBrightness: CGFloat?

    func disableIdleTimer() {
        UIApplication.shared.isIdleTimerDisabled = true
    }

    func enableIdleTimer() {
        UIApplication.shared.isIdleTimerDisabled = false
    }

    func dim() {
        if savedBrightness == nil {
            savedBrightness = UIScreen.main.brightness
        }
        UIScreen.main.brightness = 0
    }

    func restore() {
        if let savedBrightness {
            UIScreen.main.brightness = savedBrightness
        }
        savedBrightness = nil
    }
}
