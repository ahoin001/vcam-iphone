import Foundation

struct SessionUpdate: Equatable, Sendable {
    var width: Int?
    var height: Int?
    var fps: Int?
    var format: StreamFormat?
    var lens: Lens?
}

struct ControlUpdate: Equatable, Sendable {
    var lens: Lens?
    var zoom: Double?
    var manualExposure: Bool?
    var exposureBias: Double?
    var iso: Double?
    var shutter: Double?
    var whiteBalance: WhiteBalancePreset?
    var manualFocus: Bool?
    var focus: Double?
    var torch: Bool?
    var flip: Bool?
    var portrait: Bool?
    var portraitStrength: Double?
    var bitrateKbps: Int?
}

struct ControlState: Equatable, Sendable {
    var lens: Lens
    var zoom: Double
    var manualExposure: Bool
    var exposureBias: Double
    var iso: Double
    var shutter: Double
    var whiteBalance: WhiteBalancePreset
    var manualFocus: Bool
    var focus: Double
    var torch: Bool
    var flip: Bool
    var portrait: Bool
    var portraitStrength: Double
    var bitrateKbps: Int
    var activeBitrateKbps: Int

    init(settings: StudioSettings, activeBitrateKbps: Int? = nil) {
        lens = settings.lens
        zoom = settings.zoom
        manualExposure = settings.manualExposure
        exposureBias = settings.exposureBias
        iso = settings.iso
        shutter = settings.shutterSeconds
        whiteBalance = Self.preset(for: settings)
        manualFocus = settings.manualFocus
        focus = settings.lensPosition
        torch = settings.torch
        flip = settings.flip180
        portrait = settings.portraitEnabled
        portraitStrength = settings.portraitStrength
        bitrateKbps = settings.bitrateKbps
        self.activeBitrateKbps = activeBitrateKbps ?? settings.bitrateKbps
    }

    func json(success: Bool? = nil) -> String {
        var fields: [String] = []
        if let success {
            fields.append("\"success\":\(success ? "true" : "false")")
        }
        fields.append("\"lens\":\"\(lens.rawValue)\"")
        fields.append("\"zoom\":\(Self.number(zoom))")
        fields.append("\"manualExposure\":\(manualExposure ? "true" : "false")")
        fields.append("\"exposureBias\":\(Self.number(exposureBias))")
        fields.append("\"iso\":\(Self.number(iso))")
        fields.append("\"shutter\":\(Self.number(shutter))")
        fields.append("\"whiteBalance\":\"\(whiteBalance.rawValue)\"")
        fields.append("\"manualFocus\":\(manualFocus ? "true" : "false")")
        fields.append("\"focus\":\(Self.number(focus))")
        fields.append("\"torch\":\(torch ? "true" : "false")")
        fields.append("\"flip\":\(flip ? "true" : "false")")
        fields.append("\"portrait\":\(portrait ? "true" : "false")")
        fields.append("\"portraitStrength\":\(Self.number(portraitStrength))")
        fields.append("\"bitrateKbps\":\(bitrateKbps)")
        fields.append("\"activeBitrateKbps\":\(activeBitrateKbps)")
        return "{\(fields.joined(separator: ","))}"
    }

    private static func preset(for settings: StudioSettings) -> WhiteBalancePreset {
        guard settings.manualWhiteBalance else { return .auto }
        let presets = WhiteBalancePreset.allCases.filter { $0.kelvin != nil }
        return presets.min { lhs, rhs in
            abs((lhs.kelvin ?? 0) - settings.kelvin) < abs((rhs.kelvin ?? 0) - settings.kelvin)
        } ?? .daylight
    }

    private static func number(_ value: Double) -> String {
        var text = String(format: "%.4f", locale: Locale(identifier: "en_US_POSIX"), value)
        if text.contains(".") {
            while text.last == "0" {
                text.removeLast()
            }
            if text.last == "." {
                text.removeLast()
            }
        }
        if text == "-0" {
            return "0"
        }
        return text
    }
}

enum VideoClientKind: Equatable, Sendable {
    case legacy
    case panel
}

struct VideoRequest: Equatable, Sendable {
    var format: StreamFormat
    var width: Int
    var height: Int
    var client: VideoClientKind = .legacy
}

enum DroidCamRequest: Equatable, Sendable {
    case ping
    case resolutions
    case capabilities
    case session
    case update(SessionUpdate)
    case controls
    case applyControls(ControlUpdate)
    case presets
    case presetCommand(PresetCommand)
    case panel
    case preview
    case battery
    case tally(TallyState)
    case video(VideoRequest)
    case audio
    case unknown
}
