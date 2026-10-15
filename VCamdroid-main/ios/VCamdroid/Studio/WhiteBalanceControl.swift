import SwiftUI

struct WhiteBalanceControl: View {
    @Environment(StudioSession.self) private var session

    var body: some View {
        VStack(alignment: .leading) {
            Text(.whiteBalance)
            ForEach(WhiteBalancePreset.allCases) { preset in
                Button(title(preset), action: { session.selectWhiteBalance(preset) })
                    .buttonStyle(PressableButtonStyle())
            }
        }
    }

    private func title(_ preset: WhiteBalancePreset) -> String {
        switch preset {
        case .auto: String(localized: .auto)
        case .daylight: String(localized: .daylight)
        case .cloudy: String(localized: .cloudy)
        case .fluorescent: String(localized: .fluorescent)
        case .incandescent: String(localized: .incandescent)
        }
    }
}
