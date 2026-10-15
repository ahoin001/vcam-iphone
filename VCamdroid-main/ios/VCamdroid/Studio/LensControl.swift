import SwiftUI

struct LensControl: View {
    @Environment(StudioSession.self) private var session

    var body: some View {
        HStack {
            Button(String(localized: .backLens), systemImage: "camera", action: { session.selectLens(.back) })
                .buttonStyle(PressableButtonStyle(prominent: session.settings.lens == .back))
            Button(String(localized: .frontLens), systemImage: "person.crop.square", action: { session.selectLens(.front) })
                .buttonStyle(PressableButtonStyle(prominent: session.settings.lens == .front))
            Button(String(localized: .torch), systemImage: "flashlight.on.fill", action: session.toggleTorch)
                .buttonStyle(PressableButtonStyle(prominent: session.settings.torch))
            Button(String(localized: .flip), systemImage: "arrow.triangle.2.circlepath", action: session.toggleFlip)
                .buttonStyle(PressableButtonStyle(prominent: session.settings.flip180))
        }
    }
}
