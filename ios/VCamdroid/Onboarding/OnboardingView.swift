import AVFoundation
import SwiftUI
import UIKit

struct OnboardingView: View {
    @Environment(StudioSession.self) private var session
    @State private var page = 0
    @State private var copied = false

    private let panelURL = URL(string: "https://github.com/ahoin001/vcam-iphone/releases/latest")!

    var body: some View {
        VStack(spacing: 16) {
            TabView(selection: $page) {
                step(title: .onboardingWelcomeTitle, body: .onboardingWelcomeBody, systemImage: "video")
                    .tag(0)
                panelStep
                    .tag(1)
                step(title: .onboardingConnectTitle, body: .onboardingConnectBody, systemImage: "cable.connector")
                    .tag(2)
                permissionStep
                    .tag(3)
            }
            .tabViewStyle(.page(indexDisplayMode: .always))
            if page < 3 {
                Button(.nextAction, systemImage: "arrow.right", action: advance)
                    .buttonStyle(PressableButtonStyle(prominent: true))
            }
        }
        .padding()
        .background(Color.black.opacity(0.001))
    }

    private var panelStep: some View {
        VStack(spacing: 16) {
            step(title: .onboardingPanelTitle, body: .onboardingPanelBody, systemImage: "desktopcomputer")
            Text(panelURL.absoluteString)
                .font(.footnote.monospaced())
                .foregroundStyle(.secondary)
                .multilineTextAlignment(.center)
            HStack(spacing: 12) {
                Button(copied ? String(localized: .panelCopied) : String(localized: .copyLink), systemImage: "doc.on.doc", action: copyLink)
                    .buttonStyle(PressableButtonStyle())
                ShareLink(item: panelURL) {
                    Label(String(localized: .shareLink), systemImage: "square.and.arrow.up")
                }
                .buttonStyle(PressableButtonStyle())
            }
        }
    }

    private var permissionStep: some View {
        VStack(spacing: 16) {
            step(title: .onboardingPermissionTitle, body: .cameraNeededBody, systemImage: "camera")
            Text(.onboardingLocalNetwork)
                .foregroundStyle(.secondary)
                .multilineTextAlignment(.center)
            Button(.startCamera, systemImage: "video", action: finish)
                .buttonStyle(PressableButtonStyle(prominent: true))
        }
    }

    private func step(title: LocalizedStringResource, body: LocalizedStringResource, systemImage: String) -> some View {
        VStack(spacing: 12) {
            Image(systemName: systemImage)
                .font(.largeTitle)
                .foregroundStyle(Color.accentColor)
            Text(title)
                .font(.title2.bold())
                .multilineTextAlignment(.center)
            Text(body)
                .foregroundStyle(.secondary)
                .multilineTextAlignment(.center)
        }
        .padding(24)
        .frame(maxWidth: .infinity)
        .glassEffect(.regular, in: .rect(cornerRadius: 28))
    }

    private func advance() {
        page = min(page + 1, 3)
    }

    private func copyLink() {
        UIPasteboard.general.string = panelURL.absoluteString
        copied = true
    }

    private func finish() {
        Task {
            _ = await AVCaptureDevice.requestAccess(for: .video)
            session.markLocalNetworkExplained()
            session.settings.onboardingCompleted = true
        }
    }
}
