import SwiftUI

struct StudioReadout: View {
    @Environment(\.accessibilityReduceMotion) private var reduceMotion
    let title: LocalizedStringResource
    private let value: Text

    init(title: LocalizedStringResource, value: String) {
        self.title = title
        self.value = Text(value)
    }

    init(title: LocalizedStringResource, value: LocalizedStringResource) {
        self.title = title
        self.value = Text(value)
    }

    var body: some View {
        HStack(alignment: .firstTextBaseline) {
            Text(title)
                .font(.subheadline.weight(.semibold))
            Spacer(minLength: 8)
            value
                .font(.subheadline.weight(.semibold).monospacedDigit())
                .foregroundStyle(.yellow)
                .contentTransition(reduceMotion ? .identity : .numericText())
        }
    }
}
