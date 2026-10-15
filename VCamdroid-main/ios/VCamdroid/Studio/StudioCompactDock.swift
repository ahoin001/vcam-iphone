import SwiftUI

struct StudioCompactDock: View {
    @Binding var tool: StudioTool?
    var onStop: () -> Void

    var body: some View {
        ScrollView(.horizontal) {
            HStack(spacing: 12) {
                StudioToolDock(tool: $tool)
                StudioSessionDock(onStop: onStop, spreads: false)
            }
        }
        .scrollIndicators(.hidden)
        .fixedSize(horizontal: false, vertical: true)
    }
}
