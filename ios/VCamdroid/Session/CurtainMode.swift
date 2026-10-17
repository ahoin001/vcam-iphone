enum CurtainMode: String, Codable, Equatable, Hashable, Sendable, CaseIterable, Identifiable {
    case automatic
    case on
    case off

    var id: String { rawValue }
}
