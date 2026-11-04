import Foundation

struct ShotPreset: Codable, Equatable, Sendable, Identifiable {
    var id: String
    var name: String
    var lens: Lens
    var zoom: Double
    var manualExposure: Bool
    var exposureBias: Double
    var iso: Double
    var shutterSeconds: Double
    var whiteBalance: WhiteBalancePreset
    var manualFocus: Bool
    var lensPosition: Double
    var torch: Bool
    var flip180: Bool
    var portraitEnabled: Bool
    var portraitStrength: Double

    init(name: String, settings: StudioSettings) {
        id = UUID().uuidString
        self.name = name
        lens = settings.lens
        zoom = settings.zoom
        manualExposure = settings.manualExposure
        exposureBias = settings.exposureBias
        iso = settings.iso
        shutterSeconds = settings.shutterSeconds
        whiteBalance = Self.whiteBalance(for: settings)
        manualFocus = settings.manualFocus
        lensPosition = settings.lensPosition
        torch = settings.torch
        flip180 = settings.flip180
        portraitEnabled = settings.portraitEnabled
        portraitStrength = settings.portraitStrength
    }

    func asControlUpdate() -> ControlUpdate {
        var update = ControlUpdate()
        update.lens = lens
        update.zoom = zoom
        update.manualExposure = manualExposure
        update.exposureBias = exposureBias
        update.iso = iso
        update.shutter = shutterSeconds
        update.whiteBalance = whiteBalance
        update.manualFocus = manualFocus
        update.focus = lensPosition
        update.torch = torch
        update.flip = flip180
        update.portrait = portraitEnabled
        update.portraitStrength = portraitStrength
        return update
    }

    private static func whiteBalance(for settings: StudioSettings) -> WhiteBalancePreset {
        guard settings.manualWhiteBalance else { return .auto }
        let presets = WhiteBalancePreset.allCases.filter { $0.kelvin != nil }
        return presets.min { lhs, rhs in
            abs((lhs.kelvin ?? 0) - settings.kelvin) < abs((rhs.kelvin ?? 0) - settings.kelvin)
        } ?? .daylight
    }
}

enum PresetCommand: Equatable, Sendable {
    case save(String)
    case apply(String)
    case delete(String)
    case rename(String, String)
}

enum ShotPresetStore {
    private static let key = "studio.presets.v1"

    static func load() -> [ShotPreset] {
        guard let data = UserDefaults.standard.data(forKey: key),
              let presets = try? JSONDecoder().decode([ShotPreset].self, from: data) else {
            return []
        }
        return presets
    }

    static func save(_ presets: [ShotPreset]) {
        guard let data = try? JSONEncoder().encode(presets) else { return }
        UserDefaults.standard.set(data, forKey: key)
    }

    static func json(_ presets: [ShotPreset]) -> String {
        guard let data = try? JSONEncoder().encode(presets),
              let body = String(data: data, encoding: .utf8) else {
            return "{\"presets\":[]}"
        }
        return "{\"presets\":\(body)}"
    }

    static func cleanedName(_ raw: String) -> String? {
        let trimmed = raw.trimmingCharacters(in: .whitespacesAndNewlines)
        guard !trimmed.isEmpty else { return nil }
        if trimmed.count <= 40 {
            return trimmed
        }
        return String(trimmed.prefix(40))
    }
}
