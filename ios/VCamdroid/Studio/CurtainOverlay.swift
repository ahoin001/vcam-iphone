import SwiftUI

struct CurtainOverlay: View {
    @Environment(StudioSession.self) private var session
    @Environment(\.accessibilityVoiceOverEnabled) private var voiceOver
    @State private var hintVisible = true

    var body: some View {
        Button(action: session.dismissCurtain) {
            Color.black
                .overlay(alignment: .bottom) {
                    Text(.showControls)
                        .font(.subheadline.weight(.medium))
                        .foregroundStyle(.white.opacity(0.45))
                        .padding(.bottom, 28)
                        .opacity(hintVisible ? 1 : 0)
                }
        }
        .buttonStyle(.plain)
        .accessibilityLabel(Text(.showControls))
        .task {
            guard !voiceOver else { return }
            try? await Task.sleep(for: .seconds(1.6))
            guard !Task.isCancelled else { return }
            withAnimation(.easeOut(duration: 0.35)) {
                hintVisible = false
            }
        }
    }
}
