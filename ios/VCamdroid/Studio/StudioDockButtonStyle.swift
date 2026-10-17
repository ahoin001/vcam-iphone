import SwiftUI

struct StudioDockButtonStyle: ButtonStyle {
    @Environment(\.accessibilityReduceMotion) private var reduceMotion

    func makeBody(configuration: Configuration) -> some View {
        configuration.label
            .scaleEffect(!reduceMotion && configuration.isPressed ? 0.96 : 1)
            .opacity(reduceMotion && configuration.isPressed ? 0.6 : 1)
            .animation(.timingCurve(0.23, 1, 0.32, 1, duration: 0.14), value: configuration.isPressed)
    }
}
