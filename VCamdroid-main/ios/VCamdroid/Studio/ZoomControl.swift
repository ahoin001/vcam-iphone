import SwiftUI

struct ZoomControl: View {
    @Environment(StudioSession.self) private var session

    var body: some View {
        VStack(alignment: .leading) {
            LabeledContent {
                Text(session.settings.zoom, format: .number.precision(.fractionLength(1)))
            } label: {
                Text(.zoom)
            }
            Slider(value: StudioBindings.setting(\.zoom, on: session), in: 1...10)
                .onChange(of: session.settings.zoom) {
                    session.applyControls()
                }
        }
    }
}
