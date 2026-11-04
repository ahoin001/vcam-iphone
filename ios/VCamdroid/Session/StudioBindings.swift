import SwiftUI

/// Bindings into `StudioSettings`. Nested struct fields are not reference-writable key paths.
enum StudioBindings {
    @MainActor
    static func setting<Value>(
        _ keyPath: WritableKeyPath<StudioSettings, Value>,
        on session: StudioSession
    ) -> Binding<Value> {
        Binding(
            get: { session.settings[keyPath: keyPath] },
            set: { session.settings[keyPath: keyPath] = $0 }
        )
    }

    @MainActor
    static func mode(on session: StudioSession) -> Binding<VideoMode> {
        Binding(get: { session.selectedMode }, set: { session.selectedMode = $0 })
    }

    @MainActor
    static func framesPerSecond(on session: StudioSession) -> Binding<Int> {
        Binding(get: { session.framesPerSecond }, set: { session.framesPerSecond = $0 })
    }

    @MainActor
    static func bitrate(on session: StudioSession) -> Binding<Double> {
        Binding(get: { session.bitrate }, set: { session.bitrate = $0 })
    }
}
