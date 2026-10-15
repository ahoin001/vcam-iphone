import SwiftUI
import UIKit

struct ConnectionBanner: View {
    @Environment(StudioSession.self) private var session
    @State private var copiedPanel = false

    var body: some View {
        VStack(alignment: .leading, spacing: 2) {
            Text(session.statusMessage)
                .font(.subheadline.weight(.semibold))
                .lineLimit(2)
                .foregroundStyle(session.isThermalHot ? .yellow : .primary)
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
            Button(action: copyPanelAddress) {
                HStack(spacing: 4) {
                    Image(systemName: copiedPanel ? "checkmark" : "slider.horizontal.3")
                    Text(copiedPanel ? String(localized: .panelCopied) : String(localized: .panelControls))
                    Text(session.panelLine)
                        .lineLimit(1)
                        .minimumScaleFactor(0.7)
                }
                .font(.caption2.monospacedDigit())
                .foregroundStyle(.secondary)
            }
            .buttonStyle(.plain)
            .accessibilityLabel(Text(.shareDeskControls))
            .accessibilityHint(Text(.deskControlsCopyHint))
        }
        .padding(.horizontal, 14)
        .padding(.vertical, 8)
        .frame(maxWidth: .infinity, alignment: .leading)
        .glassEffect(.regular, in: .rect(cornerRadius: 22))
    }

    private func copyPanelAddress() {
        UIPasteboard.general.string = session.panelLine
        copiedPanel = true
        Task { @MainActor in
            try? await Task.sleep(for: .seconds(1.4))
            copiedPanel = false
        }
    }
}
