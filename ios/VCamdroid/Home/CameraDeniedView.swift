import SwiftUI

struct CameraDeniedView: View {
    @Environment(\.openURL) private var openURL

    var body: some View {
        VStack(spacing: 18) {
            Text(.permissionDenied)
                .foregroundStyle(StudioTheme.ink2)
                .multilineTextAlignment(.center)
            Button(.openSettings, systemImage: "gearshape", action: openSettings)
                .buttonStyle(PressableButtonStyle(prominent: true))
        }
        .padding(28)
        .frame(maxWidth: .infinity, maxHeight: .infinity)
        .background(StudioTheme.paper)
    }

    private func openSettings() {
        guard let url = URL(string: UIApplication.openSettingsURLString) else { return }
        openURL(url)
    }
}
