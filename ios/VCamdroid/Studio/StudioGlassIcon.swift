import SwiftUI

struct StudioGlassIcon: View {
    let systemImage: String
    let label: LocalizedStringResource
    var tint: Color = .primary
    let action: () -> Void

    var body: some View {
        Button(action: action) {
            Image(systemName: systemImage)
                .font(.body.weight(.semibold))
                .foregroundStyle(tint)
                .frame(width: 44, height: 44)
                .contentShape(.rect)
        }
        .buttonStyle(StudioDockButtonStyle())
        .glassEffect(.regular, in: .circle)
        .accessibilityLabel(Text(label))
    }
}
