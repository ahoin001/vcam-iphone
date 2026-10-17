import SwiftUI

struct CameraDeniedView: View {
    @Environment(\.openURL) private var openURL

    var body: some View {
        VStack {
            Text(.permissionDenied)
                .foregroundStyle(.secondary)
            Button(.openSettings, systemImage: "gearshape", action: openSettings)
                .buttonStyle(PressableButtonStyle(prominent: true))
        }
        .padding()
    }

    private func openSettings() {
        guard let url = URL(string: UIApplication.openSettingsURLString) else { return }
        openURL(url)
    }
}
