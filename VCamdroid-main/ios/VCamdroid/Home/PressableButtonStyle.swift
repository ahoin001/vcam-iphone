import SwiftUI

struct PressableButtonStyle: ButtonStyle {
    var prominent = false
    @Environment(\.accessibilityReduceMotion) private var reduceMotion

    func makeBody(configuration: Configuration) -> some View {
        configuration.label
            .font(.body.weight(.semibold))
            .padding(.horizontal, 18)
            .frame(minHeight: 48)
            .foregroundStyle(prominent ? StudioTheme.onInk : StudioTheme.ink)
            .background(prominent ? StudioTheme.primaryFill : StudioTheme.raised, in: .capsule)
            .scaleEffect(!reduceMotion && configuration.isPressed ? StudioTheme.pressScale : 1)
            .animation(StudioTheme.ease, value: configuration.isPressed)
    }
}
