import AVFoundation
import SwiftUI

struct CameraPermissionView: View {
    @Binding var path: NavigationPath
    @Environment(StudioSession.self) private var session

    var body: some View {
        VStack(spacing: 18) {
            Text(.cameraNeededTitle)
                .font(.system(size: 28, weight: .medium, design: .rounded))
                .tracking(-0.4)
            Text(.cameraNeededBody)
                .foregroundStyle(StudioTheme.ink2)
                .multilineTextAlignment(.center)
            Button(.continueAction, systemImage: "camera", action: request)
                .buttonStyle(PressableButtonStyle(prominent: true))
        }
        .padding(28)
        .frame(maxWidth: .infinity, maxHeight: .infinity)
        .background(StudioTheme.paper)
    }

    private func request() {
        Task {
            let granted = await AVCaptureDevice.requestAccess(for: .video)
            path.append(granted ? nextRoute : .cameraDenied)
        }
    }

    private var nextRoute: AppRoute {
        session.settings.explainedLocalNetwork ? .studio : .localNetwork
    }
}
