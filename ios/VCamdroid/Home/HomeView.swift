import AVFoundation
import SwiftUI

struct HomeView: View {
    @Environment(StudioSession.self) private var session
    @Environment(\.scenePhase) private var scenePhase
    @State private var path = NavigationPath()
    @State private var camera = AVCaptureDevice.authorizationStatus(for: .video)

    var body: some View {
        Group {
            if !session.settings.onboardingCompleted {
                OnboardingView()
            } else if camera == .authorized {
                NavigationStack(path: $path) {
                    StudioView()
                        .navigationDestination(for: AppRoute.self) { route in
                            RouteDestination(route: route, path: $path)
                        }
                }
            } else if camera == .notDetermined {
                VStack(spacing: 16) {
                    Text(.cameraNeededTitle)
                        .font(.title2.bold())
                    Text(.cameraNeededBody)
                        .foregroundStyle(.secondary)
                        .multilineTextAlignment(.center)
                    Button(.continueAction, systemImage: "camera", action: requestCamera)
                        .buttonStyle(PressableButtonStyle(prominent: true))
                }
                .padding()
            } else {
                CameraDeniedView()
            }
        }
        .onChange(of: scenePhase) { _, phase in
            session.updateScenePhase(phase)
            if phase == .active {
                camera = AVCaptureDevice.authorizationStatus(for: .video)
            }
        }
        .onChange(of: session.settings.onboardingCompleted) {
            camera = AVCaptureDevice.authorizationStatus(for: .video)
            if !session.settings.onboardingCompleted {
                path = NavigationPath()
            }
        }
    }

    private func requestCamera() {
        Task {
            _ = await AVCaptureDevice.requestAccess(for: .video)
            camera = AVCaptureDevice.authorizationStatus(for: .video)
        }
    }
}

#Preview {
    HomeView()
        .environment(StudioSession())
}
