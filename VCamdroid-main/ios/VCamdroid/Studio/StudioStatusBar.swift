import SwiftUI

struct StudioStatusBar: View {
    @Environment(StudioSession.self) private var session
    var onBack: () -> Void

    var body: some View {
        HStack(alignment: .center, spacing: 12) {
            StudioGlassIcon(systemImage: "chevron.backward", label: .backAction, action: onBack)
            ConnectionBanner()
                .frame(maxWidth: .infinity, alignment: .leading)
            if session.settings.showTally {
                TallyBadge(tally: session.snapshot.tally)
            }
        }
    }
}
