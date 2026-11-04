import AVFoundation
import SwiftUI

struct HomeView: View {
    @Environment(StudioSession.self) private var session
    @Environment(\.scenePhase) private var scenePhase
    @State private var path = NavigationPath()

    var body: some View {
        NavigationStack(path: $path) {
            ZStack {
                StudioTheme.paper.ignoresSafeArea()
                Circle()
                    .fill(StudioTheme.prism.opacity(0.22))
                    .blur(radius: 80)
                    .frame(width: 280, height: 280)
                    .offset(x: 120, y: -220)
                    .allowsHitTesting(false)
                Circle()
                    .fill(StudioTheme.accent.opacity(0.16))
                    .blur(radius: 90)
                    .frame(width: 240, height: 240)
                    .offset(x: -140, y: 260)
                    .allowsHitTesting(false)
                VStack(spacing: 18) {
                    Image(systemName: "sparkle")
                        .font(.title2.weight(.semibold))
                        .foregroundStyle(StudioTheme.prismFill)
                    Text(.homeTitle)
                        .font(.system(size: 34, weight: .medium, design: .rounded))
                        .tracking(-0.6)
                    Text(.homeSubtitle)
                        .font(.body)
                        .foregroundStyle(StudioTheme.ink2)
                        .multilineTextAlignment(.center)
                    Button(.openStudio, systemImage: "video", action: openStudio)
                        .buttonStyle(PressableButtonStyle(prominent: true))
                    Button(.howToConnect, systemImage: "questionmark.circle", action: openHelp)
                        .buttonStyle(PressableButtonStyle())
                }
                .padding(28)
            }
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
