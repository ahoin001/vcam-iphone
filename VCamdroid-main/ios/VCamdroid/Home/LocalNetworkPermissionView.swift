import SwiftUI

struct LocalNetworkPermissionView: View {
    @Binding var path: NavigationPath
    @Environment(StudioSession.self) private var session

    var body: some View {
        VStack(spacing: 18) {
            Text(.localNetworkTitle)
                .font(.system(size: 28, weight: .medium, design: .rounded))
                .tracking(-0.4)
            Text(.localNetworkBody)
                .foregroundStyle(StudioTheme.ink2)
                .multilineTextAlignment(.center)
            Button(.continueAction, systemImage: "wifi", action: allow)
                .buttonStyle(PressableButtonStyle(prominent: true))
            Button(.notNow, systemImage: "cable.connector", action: skip)
                .buttonStyle(PressableButtonStyle())
        }
        .padding(28)
        .frame(maxWidth: .infinity, maxHeight: .infinity)
        .background(StudioTheme.paper)
    }

    private func allow() {
        session.settings.discoveryEnabled = true
        session.markLocalNetworkExplained()
        path.append(AppRoute.studio)
    }

    private func skip() {
        session.settings.discoveryEnabled = false
        session.markLocalNetworkExplained()
        path.append(AppRoute.studio)
    }
}
