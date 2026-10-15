import AVFoundation
import SwiftUI

struct CameraPermissionView: View {
    @Binding var path: NavigationPath
    @Environment(StudioSession.self) private var session

    var body: some View {
        VStack {
            Text(.cameraNeededTitle)
                .font(.title2)
                .bold()
            Text(.cameraNeededBody)
                .foregroundStyle(.secondary)
            Button(String(localized: .continueAction), systemImage: "camera", action: request)
                .buttonStyle(PressableButtonStyle(prominent: true))
        }
        .padding()
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
