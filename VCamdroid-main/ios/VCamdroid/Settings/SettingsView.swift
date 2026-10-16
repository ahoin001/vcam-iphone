import SwiftUI

struct SettingsView: View {
    @Environment(StudioSession.self) private var session

    var body: some View {
        Form {
            Section {
                Picker(.curtainMode, selection: StudioBindings.setting(\.curtainMode, on: session)) {
                    Text(.curtainAutomatic).tag(CurtainMode.automatic)
                    Text(.curtainAlways).tag(CurtainMode.on)
                    Text(.curtainOff).tag(CurtainMode.off)
                }
                .onChange(of: session.settings.curtainMode) {
                    session.setCurtainMode(session.settings.curtainMode)
                }
                Toggle(.showTally, isOn: StudioBindings.setting(\.showTally, on: session))
                Toggle(.confirmStopToggle, isOn: StudioBindings.setting(\.confirmStop, on: session))
                NavigationLink(value: AppRoute.log) {
                    Label {
                        Text(.openLog)
                    } icon: {
                        Image(systemName: "list.bullet")
                    }
                }
            } header: {
                Text(.sessionSection)
            } footer: {
                Text(.sessionSettingsFooter)
            }

            Section {
                Toggle(.portrait, isOn: StudioBindings.setting(\.portraitEnabled, on: session))
                    .onChange(of: session.settings.portraitEnabled) {
                        session.applyPortrait()
                    }
                if session.settings.portraitEnabled {
                    LabeledContent {
                        Text(session.settings.portraitStrength, format: .percent.precision(.fractionLength(0)))
                    } label: {
                        Text(.portraitStrength)
                    }
                    Slider(value: StudioBindings.setting(\.portraitStrength, on: session), in: 0...1)
                        .onChange(of: session.settings.portraitStrength) {
                            session.applyPortrait()
                        }
                    Toggle(.portraitPhonePreview, isOn: StudioBindings.setting(\.portraitPhonePreview, on: session))
                        .onChange(of: session.settings.portraitPhonePreview) {
                            session.applyPortrait()
                        }
                }
            } header: {
                Text(.portrait)
            } footer: {
                Text(.portraitSettingsFooter)
            }

            Section {
                TextField(
                    .port,
                    value: StudioBindings.setting(\.port, on: session),
                    format: .number
                )
                .keyboardType(.numberPad)
                .onChange(of: session.settings.port) {
                    session.applyControls()
                }
            } header: {
                Text(.port)
            } footer: {
                Text(.portSettingsFooter)
            }

            Section {
                Picker(.resolution, selection: StudioBindings.mode(on: session)) {
                    ForEach(VideoMode.allowed(allowsHighFrameRate: session.settings.allowsHighFrameRate)) { mode in
                        Text(mode.label).tag(mode)
                    }
                }
                Picker(.framesPerSecond, selection: StudioBindings.framesPerSecond(on: session)) {
                    Text(30, format: .number).tag(30)
                    if session.settings.allowsHighFrameRate {
                        Text(60, format: .number).tag(60)
                    }
                }
                LabeledContent {
                    Text(session.settings.bitrateKbps, format: .number)
                } label: {
                    Text(.bitrate)
                }
                Slider(
                    value: Binding(
                        get: { Double(session.settings.bitrateKbps) },
                        set: { session.setBitrate(Int($0.rounded())) }
                    ),
                    in: 2500...16000,
                    step: 100
                )
                HStack(spacing: 8) {
                    bitrateChip(4000, title: .bitrateLive)
                    bitrateChip(6000, title: .bitrateRecord)
                    bitrateChip(8000, title: .bitrateRecordHigh)
                }
                .controlSize(.small)
            } header: {
                Text(.obsOutputSection)
            } footer: {
                Text(.outputSettingsFooter)
            }
        }
        .navigationTitle(Text(.settingsTitle))
        .onChange(of: session.settings) {
            StudioSettingsStore.save(session.settings)
        }
    }

    private func bitrateChip(_ value: Int, title: LocalizedStringResource) -> some View {
        let selected = session.settings.bitrateKbps == value
        return Button {
            session.setBitrate(value)
        } label: {
            Text(title)
                .frame(maxWidth: .infinity)
        }
        .buttonStyle(.bordered)
        .tint(selected ? Color.accentColor : Color.secondary)
    }
}
