import SwiftUI

struct ExposureValueSlider: View {
    @Environment(StudioSession.self) private var session
    @Environment(\.accessibilityReduceMotion) private var reduceMotion
    let title: LocalizedStringResource
    let readout: String
    let unit: Binding<Double>
    let watched: Double

    var body: some View {
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
}
