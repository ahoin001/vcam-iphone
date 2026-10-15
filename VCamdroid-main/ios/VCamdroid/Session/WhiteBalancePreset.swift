enum WhiteBalancePreset: String, Equatable, Sendable, CaseIterable, Identifiable {
    case auto
    case daylight
    case cloudy
    case fluorescent
    case incandescent

    var id: String { rawValue }

    var kelvin: Double? {
        switch self {
        case .auto:
            nil
        case .daylight:
            5200
        case .cloudy:
            6000
        case .fluorescent:
            4000
        case .incandescent:
            3200
        }
    }
}
