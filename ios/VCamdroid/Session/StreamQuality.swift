import Foundation

enum H264Profile: Equatable, Sendable {
    case baseline
    case high
}

/// Encode settings for one video client. The panel rate is chosen when that client connects and is not stored in `StudioSettings`, which clamps the OBS slider to 2500–16000 kbps.
struct StreamQuality: Equatable, Sendable {
    var width: Int
    var height: Int
    var fps: Int
    var bitrateKbps: Int
    var peakMultiplier: Int
    var profile: H264Profile
    /// Seconds between keyframes. The panel can wait longer because USB does not drop packets. The OBS source keeps the saved setting.
    var keyframeInterval: Int

    static func panel(usb: Bool) -> StreamQuality {
        StreamQuality(
            width: 1920,
            height: 1080,
            fps: 30,
            bitrateKbps: usb ? 30_000 : 12_000,
            peakMultiplier: 2,
            profile: .high,
            keyframeInterval: 2
        )
    }

    static func legacy(width: Int, height: Int, fps: Int, bitrateKbps: Int, keyframeInterval: Int = 1) -> StreamQuality {
        StreamQuality(
            width: width,
            height: height,
            fps: fps,
            bitrateKbps: bitrateKbps,
            peakMultiplier: 1,
            profile: .baseline,
            keyframeInterval: keyframeInterval
        )
    }
}
