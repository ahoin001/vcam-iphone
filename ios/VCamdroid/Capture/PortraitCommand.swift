import Foundation

enum PortraitGrade: Equatable, Sendable {
    case off
    case balanced
    case fast
    case paused
}

struct PortraitCommand: Equatable, Sendable {
    var grade: PortraitGrade
    var strength: Double

    static let off = PortraitCommand(grade: .off, strength: 0)
}

enum PortraitCare: Equatable, Sendable {
    case hidden
    case steady
    case heavierMode
    case lowBattery
    case heavierAndBattery
    case eased
    case paused

    var message: LocalizedStringResource? {
        switch self {
        case .hidden:
            nil
        case .steady:
            .portraitCareSteady
        case .heavierMode:
            .portraitCareHeavier
        case .lowBattery:
            .portraitCareBattery
        case .heavierAndBattery:
            .portraitCareHeavierBattery
        case .eased:
            .portraitCareEased
        case .paused:
            .portraitCarePaused
        }
    }
}

enum PortraitPolicy {
    static func grade(enabled: Bool, thermal: ProcessInfo.ThermalState) -> PortraitGrade {
        if !enabled {
            .off
        } else {
            switch thermal {
            case .critical: .paused
            case .serious: .fast
            default: .balanced
            }
        }
    }

    static func care(
        enabled: Bool,
        thermal: ProcessInfo.ThermalState,
        fps: Int,
        height: Int,
        batteryPercent: Int,
        unplugged: Bool
    ) -> PortraitCare {
        guard enabled else { return .hidden }
        switch thermal {
        case .critical:
            return .paused
        case .serious:
            return .eased
        default:
            break
        }
        let heavier = fps > 30 || height > 1080
        let lowBattery = unplugged && batteryPercent < 20
        if heavier && lowBattery { return .heavierAndBattery }
        if heavier { return .heavierMode }
        if lowBattery { return .lowBattery }
        return .steady
    }
}
