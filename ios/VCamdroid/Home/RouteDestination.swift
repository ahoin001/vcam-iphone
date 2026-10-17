import SwiftUI

struct RouteDestination: View {
    let route: AppRoute
    @Binding var path: NavigationPath
    @Environment(StudioSession.self) private var session

    var body: some View {
        switch route {
        case .howToConnect:
            HowToConnectView { session.replaySetup() }
        case .cameraPermission:
            CameraPermissionView(path: $path)
        case .cameraDenied:
            CameraDeniedView()
        case .localNetwork:
            LocalNetworkPermissionView(path: $path)
        case .studio:
            StudioView()
        case .settings:
            SettingsView()
        case .log:
            LogView()
        }
    }
}
