import SwiftUI

struct StudioView: View {
    @Environment(StudioSession.self) private var session
    @Environment(\.dismiss) private var dismiss
    @Environment(\.accessibilityReduceMotion) private var reduceMotion
    @Environment(\.verticalSizeClass) private var verticalSizeClass
    @State private var tool: StudioTool?
    @State private var confirmingStop = false

    var body: some View {
        ZStack {
            PreviewStage(onPreviewTap: dismissTool)
                .ignoresSafeArea()
            StudioScrim()
                .ignoresSafeArea()
                .allowsHitTesting(false)
            StudioChrome(
                tool: $tool,
                isCompactHeight: verticalSizeClass == .compact,
                onBack: goBack,
                onStop: requestStop
            )
            .padding(.horizontal, 16)
            .padding(.bottom, 8)
            .opacity(session.curtainPresented ? 0 : 1)
            .allowsHitTesting(!session.curtainPresented)
            .accessibilityHidden(session.curtainPresented)
            CurtainOverlay()
                .opacity(session.curtainPresented ? 1 : 0)
                .scaleEffect(reduceMotion ? 1 : (session.curtainPresented ? 1 : 0.98))
                .animation(curtainAnimation, value: session.curtainPresented)
                .allowsHitTesting(session.curtainPresented)
                .ignoresSafeArea()
        }
        .background {
            StudioOrientation { session.setInterfaceOrientation($0) }
        }
        .preferredColorScheme(.dark)
        .toolbar(.hidden, for: .navigationBar)
        .toolbarBackground(.hidden, for: .navigationBar)
        .task {
            await session.start()
        }
        .simultaneousGesture(
            DragGesture(minimumDistance: 0).onEnded { _ in
                session.noteActivity()
            }
        )
        .confirmationDialog(Text(.confirmStopTitle), isPresented: $confirmingStop, titleVisibility: .visible) {
            Button(.stop, role: .destructive, action: stop)
            Button(.notNow, role: .cancel, action: dismissStop)
        } message: {
            Text(.confirmStopBody)
        }
    }

    private var curtainAnimation: Animation {
        reduceMotion ? .easeOut(duration: 0.2) : .timingCurve(0.23, 1, 0.32, 1, duration: 0.22)
    }

    private func goBack() {
        dismiss()
    }

    private func dismissTool() {
        tool = nil
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
