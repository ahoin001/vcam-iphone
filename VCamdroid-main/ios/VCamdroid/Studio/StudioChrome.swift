import SwiftUI

enum StudioTool: Hashable, CaseIterable, Identifiable {
    case zoom
    case exposure
    case whiteBalance
    case focus
    case portrait

    var id: Self { self }

    var label: LocalizedStringResource {
        switch self {
        case .zoom: .zoom
        case .exposure: .exposure
        case .whiteBalance: .whiteBalance
        case .focus: .focus
        case .portrait: .portrait
        }
    }

    var systemImage: String {
        switch self {
        case .zoom: "plus.magnifyingglass"
        case .exposure: "sun.max"
        case .whiteBalance: "circle.lefthalf.filled"
        case .focus: "scope"
        case .portrait: "person.and.background.dotted"
        }
    }
}

struct StudioScrim: View {
    var body: some View {
        VStack(spacing: 0) {
            LinearGradient(
                colors: [.black.opacity(0.62), .black.opacity(0)],
                startPoint: .top,
                endPoint: .bottom
            )
            .frame(height: 148)
            Spacer(minLength: 0)
            LinearGradient(
                colors: [.black.opacity(0), .black.opacity(0.72)],
                startPoint: .top,
                endPoint: .bottom
            )
            .frame(height: 260)
        }
    }
}

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
                        ScrollView(.horizontal) {
                            HStack(spacing: 12) {
                                StudioToolDock(tool: $tool)
                                StudioSessionDock(onStop: onStop, spreads: false)
                            }
                        }
                        .scrollIndicators(.hidden)
                        .fixedSize(horizontal: false, vertical: true)
                    } else {
                        toolDock
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

    private var toolDock: some View {
        GeometryReader { proxy in
            ScrollView(.horizontal) {
                StudioToolDock(tool: $tool)
                    .frame(minWidth: proxy.size.width)
            }
            .scrollIndicators(.hidden)
            .scrollBounceBehavior(.basedOnSize, axes: .horizontal)
        }
        .frame(height: 52)
    }
}

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

struct StudioToolDock: View {
    @Environment(StudioSession.self) private var session
    @Binding var tool: StudioTool?

    var body: some View {
        HStack(spacing: 0) {
            ForEach(StudioTool.allCases) { item in
                StudioDockButton(
                    systemImage: item.systemImage,
                    label: item.label,
                    isSelected: tool == item || (item == .portrait && session.settings.portraitEnabled),
                    action: { toggle(item) }
                )
            }
            StudioDockButton(
                systemImage: "arrow.triangle.2.circlepath.camera",
                label: .switchCamera,
                action: switchLens
            )
            .accessibilityValue(Text(session.settings.lens == .back ? .backLens : .frontLens))
            StudioDockButton(
                systemImage: session.settings.torch ? "flashlight.on.fill" : "flashlight.off.fill",
                label: .torch,
                isSelected: session.settings.torch,
                action: session.toggleTorch
            )
            StudioDockButton(
                systemImage: "rotate.right",
                label: .flip,
                isSelected: session.settings.flip180,
                action: session.toggleFlip
            )
        }
        .padding(.horizontal, 4)
        .glassEffect(.regular, in: .capsule)
        .sensoryFeedback(.selection, trigger: session.settings.lens)
        .sensoryFeedback(.impact(weight: .light, intensity: 0.8), trigger: session.settings.torch)
        .sensoryFeedback(.impact(weight: .light, intensity: 0.8), trigger: session.settings.flip180)
        .sensoryFeedback(.impact(weight: .light, intensity: 0.8), trigger: session.settings.portraitEnabled)
    }

    private func toggle(_ item: StudioTool) {
        tool = tool == item ? nil : item
    }

    private func switchLens() {
        session.selectLens(session.settings.lens == .back ? .front : .back)
    }
}

struct StudioSessionDock: View {
    @Environment(StudioSession.self) private var session
    var onStop: () -> Void
    var spreads: Bool

    var body: some View {
        HStack(spacing: 12) {
            HStack(spacing: 0) {
                StudioDockButton(systemImage: "moon.fill", label: .curtain, action: session.engageCurtain)
                NavigationLink(value: AppRoute.settings) {
                    dockIcon("gearshape")
                }
                .buttonStyle(StudioDockButtonStyle())
                .accessibilityLabel(Text(.openSettings))
                NavigationLink(value: AppRoute.log) {
                    dockIcon("list.bullet")
                }
                .buttonStyle(StudioDockButtonStyle())
                .accessibilityLabel(Text(.openLog))
            }
            .padding(.horizontal, 4)
            .glassEffect(.regular, in: .capsule)
            if spreads {
                Spacer(minLength: 12)
            }
            StudioGlassIcon(systemImage: "stop.fill", label: .stop, tint: .red, action: onStop)
        }
    }

    private func dockIcon(_ name: String) -> some View {
        Image(systemName: name)
            .font(.system(size: 20, weight: .semibold))
            .foregroundStyle(.primary)
            .frame(width: 44, height: 44)
            .contentShape(.rect)
    }
}

struct StudioDockButton: View {
    let systemImage: String
    let label: LocalizedStringResource
    var isSelected = false
    let action: () -> Void

    var body: some View {
        Button(action: action) {
            Image(systemName: systemImage)
                .font(.system(size: 20, weight: .semibold))
                .symbolRenderingMode(.monochrome)
                .foregroundStyle(isSelected ? AnyShapeStyle(.yellow) : AnyShapeStyle(.primary))
                .frame(width: 44, height: 44)
                .contentShape(.rect)
        }
        .buttonStyle(StudioDockButtonStyle())
        .accessibilityLabel(Text(label))
        .accessibilityAddTraits(isSelected ? .isSelected : [])
    }
}

struct StudioGlassIcon: View {
    let systemImage: String
    let label: LocalizedStringResource
    var tint: Color = .primary
    let action: () -> Void

    var body: some View {
        Button(action: action) {
            Image(systemName: systemImage)
                .font(.system(size: 17, weight: .semibold))
                .foregroundStyle(tint)
                .frame(width: 44, height: 44)
                .contentShape(.rect)
        }
        .buttonStyle(StudioDockButtonStyle())
        .glassEffect(.regular, in: .circle)
        .accessibilityLabel(Text(label))
    }
}

struct StudioDockButtonStyle: ButtonStyle {
    @Environment(\.accessibilityReduceMotion) private var reduceMotion

    func makeBody(configuration: Configuration) -> some View {
        configuration.label
            .scaleEffect(!reduceMotion && configuration.isPressed ? 0.96 : 1)
            .opacity(reduceMotion && configuration.isPressed ? 0.6 : 1)
            .animation(.timingCurve(0.23, 1, 0.32, 1, duration: 0.14), value: configuration.isPressed)
    }
}

struct StudioReadout: View {
    @Environment(\.accessibilityReduceMotion) private var reduceMotion
    let title: LocalizedStringResource
    let value: String

    var body: some View {
        HStack(alignment: .firstTextBaseline) {
            Text(title)
                .font(.subheadline.weight(.semibold))
            Spacer(minLength: 8)
            Text(value)
                .font(.subheadline.weight(.semibold).monospacedDigit())
                .foregroundStyle(.yellow)
                .contentTransition(reduceMotion ? .identity : .numericText())
        }
    }
}
