import SwiftUI

struct PortraitControl: View {
    @Environment(StudioSession.self) private var session
    @Environment(\.accessibilityReduceMotion) private var reduceMotion

    var body: some View {
        VStack(alignment: .leading, spacing: 14) {
            if session.settings.portraitEnabled {
                StudioReadout(title: .portrait, value: strengthReadout)
            } else {
                StudioReadout(title: .portrait, value: .portraitOff)
            }
            Toggle(.portrait, isOn: StudioBindings.setting(\.portraitEnabled, on: session))
                .tint(.yellow)
                .onChange(of: session.settings.portraitEnabled) {
                    session.applyPortrait()
                }
            Text(.portraitControlHint)
                .font(.caption)
                .foregroundStyle(.secondary)
                .fixedSize(horizontal: false, vertical: true)
            Text(.portraitSmoothnessHint)
                .font(.caption)
                .foregroundStyle(.secondary)
                .fixedSize(horizontal: false, vertical: true)
            if session.settings.portraitEnabled {
                Slider(value: StudioBindings.setting(\.portraitStrength, on: session), in: 0...1)
                    .tint(.yellow)
                    .onChange(of: session.settings.portraitStrength) {
                        session.applyPortrait()
                    }
                Toggle(.portraitPhonePreview, isOn: StudioBindings.setting(\.portraitPhonePreview, on: session))
                    .tint(.yellow)
                    .onChange(of: session.settings.portraitPhonePreview) {
                        session.applyPortrait()
                    }
                Text(.portraitPhonePreviewHint)
                    .font(.caption)
                    .foregroundStyle(.secondary)
                    .fixedSize(horizontal: false, vertical: true)
            }
        }
        .animation(reduceMotion ? .easeOut(duration: 0.2) : .spring(response: 0.32, dampingFraction: 1), value: session.settings.portraitEnabled)
    }

    private var strengthReadout: String {
        session.settings.portraitStrength.formatted(.percent.precision(.fractionLength(0)))
    }
}
