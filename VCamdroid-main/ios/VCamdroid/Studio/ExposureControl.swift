import SwiftUI

struct ExposureControl: View {
    @Environment(StudioSession.self) private var session
    @Environment(\.accessibilityReduceMotion) private var reduceMotion

    var body: some View {
        VStack(alignment: .leading, spacing: 14) {
            StudioReadout(title: .exposure, value: exposureReadout)
            Picker(String(localized: .exposure), selection: StudioBindings.setting(\.manualExposure, on: session)) {
                Text(.auto).tag(false)
                Text(.manual).tag(true)
            }
            .pickerStyle(.segmented)
            .onChange(of: session.settings.manualExposure) {
                session.applyControls()
            }
            if session.settings.manualExposure {
                valueSlider(
                    title: .iso,
                    readout: isoReadout,
                    unit: isoUnit,
                    watched: session.settings.iso
                )
                valueSlider(
                    title: .shutter,
                    readout: shutterReadout,
                    unit: shutterUnit,
                    watched: session.settings.shutterSeconds
                )
            } else {
                Slider(value: StudioBindings.setting(\.exposureBias, on: session), in: -3...3)
                    .tint(.yellow)
                    .onChange(of: session.settings.exposureBias) {
                        session.applyControls()
                    }
            }
        }
        .animation(reduceMotion ? .easeOut(duration: 0.2) : .spring(response: 0.32, dampingFraction: 1), value: session.settings.manualExposure)
    }

    private var exposureReadout: String {
        if session.settings.manualExposure {
            return "\(isoReadout) · \(shutterReadout)"
        }
        let bias = session.settings.exposureBias
        let digits = bias.formatted(.number.precision(.fractionLength(1)))
        if bias > 0.049 {
            return "+\(digits)"
        }
        return digits
    }

    private var isoReadout: String {
        Int(session.settings.iso.rounded()).formatted(.number.precision(.fractionLength(0)))
    }

    private var shutterReadout: String {
        let seconds = session.settings.shutterSeconds
        guard seconds > 0 else { return "0" }
        if seconds >= 1 {
            return seconds.formatted(.number.precision(.fractionLength(1))) + "s"
        }
        let denominator = max(Int((1.0 / seconds).rounded()), 1)
        return "1/\(denominator)"
    }

    private var isoUnit: Binding<Double> {
        Binding(
            get: { Self.unit(for: session.settings.iso, lower: 50, upper: 6400) },
            set: { session.settings.iso = Self.value(for: $0, lower: 50, upper: 6400) }
        )
    }

    private var shutterUnit: Binding<Double> {
        Binding(
            get: { Self.unit(for: session.settings.shutterSeconds, lower: 0.0001, upper: 0.5) },
            set: { session.settings.shutterSeconds = Self.value(for: $0, lower: 0.0001, upper: 0.5) }
        )
    }

    private func valueSlider(
        title: LocalizedStringResource,
        readout: String,
        unit: Binding<Double>,
        watched: Double
    ) -> some View {
        VStack(alignment: .leading, spacing: 6) {
            HStack {
                Text(title)
                Spacer(minLength: 8)
                Text(readout)
                    .font(.subheadline.weight(.semibold).monospacedDigit())
                    .foregroundStyle(.yellow)
                    .contentTransition(reduceMotion ? .identity : .numericText())
            }
            .font(.subheadline)
            Slider(value: unit, in: 0...1)
                .tint(.yellow)
                .accessibilityValue(Text(readout))
        }
        .onChange(of: watched) {
            session.applyControls()
        }
    }

    private static func unit(for value: Double, lower: Double, upper: Double) -> Double {
        let clamped = min(max(value, lower), upper)
        let logLower = log(lower)
        let logUpper = log(upper)
        return (log(clamped) - logLower) / (logUpper - logLower)
    }

    private static func value(for unit: Double, lower: Double, upper: Double) -> Double {
        let clamped = min(max(unit, 0), 1)
        let logLower = log(lower)
        let logUpper = log(upper)
        return exp(logLower + clamped * (logUpper - logLower))
    }
}
