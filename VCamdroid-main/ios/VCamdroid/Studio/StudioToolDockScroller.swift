import SwiftUI

struct StudioToolDockScroller: View {
    @Binding var tool: StudioTool?

    var body: some View {
        GeometryReader { proxy in
            ScrollView(.horizontal) {
                StudioToolDock(tool: $tool)
                    .frame(minWidth: proxy.size.width)
            }
            .scrollIndicators(.hidden)
            .scrollBounceBehavior(.basedOnSize, axes: .horizontal)
        }
        .frame(height: 52)
    }
}
