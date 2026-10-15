import SwiftUI

struct LocalNetworkPermissionView: View {
    @Binding var path: NavigationPath
    @Environment(StudioSession.self) private var session

    var body: some View {
        VStack {
            Text(.localNetworkTitle)
                .font(.title2)
                .bold()
            Text(.localNetworkBody)
                .foregroundStyle(.secondary)
            Button(String(localized: .continueAction), systemImage: "wifi", action: allow)
                .buttonStyle(PressableButtonStyle(prominent: true))
            Button(String(localized: .notNow), systemImage: "cable.connector", action: skip)
                .buttonStyle(PressableButtonStyle())
        }
        .padding()
    }

    private func allow() {
        session.markLocalNetworkExplained()
        path.append(AppRoute.studio)
    }

    private func skip() {
        session.markLocalNetworkExplained()
        path.append(AppRoute.studio)
    }
}
