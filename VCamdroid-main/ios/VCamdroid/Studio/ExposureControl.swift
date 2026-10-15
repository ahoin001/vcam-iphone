import SwiftUI

struct ExposureControl: View {
    @Environment(StudioSession.self) private var session
    @Environment(\.accessibilityReduceMotion) private var reduceMotion

    var body: some View {
        VStack(alignment: .leading, spacing: 14) {
            StudioReadout(title: .exposure, value: exposureReadout)
            Picker(.exposure, selection: StudioBindings.setting(\.manualExposure, on: session)) {
                Text(.auto).tag(false)
                Text(.manual).tag(true)
            }
            .pickerStyle(.segmented)
            .onChange(of: session.settings.manualExposure) {
                session.applyControls()
            }
            if session.settings.manualExposure {
                ExposureValueSlider(title: .iso, readout: isoReadout, unit: isoUnit, watched: session.settings.iso)
                ExposureValueSlider(title: .shutter, readout: shutterReadout, unit: shutterUnit, watched: session.settings.shutterSeconds)
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
            "\(isoReadout) · \(shutterReadout)"
        } else {
            session.settings.exposureBias.formatted(
                .number.precision(.fractionLength(1)).sign(strategy: .always(includingZero: false))
            )
        }
    }

    private var isoReadout: String {
        Int(session.settings.iso.rounded()).formatted(.number.precision(.fractionLength(0)))
    }

    private var shutterReadout: String {
        let seconds = session.settings.shutterSeconds
        if seconds <= 0 {
            return "0"
        }
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
