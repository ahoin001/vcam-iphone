import SwiftUI

struct PortraitPreviewNotice: View {
    @Environment(StudioSession.self) private var session

    var body: some View {
        ZStack {
            Color.black.opacity(0.78)
            VStack(spacing: 14) {
                Image(systemName: "desktopcomputer")
                    .font(.title2)
                    .foregroundStyle(.yellow)
                    .accessibilityHidden(true)
                Text(.portraitPreviewTitle)
                    .font(.title3.weight(.semibold))
                    .multilineTextAlignment(.center)
                Text(.portraitPreviewBody)
                    .font(.body)
                    .foregroundStyle(.secondary)
                    .multilineTextAlignment(.center)
                Button(.portraitPreviewShow, action: showOnPhone)
                    .buttonStyle(.borderedProminent)
                    .tint(.yellow)
                    .padding(.top, 4)
            }
            .padding(24)
            .frame(maxWidth: 340)
            .glassEffect(.regular, in: .rect(cornerRadius: 28))
            .padding(.horizontal, 28)
            .padding(.bottom, 200)
        }
    }

    private func showOnPhone() {
        session.settings.portraitPhonePreview = true
        session.applyPortrait()
    }
}
