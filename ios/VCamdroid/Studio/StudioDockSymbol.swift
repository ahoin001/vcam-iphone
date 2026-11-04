import SwiftUI

struct StudioDockSymbol: View {
    let systemImage: String

    var body: some View {
        Image(systemName: systemImage)
            .font(.title3.weight(.semibold))
            .foregroundStyle(.primary)
            .frame(width: 44, height: 44)
            .contentShape(.rect)
    }
}
