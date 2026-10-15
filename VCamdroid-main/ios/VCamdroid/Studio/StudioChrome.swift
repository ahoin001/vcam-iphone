import SwiftUI

struct StudioChrome: View {
    @Binding var tool: StudioTool?
    var isCompactHeight: Bool
    var onBack: () -> Void
    var onStop: () -> Void
    @Environment(StudioSession.self) private var session
    @Environment(\.accessibilityReduceMotion) private var reduceMotion

    var body: some View {
        VStack(spacing: 12) {
            GlassEffectContainer(spacing: 8) {
                StudioStatusBar(onBack: onBack)
            }
            if session.portraitCare != .hidden {
                PortraitCareBanner()
            }
            Spacer(minLength: 0)
            GlassEffectContainer(spacing: 8) {
                VStack(spacing: 12) {
                    if let tool {
                        ControlRack(tool: tool)
                            .padding(16)
                            .frame(maxWidth: .infinity, alignment: .leading)
                            .glassEffect(.regular, in: .rect(cornerRadius: 28))
                            .transition(reduceMotion ? .opacity : .move(edge: .bottom).combined(with: .opacity))
                    }
                    if isCompactHeight {
                        StudioCompactDock(tool: $tool, onStop: onStop)
                    } else {
                        StudioToolDockScroller(tool: $tool)
                        StudioSessionDock(onStop: onStop, spreads: true)
                    }
                }
            }
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity)
        .animation(motion, value: tool)
        .sensoryFeedback(.selection, trigger: tool) { _, newValue in
            newValue != nil
        }
    }

    private var motion: Animation {
        reduceMotion ? .easeOut(duration: 0.2) : .spring(response: 0.32, dampingFraction: 1)
    }
}
