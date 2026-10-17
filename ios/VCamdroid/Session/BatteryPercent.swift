import UIKit

enum BatteryPercent {
    @MainActor
    static func current() -> Int {
        let device = UIDevice.current
        device.isBatteryMonitoringEnabled = true
        let level = device.batteryLevel
        guard level >= 0 else { return 100 }
        return Int((level * 100).rounded())
    }
}
