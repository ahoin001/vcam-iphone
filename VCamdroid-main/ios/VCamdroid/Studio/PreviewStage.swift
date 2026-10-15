import SwiftUI

struct PreviewStage: View {
    @Environment(StudioSession.self) private var session
    @Environment(\.accessibilityReduceMotion) private var reduceMotion

    var body: some View {
        ZStack {
            CameraPreview(session: session.previewSession.session, rotation: session.settings.flip180 ? 180 : 0) { point in
                session.focus(at: point)
            }
            .accessibilityLabel(Text(.previewMatches))
            CurtainOverlay()
                .opacity(session.curtainPresented ? 1 : 0)
                .scaleEffect(reduceMotion ? 1 : (session.curtainPresented ? 1 : 0.98))
                .animation(
                    reduceMotion ? .easeOut(duration: 0.2) : .timingCurve(0.23, 1, 0.32, 1, duration: 0.22),
                    value: session.curtainPresented
                )
                .allowsHitTesting(session.curtainPresented)
        }
        .background(.black)
        .clipShape(.rect(cornerRadius: 16))
    }
}
