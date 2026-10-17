import SwiftUI

struct PreviewStage: View {
    var onPreviewTap: () -> Void = {}
    @Environment(StudioSession.self) private var session

    var body: some View {
        CameraPreview(
            session: session.previewSession.session,
            rotation: session.streamRotation,
            showsProcessed: session.portraitPreviewActive,
            sink: session.portraitPreview
        ) { point in
            onPreviewTap()
            session.focus(at: point)
        }
        .accessibilityLabel(Text(.previewMatches))
        .accessibilityHidden(session.portraitPreviewDeferred)
        .background(.black)
        .overlay {
            if session.portraitPreviewDeferred {
                PortraitPreviewNotice()
                    .transition(.opacity)
            }
        }
    }
}
