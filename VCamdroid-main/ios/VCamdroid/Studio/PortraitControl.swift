import SwiftUI

struct PortraitControl: View {
    @Environment(StudioSession.self) private var session
    @Environment(\.accessibilityReduceMotion) private var reduceMotion

    var body: some View {
        VStack(alignment: .leading, spacing: 14) {
            StudioReadout(title: .portrait, value: readout)
            Toggle(String(localized: .portrait), isOn: StudioBindings.setting(\.portraitEnabled, on: session))
                .tint(.yellow)
                .onChange(of: session.settings.portraitEnabled) {
                    session.applyPortrait()
                }
            if session.settings.portraitEnabled {
                Slider(value: StudioBindings.setting(\.portraitStrength, on: session), in: 0...1)
                    .tint(.yellow)
                    .onChange(of: session.settings.portraitStrength) {
                        session.applyPortrait()
                    }
                Text(.portraitControlHint)
                    .font(.caption)
                    .foregroundStyle(.secondary)
                    .fixedSize(horizontal: false, vertical: true)
            }
        }
        .animation(reduceMotion ? .easeOut(duration: 0.2) : .spring(response: 0.32, dampingFraction: 1), value: session.settings.portraitEnabled)
    }

    private var readout: String {
        guard session.settings.portraitEnabled else { return String(localized: .portraitOff) }
        return session.settings.portraitStrength.formatted(.percent.precision(.fractionLength(0)))
    }
}

struct PortraitCareBanner: View {
    @Environment(StudioSession.self) private var session

    var body: some View {
        if let message = session.portraitCare.message {
            Text(message)
                .font(.caption.weight(.semibold))
                .foregroundStyle(tone)
                .fixedSize(horizontal: false, vertical: true)
                .padding(.horizontal, 14)
                .padding(.vertical, 8)
                .frame(maxWidth: .infinity, alignment: .leading)
                .glassEffect(.regular, in: .rect(cornerRadius: 18))
                .accessibilityAddTraits(.updatesFrequently)
        }
    }

    private var tone: Color {
        switch session.portraitCare {
        case .eased, .paused, .lowBattery, .heavierAndBattery:
            .yellow
        default:
            .primary
        }
    }
}
