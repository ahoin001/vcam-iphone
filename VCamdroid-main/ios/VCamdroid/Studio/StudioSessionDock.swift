import SwiftUI

struct StudioSessionDock: View {
    @Environment(StudioSession.self) private var session
    var onStop: () -> Void
    var spreads: Bool

    var body: some View {
        HStack(spacing: 12) {
            HStack(spacing: 0) {
                StudioDockButton(systemImage: "moon.fill", label: .curtain, action: session.engageCurtain)
                NavigationLink(value: AppRoute.settings) {
                    StudioDockSymbol(systemImage: "gearshape")
                }
                .buttonStyle(StudioDockButtonStyle())
                .accessibilityLabel(Text(.openSettings))
                NavigationLink(value: AppRoute.log) {
                    StudioDockSymbol(systemImage: "list.bullet")
                }
                .buttonStyle(StudioDockButtonStyle())
                .accessibilityLabel(Text(.openLog))
            }
            .padding(.horizontal, 4)
            .glassEffect(.regular, in: .capsule)
            if spreads {
                Spacer(minLength: 12)
            }
            StudioGlassIcon(systemImage: "stop.fill", label: .stop, tint: .red, action: onStop)
        }
    }
}
