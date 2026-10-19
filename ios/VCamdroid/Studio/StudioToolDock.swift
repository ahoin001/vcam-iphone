import SwiftUI

struct StudioToolDock: View {
    @Environment(StudioSession.self) private var session
    @Binding var tool: StudioTool?

    var body: some View {
        HStack(spacing: 0) {
            ForEach(StudioTool.allCases) { item in
                StudioDockButton(
                    systemImage: item.systemImage,
                    label: item.label,
                    isSelected: tool == item || (item == .portrait && session.settings.portraitEnabled),
                    action: { toggle(item) }
                )
            }
            StudioDockButton(
                systemImage: "arrow.triangle.2.circlepath.camera",
                label: .switchCamera,
                action: switchLens
            )
            .accessibilityValue(Text(session.settings.lens == .back ? .backLens : .frontLens))
            StudioDockButton(
                systemImage: session.settings.torch ? "flashlight.on.fill" : "flashlight.off.fill",
                label: .torch,
                isSelected: session.settings.torch,
                action: session.toggleTorch
            )
            .disabled(!session.torchSupported)
            .opacity(session.torchSupported ? 1 : 0.4)
            StudioDockButton(
                systemImage: "rotate.right",
                label: .flip,
                isSelected: session.settings.flip180,
                action: session.toggleFlip
            )
        }
        .padding(.horizontal, 4)
        .glassEffect(.regular, in: .capsule)
        .sensoryFeedback(.selection, trigger: session.settings.lens)
        .sensoryFeedback(.impact(weight: .light, intensity: 0.8), trigger: session.settings.torch)
        .sensoryFeedback(.impact(weight: .light, intensity: 0.8), trigger: session.settings.flip180)
        .sensoryFeedback(.impact(weight: .light, intensity: 0.8), trigger: session.settings.portraitEnabled)
    }

    private func toggle(_ item: StudioTool) {
        tool = tool == item ? nil : item
    }

    private func switchLens() {
        session.selectLens(session.settings.lens == .back ? .front : .back)
    }
}
