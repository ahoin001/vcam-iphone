import SwiftUI
import UIKit

struct ConnectionBanner: View {
    @Environment(StudioSession.self) private var session
    @State private var copiedPanel = false
    @State private var showDeskLink = false
    @State private var holdLiveLine = false
    @State private var heldLive: LocalizedStringResource = .streaming
    @State private var liveHold: Task<Void, Never>?

    var body: some View {
        VStack(alignment: .leading, spacing: 2) {
            Text(headline)
                .font(.subheadline.weight(.semibold))
                .lineLimit(5)
                .fixedSize(horizontal: false, vertical: true)
                .foregroundStyle(session.isThermalHot || session.statusMessage == .pcCannotDecode ? .yellow : .primary)
            if waitingForPanel {
                Text(.usbOrWifiHint)
                    .font(.caption)
                    .foregroundStyle(.secondary)
                    .lineLimit(2)
            }
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
            Button(showDeskLink ? String(localized: .hideDeskLink) : String(localized: .panelControls), action: toggleDeskLink)
                .font(.caption2)
                .buttonStyle(.plain)
                .foregroundStyle(.secondary)
            if showDeskLink {
                Button(action: copyPanelAddress) {
                    HStack(spacing: 4) {
                        Image(systemName: copiedPanel ? "checkmark" : "doc.on.doc")
                        Text(copiedPanel ? String(localized: .panelCopied) : session.panelLine)
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
        }
        .padding(.horizontal, 14)
        .padding(.vertical, 8)
        .frame(maxWidth: .infinity, alignment: .leading)
        .glassEffect(.regular, in: .rect(cornerRadius: 22))
        .onChange(of: session.snapshot.isStreaming) { _, streaming in
            noteStreaming(streaming)
        }
    }

    private var waitingForPanel: Bool {
        session.isRunning && !session.snapshot.isStreaming && !holdLiveLine
    }

    private var headline: LocalizedStringResource {
        if session.stoppedByUser && !session.isRunning {
            return .cameraStopped
        }
        if session.statusMessage == .pcCannotDecode {
            return .pcCannotDecode
        }
        if holdLiveLine && session.isRunning && !session.snapshot.isStreaming {
            return heldLive
        }
        if session.isRunning && !session.snapshot.isStreaming && !session.isThermalHot {
            return .openPanelNextStep
        }
        return session.statusMessage
    }

    private func noteStreaming(_ streaming: Bool) {
        liveHold?.cancel()
        if streaming {
            holdLiveLine = false
            heldLive = session.statusMessage == .cameraLive ? .cameraLive : .streaming
            return
        }
        guard session.isRunning, !session.stoppedByUser else {
            holdLiveLine = false
            return
        }
        holdLiveLine = true
        liveHold = Task { @MainActor in
            try? await Task.sleep(for: .seconds(2))
            guard !Task.isCancelled else { return }
            holdLiveLine = false
        }
    }

    private func toggleDeskLink() {
        showDeskLink.toggle()
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
