import Foundation

struct LogEntry: Identifiable, Equatable, Sendable {
    let id: UUID
    let date: Date
    let message: String

    init(message: String, date: Date = .now) {
        self.id = UUID()
        self.date = date
        self.message = message
    }
}
