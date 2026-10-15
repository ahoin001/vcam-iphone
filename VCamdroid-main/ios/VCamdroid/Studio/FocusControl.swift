import SwiftUI

struct FocusControl: View {
    @Environment(StudioSession.self) private var session
    @Environment(\.accessibilityReduceMotion) private var reduceMotion

    var body: some View {
        VStack(alignment: .leading, spacing: 14) {
            if session.settings.manualFocus {
                StudioReadout(
                    title: .focus,
                    value: session.settings.lensPosition.formatted(.number.precision(.fractionLength(2)))
                )
            } else {
                StudioReadout(title: .focus, value: .auto)
            }
            Picker(.focus, selection: StudioBindings.setting(\.manualFocus, on: session)) {
                Text(.auto).tag(false)
                Text(.manual).tag(true)
            }
            .pickerStyle(.segmented)
            .onChange(of: session.settings.manualFocus) {
                session.applyControls()
            }
            if session.settings.manualFocus {
                Slider(value: StudioBindings.setting(\.lensPosition, on: session), in: 0...1)
                    .tint(.yellow)
                    .onChange(of: session.settings.lensPosition) {
                        session.applyControls()
                    }
                HStack {
                    Text(.focusNear)
                    Spacer()
                    Text(.focusFar)
                }
                .font(.caption)
                .foregroundStyle(.secondary)
            }
        }
        .animation(reduceMotion ? .easeOut(duration: 0.2) : .spring(response: 0.32, dampingFraction: 1), value: session.settings.manualFocus)
    }
}
