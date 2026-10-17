import SwiftUI

struct StudioDockButton: View {
    let systemImage: String
    let label: LocalizedStringResource
    var isSelected = false
    let action: () -> Void

    var body: some View {
        Button(action: action) {
            Image(systemName: systemImage)
                .font(.title3.weight(.semibold))
                .symbolRenderingMode(.monochrome)
                .foregroundStyle(isSelected ? AnyShapeStyle(.yellow) : AnyShapeStyle(.primary))
                .frame(width: 44, height: 44)
                .contentShape(.rect)
        }
        .buttonStyle(StudioDockButtonStyle())
        .accessibilityLabel(Text(label))
        .accessibilityAddTraits(isSelected ? .isSelected : [])
    }
}
