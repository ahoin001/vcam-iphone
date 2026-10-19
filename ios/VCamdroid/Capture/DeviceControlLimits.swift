import Foundation

/// What the active camera will accept without throwing. Unknown cameras start as allowed until the session reads the device.
struct DeviceControlLimits: Equatable, Sendable {
    var customWhiteBalance: Bool
    var customFocus: Bool
    var customExposure: Bool
    var torch: Bool

    static let unknown = DeviceControlLimits(
        customWhiteBalance: true,
        customFocus: true,
        customExposure: true,
        torch: true
    )
}
