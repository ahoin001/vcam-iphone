import SwiftUI

struct CurtainOverlay: View {
    @Environment(StudioSession.self) private var session

    var body: some View {
        Button(String(localized: .showControls), action: session.dismissCurtain)
            .buttonStyle(.plain)
            .foregroundStyle(.black)
            .frame(maxWidth: .infinity, maxHeight: .infinity)
            .background(.black)
    }
}
