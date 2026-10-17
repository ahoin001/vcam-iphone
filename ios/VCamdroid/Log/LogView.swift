import SwiftUI

struct LogView: View {
    @Environment(StudioSession.self) private var session

    var body: some View {
        List(session.logs) { entry in
            LogRow(entry: entry)
        }
        .navigationTitle(Text(.logTitle))
    }
}
