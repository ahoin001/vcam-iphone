import SwiftUI

struct RouteDestination: View {
    let route: AppRoute
    @Binding var path: NavigationPath

    var body: some View {
        switch route {
        case .howToConnect:
            HowToConnectView()
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
