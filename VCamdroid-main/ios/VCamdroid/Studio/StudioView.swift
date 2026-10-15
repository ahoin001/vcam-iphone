import SwiftUI

struct StudioView: View {
    @Environment(StudioSession.self) private var session
    @State private var confirmingStop = false

    var body: some View {
        VStack {
            HStack {
                if session.settings.showTally {
                    TallyBadge(tally: session.snapshot.tally)
                }
                ConnectionBanner()
                Spacer()
            }
            PreviewStage()
            ScrollView {
                ControlRack()
            }
            .scrollIndicators(.hidden)
            HStack {
                Button(String(localized: .curtain), systemImage: "moon.fill", action: session.engageCurtain)
                    .buttonStyle(PressableButtonStyle())
                NavigationLink(value: AppRoute.settings) {
                    Label(String(localized: .openSettings), systemImage: "gearshape")
                }
                NavigationLink(value: AppRoute.log) {
                    Label(String(localized: .openLog), systemImage: "list.bullet")
                }
                Button(String(localized: .stop), systemImage: "stop.fill", action: requestStop)
                    .buttonStyle(PressableButtonStyle(prominent: true))
            }
        }
        .padding()
        .navigationTitle(Text(.studioTitle))
        .navigationBarTitleDisplayMode(.inline)
        .task {
            await session.start()
        }
        .simultaneousGesture(
            DragGesture(minimumDistance: 0).onEnded { _ in
                session.noteActivity()
            }
        )
        .confirmationDialog(Text(.confirmStopTitle), isPresented: $confirmingStop, titleVisibility: .visible) {
            Button(String(localized: .stop), role: .destructive, action: stop)
            Button(String(localized: .notNow), role: .cancel, action: dismissStop)
        } message: {
            Text(.confirmStopBody)
        }
    }

    private func requestStop() {
        if session.confirmStopIfNeeded() {
            confirmingStop = true
        } else {
            stop()
        }
    }

    private func stop() {
        Task { await session.stop() }
    }

    private func dismissStop() {
        confirmingStop = false
    }
}
