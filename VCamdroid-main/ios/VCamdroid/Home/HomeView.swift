import AVFoundation
import SwiftUI

struct HomeView: View {
    @Environment(StudioSession.self) private var session
    @Environment(\.scenePhase) private var scenePhase
    @State private var path = NavigationPath()

    var body: some View {
        NavigationStack(path: $path) {
            VStack {
                Text(.homeTitle)
                    .font(.largeTitle)
                    .bold()
                Text(.homeSubtitle)
                    .foregroundStyle(.secondary)
                Button(String(localized: .obsMode), systemImage: "video", action: openStudio)
                    .buttonStyle(PressableButtonStyle(prominent: true))
                Button(String(localized: .howToConnect), systemImage: "questionmark.circle", action: openHelp)
                    .buttonStyle(PressableButtonStyle())
            }
            .padding()
            .frame(maxWidth: .infinity, maxHeight: .infinity)
            .navigationDestination(for: AppRoute.self) { route in
                RouteDestination(route: route, path: $path)
            }
        }
        .onChange(of: scenePhase) { _, phase in
            session.updateScenePhase(phase)
        }
    }

    private func openStudio() {
        switch AVCaptureDevice.authorizationStatus(for: .video) {
        case .authorized:
            path.append(session.settings.explainedLocalNetwork ? AppRoute.studio : AppRoute.localNetwork)
        case .notDetermined:
            path.append(AppRoute.cameraPermission)
        default:
            path.append(AppRoute.cameraDenied)
        }
    }

    private func openHelp() {
        path.append(AppRoute.howToConnect)
    }
}

#Preview {
    HomeView()
        .environment(StudioSession())
}
