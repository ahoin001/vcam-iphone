import SwiftUI

struct PreviewStage: View {
    var onPreviewTap: () -> Void = {}
    @Environment(StudioSession.self) private var session

    var body: some View {
        CameraPreview(session: session.previewSession.session, rotation: session.settings.flip180 ? 180 : 0) { point in
            onPreviewTap()
            session.focus(at: point)
        }
        .accessibilityLabel(Text(.previewMatches))
        .background(.black)
    }
}
