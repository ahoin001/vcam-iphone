import SwiftUI

struct ConnectionBanner: View {
    @Environment(StudioSession.self) private var session

    var body: some View {
        VStack(alignment: .leading, spacing: 2) {
            Text(session.statusMessage)
                .font(.subheadline.weight(.semibold))
                .lineLimit(2)
            HStack(spacing: 6) {
                Text(session.addressLine)
                    .textSelection(.enabled)
                if session.snapshot.isStreaming {
                    Text("·")
                        .accessibilityHidden(true)
                    if session.isUSBClient {
                        Text(.usbBadge)
                    } else {
                        Text(.wifiBadge)
                    }
                }
            }
            .font(.caption.monospacedDigit())
            .foregroundStyle(.secondary)
            .lineLimit(1)
            .minimumScaleFactor(0.8)
        }
        .padding(.horizontal, 14)
        .padding(.vertical, 8)
        .frame(maxWidth: .infinity, alignment: .leading)
        .glassEffect(.regular, in: .capsule)
    }
}
