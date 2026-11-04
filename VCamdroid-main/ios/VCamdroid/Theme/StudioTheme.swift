import SwiftUI

enum StudioTheme {
    static let paper = Color(red: 22 / 255, green: 22 / 255, blue: 24 / 255)
    static let ink = Color(red: 245 / 255, green: 245 / 255, blue: 247 / 255)
    static let ink2 = Color(red: 199 / 255, green: 199 / 255, blue: 204 / 255)
    static let onInk = Color(red: 22 / 255, green: 22 / 255, blue: 24 / 255)
    static let surface = Color(red: 32 / 255, green: 32 / 255, blue: 36 / 255)
    static let raised = Color(red: 42 / 255, green: 42 / 255, blue: 46 / 255)
    static let prism = Color(red: 109 / 255, green: 123 / 255, blue: 255 / 255)
    static let accent = Color(red: 167 / 255, green: 123 / 255, blue: 255 / 255)
    static let live = Color(red: 48 / 255, green: 209 / 255, blue: 88 / 255)
    static let warn = Color(red: 255 / 255, green: 181 / 255, blue: 71 / 255)
    static let danger = Color(red: 255 / 255, green: 107 / 255, blue: 129 / 255)
    static let primaryFill = Color(red: 245 / 255, green: 245 / 255, blue: 247 / 255)

    static let pressScale: CGFloat = 0.98
    static let ease = Animation.timingCurve(0.2, 0.8, 0.2, 1, duration: 0.2)

    static var prismFill: LinearGradient {
        LinearGradient(
            colors: [
                Color(red: 109 / 255, green: 123 / 255, blue: 255 / 255),
                Color(red: 167 / 255, green: 123 / 255, blue: 255 / 255),
                Color(red: 255 / 255, green: 143 / 255, blue: 200 / 255)
            ],
            startPoint: .topLeading,
            endPoint: .bottomTrailing
        )
    }
}
