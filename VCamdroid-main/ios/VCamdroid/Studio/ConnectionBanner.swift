import SwiftUI

struct ConnectionBanner: View {
    @Environment(StudioSession.self) private var session

    var body: some View {
        VStack(alignment: .leading) {
            Text(session.statusMessage)
            Text(session.addressLine)
                .font(.caption)
                .foregroundStyle(.secondary)
            Text(session.isUSBClient ? String(localized: .usbBadge) : String(localized: .wifiBadge))
                .font(.caption)
                .bold()
        }
        .accessibilityElement(children: .combine)
    }
}
