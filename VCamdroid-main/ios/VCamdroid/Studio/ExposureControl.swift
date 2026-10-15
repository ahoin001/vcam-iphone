import SwiftUI

struct ExposureControl: View {
    @Environment(StudioSession.self) private var session

    var body: some View {
        VStack(alignment: .leading) {
            Toggle(String(localized: .manual), isOn: StudioBindings.setting(\.manualExposure, on: session))
                .onChange(of: session.settings.manualExposure) {
                    session.applyControls()
                }
            if session.settings.manualExposure {
                LabeledContent {
                    Text(session.settings.iso, format: .number.precision(.fractionLength(0)))
                } label: {
                    Text(.iso)
                }
                Slider(value: StudioBindings.setting(\.iso, on: session), in: 50...6400)
                    .onChange(of: session.settings.iso) {
                        session.applyControls()
                    }
                LabeledContent {
                    Text(session.settings.shutterSeconds, format: .number.precision(.fractionLength(4)))
                } label: {
                    Text(.shutter)
                }
                Slider(value: StudioBindings.setting(\.shutterSeconds, on: session), in: 0.0001...0.5)
                    .onChange(of: session.settings.shutterSeconds) {
                        session.applyControls()
                    }
            } else {
                LabeledContent {
                    Text(session.settings.exposureBias, format: .number.precision(.fractionLength(1)))
                } label: {
                    Text(.exposure)
                }
                Slider(value: StudioBindings.setting(\.exposureBias, on: session), in: -3...3)
                    .onChange(of: session.settings.exposureBias) {
                        session.applyControls()
                    }
            }
        }
    }
}
