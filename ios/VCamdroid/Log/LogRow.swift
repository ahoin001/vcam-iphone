import SwiftUI

struct LogRow: View {
    let entry: LogEntry

    var body: some View {
        VStack(alignment: .leading) {
            Text(entry.date, format: .dateTime.hour().minute().second())
                .font(.caption)
                .foregroundStyle(.secondary)
            Text(entry.message)
        }
    }
}
