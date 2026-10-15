import SwiftUI

@main
struct VCamdroidApp: App {
    @State private var session = StudioSession()

    var body: some Scene {
        WindowGroup {
            HomeView()
                .environment(session)
        }
    }
}
