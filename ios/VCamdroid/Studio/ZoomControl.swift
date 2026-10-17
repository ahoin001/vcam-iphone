import SwiftUI

struct ZoomControl: View {
    @Environment(StudioSession.self) private var session

    private let stops: [Double] = [1, 2, 3, 5, 10]

    var body: some View {
        VStack(alignment: .leading, spacing: 14) {
            StudioReadout(title: .zoom, value: zoomReadout)
            HStack(spacing: 8) {
                ForEach(stops, id: \.self) { stop in
                    Button {
                        session.settings.zoom = stop
                    } label: {
                        Text(stopLabel(stop))
                            .font(.subheadline.weight(isSelected(stop) ? .semibold : .regular).monospacedDigit())
                            .frame(maxWidth: .infinity, minHeight: 36)
                    }
                    .buttonStyle(.plain)
                    .foregroundStyle(isSelected(stop) ? Color.black : Color.primary)
                    .background(isSelected(stop) ? Color.yellow : Color.white.opacity(0.14), in: .capsule)
                    .accessibilityAddTraits(isSelected(stop) ? .isSelected : [])
                }
            }
            Slider(value: StudioBindings.setting(\.zoom, on: session), in: 1...10)
                .tint(.yellow)
        }
        .onChange(of: session.settings.zoom) {
            session.applyControls()
        }
    }

    private var zoomReadout: String {
        session.settings.zoom.formatted(.number.precision(.fractionLength(1))) + "×"
    }

    private func stopLabel(_ stop: Double) -> String {
        stop.formatted(.number.precision(.fractionLength(0))) + "×"
    }

    private func isSelected(_ stop: Double) -> Bool {
        abs(session.settings.zoom - stop) < 0.05
    }
}
