import SwiftUI

struct WhiteBalanceControl: View {
    @Environment(StudioSession.self) private var session
    @Environment(\.accessibilityReduceMotion) private var reduceMotion

    var body: some View {
        VStack(alignment: .leading, spacing: 14) {
            StudioReadout(title: .whiteBalance, value: readout)
            if !session.customWhiteBalanceSupported {
                Text(.whiteBalanceUnavailable)
                    .font(.caption)
                    .foregroundStyle(.secondary)
                    .fixedSize(horizontal: false, vertical: true)
            }
            ScrollView(.horizontal) {
                HStack(spacing: 8) {
                    ForEach(WhiteBalancePreset.allCases) { preset in
                        let selected = selectedPreset == preset
                        let locked = !session.customWhiteBalanceSupported && preset != .auto
                        Button(action: { select(preset) }) {
                            Text(title(preset))
                                .font(.subheadline.weight(selected ? .semibold : .regular))
                                .lineLimit(1)
                                .padding(.horizontal, 14)
                                .frame(minHeight: 44)
                        }
                        .buttonStyle(.plain)
                        .foregroundStyle(selected ? Color.black : Color.primary)
                        .background(selected ? Color.yellow : Color.white.opacity(0.14), in: .capsule)
                        .overlay {
                            Capsule().strokeBorder(.white.opacity(selected ? 0 : 0.18), lineWidth: 1)
                        }
                        .disabled(locked)
                        .opacity(locked ? 0.4 : 1)
                        .accessibilityAddTraits(selected ? .isSelected : [])
                    }
                }
            }
            .scrollIndicators(.hidden)
            if session.customWhiteBalanceSupported {
                if session.settings.manualWhiteBalance {
                    VStack(alignment: .leading, spacing: 6) {
                        HStack {
                            Text(.kelvin)
                            Spacer(minLength: 8)
                            Text(kelvinReadout)
                                .font(.subheadline.weight(.semibold).monospacedDigit())
                                .foregroundStyle(.yellow)
                        }
                        .font(.subheadline)
                        Slider(value: StudioBindings.setting(\.kelvin, on: session), in: 2500...8000)
                            .tint(.yellow)
                            .onChange(of: session.settings.kelvin) {
                                session.applyControls()
                            }
                    }
                    VStack(alignment: .leading, spacing: 6) {
                        HStack {
                            Text(.tint)
                            Spacer(minLength: 8)
                            Text(tintReadout)
                                .font(.subheadline.weight(.semibold).monospacedDigit())
                                .foregroundStyle(.yellow)
                        }
                        .font(.subheadline)
                        Slider(value: StudioBindings.setting(\.tint, on: session), in: -150...150)
                            .tint(.yellow)
                            .onChange(of: session.settings.tint) {
                                session.applyControls()
                            }
                    }
                }
                Button(.lockWhiteBalance) {
                    Task { await session.lockCurrentWhiteBalance() }
                }
                    .buttonStyle(.plain)
                    .font(.subheadline.weight(.semibold))
                    .foregroundStyle(.yellow)
                    .frame(minHeight: 44)
            }
        }
        .animation(reduceMotion ? .easeOut(duration: 0.2) : .spring(response: 0.28, dampingFraction: 1), value: selectedPreset)
        .sensoryFeedback(.selection, trigger: selectedPreset)
    }

    private var selectedPreset: WhiteBalancePreset? {
        guard session.settings.manualWhiteBalance else { return .auto }
        guard abs(session.settings.tint) < 0.5 else { return nil }
        return WhiteBalancePreset.allCases.first { $0.kelvin == session.settings.kelvin }
    }

    private var readout: String {
        if let selectedPreset {
            String(localized: title(selectedPreset))
        } else {
            kelvinReadout
        }
    }

    private var kelvinReadout: String {
        session.settings.kelvin.formatted(.number.precision(.fractionLength(0))) + "K"
    }

    private var tintReadout: String {
        session.settings.tint.formatted(.number.precision(.fractionLength(0)))
    }

    private func select(_ preset: WhiteBalancePreset) {
        session.selectWhiteBalance(preset)
    }

    private func title(_ preset: WhiteBalancePreset) -> LocalizedStringResource {
        switch preset {
        case .auto: .auto
        case .daylight: .daylight
        case .cloudy: .cloudy
        case .fluorescent: .fluorescent
        case .incandescent: .incandescent
        }
    }
}
