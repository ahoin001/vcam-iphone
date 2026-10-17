enum Lens: String, Codable, Equatable, Sendable, CaseIterable, Identifiable {
    case back
    case front

    var id: String { rawValue }
}
