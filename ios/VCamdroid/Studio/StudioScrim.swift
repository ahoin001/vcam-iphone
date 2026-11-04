import SwiftUI

struct StudioScrim: View {
    var body: some View {
        VStack(spacing: 0) {
            LinearGradient(
                colors: [.black.opacity(0.62), .black.opacity(0)],
                startPoint: .top,
                endPoint: .bottom
            )
            .frame(height: 148)
            Spacer(minLength: 0)
            LinearGradient(
                colors: [.black.opacity(0), .black.opacity(0.72)],
                startPoint: .top,
                endPoint: .bottom
            )
            .frame(height: 260)
        }
    }
}
