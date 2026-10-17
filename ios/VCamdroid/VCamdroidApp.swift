import SwiftUI

@main
struct VCamdroidApp: App {
    @UIApplicationDelegateAdaptor(AppDelegate.self) private var appDelegate
    @State private var session = StudioSession()

    var body: some Scene {
        WindowGroup {
            HomeView()
                .environment(session)
        }
    }
}
