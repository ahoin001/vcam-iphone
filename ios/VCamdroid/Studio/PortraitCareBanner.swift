import SwiftUI

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
        case .eased, .paused, .streamHot, .lowBattery, .heavierAndBattery:
            .yellow
        default:
            .primary
        }
    }
}
