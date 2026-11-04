import SwiftUI

struct StudioStatusBar: View {
    @Environment(StudioSession.self) private var session
    var onHelp: () -> Void
    var onHeat: () -> Void

    var body: some View {
        HStack(alignment: .center, spacing: 12) {
            StudioGlassIcon(systemImage: "questionmark", label: .helpAction, action: onHelp)
            StudioGlassIcon(systemImage: "thermometer.medium", label: .heatAction, action: onHeat)
            ConnectionBanner()
                .frame(maxWidth: .infinity, alignment: .leading)
            if session.settings.showTally {
                TallyBadge(tally: session.snapshot.tally)
            }
        }
    }
}
