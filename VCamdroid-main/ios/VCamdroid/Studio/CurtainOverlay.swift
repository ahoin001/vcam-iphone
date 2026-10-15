import SwiftUI

struct CurtainOverlay: View {
    @Environment(StudioSession.self) private var session

    var body: some View {
        Button(action: session.dismissCurtain) {
            VStack {
                Spacer()
                Text(.showControls)
                    .font(.subheadline.weight(.medium))
                    .foregroundStyle(.white.opacity(0.45))
                    .padding(.bottom, 28)
            }
            .frame(maxWidth: .infinity, maxHeight: .infinity)
            .background(.black)
        }
        .buttonStyle(.plain)
        .accessibilityLabel(Text(.showControls))
    }
}