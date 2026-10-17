import Foundation

enum StudioSettingsStore {
    private static let key = "studio.settings.v1"

    static func load() -> StudioSettings {
        guard let data = UserDefaults.standard.data(forKey: key),
              let settings = try? JSONDecoder().decode(StudioSettings.self, from: data) else {
            return StudioSettings()
        }
        return settings.sanitized()
    }

    static func save(_ settings: StudioSettings) {
        guard let data = try? JSONEncoder().encode(settings.sanitized()) else { return }
        UserDefaults.standard.set(data, forKey: key)
    }
}
