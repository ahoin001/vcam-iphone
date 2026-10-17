import SwiftUI

struct SettingsView: View {
    @Environment(StudioSession.self) private var session
    @State private var presetName = ""
    @State private var renaming: ShotPreset?
    @State private var renameText = ""
    @State private var helpPresented = false

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
                TextField(.presetName, text: $presetName)
                Button(.savePreset, action: savePreset)
                    .disabled(ShotPresetStore.cleanedName(presetName) == nil)
                ForEach(session.shotPresets) { preset in
                    Button(preset.name) {
                        _ = session.perform(.apply(preset.id))
                    }
                    .swipeActions {
                        Button(.renamePreset) {
                            renaming = preset
                            renameText = preset.name
                        }
                        .tint(.orange)
                        Button(.deletePreset, role: .destructive) {
                            _ = session.perform(.delete(preset.id))
                        }
                    }
                }
            } header: {
                Text(.presetsSection)
            }

            Section {
                TextField(
                    .port,
                    value: StudioBindings.setting(\.port, on: session),
                    format: .number
                )
                .keyboardType(.numberPad)
                Toggle(.discovery, isOn: StudioBindings.setting(\.discoveryEnabled, on: session))
            } header: {
                Text(.connectionSection)
            } footer: {
                Text(.discoveryFooter)
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
                Text(.advancedSection)
            } footer: {
                Text(.outputSettingsFooter)
            }

            Section {
                Button(.helpAction) { helpPresented = true }
                Button(.replaySetup) { session.replaySetup() }
                NavigationLink(value: AppRoute.log) {
                    Label {
                        Text(.openLog)
                    } icon: {
                        Image(systemName: "list.bullet")
                    }
                }
            }
        }
        .navigationTitle(Text(.settingsTitle))
        .sheet(isPresented: $helpPresented) {
            NavigationStack {
                HowToConnectView {
                    helpPresented = false
                    session.replaySetup()
                }
            }
        }
        .alert(Text(.presetRenameTitle), isPresented: renamePresented, presenting: renaming) { preset in
            TextField(.presetName, text: $renameText)
            Button(.renamePreset) {
                if let name = ShotPresetStore.cleanedName(renameText) {
                    _ = session.perform(.rename(preset.id, name))
                }
            }
            Button(.notNow, role: .cancel) {}
        }
    }

    private var renamePresented: Binding<Bool> {
        Binding(
            get: { renaming != nil },
            set: { if !$0 { renaming = nil } }
        )
    }

    private func savePreset() {
        guard let name = ShotPresetStore.cleanedName(presetName) else { return }
        _ = session.perform(.save(name))
        presetName = ""
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
