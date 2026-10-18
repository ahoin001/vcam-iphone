import UIKit

@MainActor
final class ScreenCurtainController {
    func disableIdleTimer() {
        UIApplication.shared.isIdleTimerDisabled = true
    }

    func enableIdleTimer() {
        UIApplication.shared.isIdleTimerDisabled = false
    }
}
