import SwiftUI

struct SettingsView: View {
    @Environment(StudioSession.self) private var session

    var body: some View {
        Form {
            Picker(String(localized: .resolution), selection: StudioBindings.mode(on: session)) {
                ForEach(VideoMode.allowed(allowsHighFrameRate: session.settings.allowsHighFrameRate)) { mode in
                    Text(mode.label).tag(mode)
                }
            }
            Picker(String(localized: .framesPerSecond), selection: StudioBindings.framesPerSecond(on: session)) {
                Text(30, format: .number).tag(30)
                if session.settings.allowsHighFrameRate {
                    Text(60, format: .number).tag(60)
                }
            }
            Section {
                Toggle(String(localized: .portrait), isOn: StudioBindings.setting(\.portraitEnabled, on: session))
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
                }
            } header: {
                Text(.portrait)
            } footer: {
                Text(.portraitSettingsFooter)
            }
            LabeledContent {
                Text(session.settings.bitrateKbps, format: .number)
            } label: {
                Text(.bitrate)
            }
            Slider(value: StudioBindings.bitrate(on: session), in: 2500...16000)
                .onChange(of: session.settings.bitrateKbps) {
                    session.applyControls()
                }
            TextField(
                String(localized: .port),
                value: StudioBindings.setting(\.port, on: session),
                format: .number
            )
            .keyboardType(.numberPad)
            .onChange(of: session.settings.port) {
                session.applyControls()
            }
            Picker(String(localized: .curtainMode), selection: StudioBindings.setting(\.curtainMode, on: session)) {
                Text(.curtainAutomatic).tag(CurtainMode.automatic)
                Text(.curtainAlways).tag(CurtainMode.on)
                Text(.curtainOff).tag(CurtainMode.off)
            }
            .onChange(of: session.settings.curtainMode) {
                session.setCurtainMode(session.settings.curtainMode)
            }
            Toggle(String(localized: .showTally), isOn: StudioBindings.setting(\.showTally, on: session))
            Toggle(String(localized: .confirmStopToggle), isOn: StudioBindings.setting(\.confirmStop, on: session))
        }
        .navigationTitle(Text(.settingsTitle))
        .onChange(of: session.settings) {
            StudioSettingsStore.save(session.settings)
        }
    }
}
