import SwiftUI

struct StudioDockButtonStyle: ButtonStyle {
    @Environment(\.accessibilityReduceMotion) private var reduceMotion

    func makeBody(configuration: Configuration) -> some View {
        configuration.label
            .scaleEffect(!reduceMotion && configuration.isPressed ? StudioTheme.pressScale : 1)
            .opacity(reduceMotion && configuration.isPressed ? 0.6 : 1)
            .animation(StudioTheme.ease, value: configuration.isPressed)
    }
}
