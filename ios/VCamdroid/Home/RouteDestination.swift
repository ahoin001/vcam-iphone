import SwiftUI

struct RouteDestination: View {
    let route: AppRoute

    var body: some View {
        switch route {
        case .settings:
            SettingsView()
        case .log:
            LogView()
        }
    }
}
