import SwiftUI

struct TallyBadge: View {
    let tally: TallyState

    var body: some View {
        Text(label)
            .font(.caption)
            .bold()
            .padding()
            .foregroundStyle(color)
            .background(color.opacity(0.15), in: .capsule)
    }

    private var label: String {
        switch tally {
        case .program: String(localized: .tallyProgram)
        case .preview: String(localized: .tallyPreview)
        case .idle: String(localized: .tallyStandby)
        }
    }

    private var color: Color {
        switch tally {
        case .program: .red
        case .preview: .green
        case .idle: .secondary
        }
    }
}
