import SwiftUI

struct StudioSessionDock: View {
    @Environment(StudioSession.self) private var session
    var onStop: () -> Void
    var spreads: Bool

    var body: some View {
        HStack(spacing: 12) {
            if spreads {
                Spacer(minLength: 12)
            }
            HStack(spacing: 0) {
                moonButton
                NavigationLink(value: AppRoute.settings) {
                    StudioDockSymbol(systemImage: "gearshape")
                }
                .buttonStyle(StudioDockButtonStyle())
                .accessibilityLabel(Text(.openSettings))
            }
            .padding(.horizontal, 4)
            .glassEffect(.regular, in: .capsule)
            StudioGlassIcon(systemImage: "stop.fill", label: .stop, tint: .red, action: onStop)
        }
    }

    private var moonOn: Bool {
        session.settings.curtainMode != .off
    }

    private var moonButton: some View {
        StudioDockButton(
            systemImage: moonOn ? "moon.fill" : "moon",
            label: .curtain,
            isSelected: moonOn,
            action: session.engageCurtain
        )
        .opacity(moonOn ? 1 : 0.45)
        .accessibilityHint(Text(.curtainCoverHint))
    }
}
