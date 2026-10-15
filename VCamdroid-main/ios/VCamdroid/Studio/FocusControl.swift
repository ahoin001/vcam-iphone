import SwiftUI

struct FocusControl: View {
    @Environment(StudioSession.self) private var session

    var body: some View {
        VStack(alignment: .leading) {
            Toggle(String(localized: .manual), isOn: StudioBindings.setting(\.manualFocus, on: session))
                .onChange(of: session.settings.manualFocus) {
                    session.applyControls()
                }
            if session.settings.manualFocus {
                LabeledContent {
                    Text(session.settings.lensPosition, format: .number.precision(.fractionLength(2)))
                } label: {
                    Text(.focus)
                }
                Slider(value: StudioBindings.setting(\.lensPosition, on: session), in: 0...1)
                    .onChange(of: session.settings.lensPosition) {
                        session.applyControls()
                    }
            }
        }
    }
}
