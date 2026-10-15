import SwiftUI

struct WhiteBalanceControl: View {
    @Environment(StudioSession.self) private var session
    @Environment(\.accessibilityReduceMotion) private var reduceMotion

    var body: some View {
        VStack(alignment: .leading, spacing: 14) {
            StudioReadout(title: .whiteBalance, value: readout)
            ScrollView(.horizontal) {
                HStack(spacing: 8) {
                    ForEach(WhiteBalancePreset.allCases) { preset in
                        let selected = selectedPreset == preset
                        Button {
                            session.selectWhiteBalance(preset)
                        } label: {
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
                        .accessibilityAddTraits(selected ? .isSelected : [])
                    }
                }
            }
            .scrollIndicators(.hidden)
        }
        .animation(reduceMotion ? .easeOut(duration: 0.2) : .spring(response: 0.28, dampingFraction: 1), value: selectedPreset)
        .sensoryFeedback(.selection, trigger: selectedPreset)
    }

    private var selectedPreset: WhiteBalancePreset? {
        guard session.settings.manualWhiteBalance else { return .auto }
        return WhiteBalancePreset.allCases.first { $0.kelvin == session.settings.kelvin }
    }

    private var readout: String {
        if let selectedPreset {
            return String(localized: title(selectedPreset))
        }
        return session.settings.kelvin.formatted(.number.precision(.fractionLength(0))) + "K"
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
