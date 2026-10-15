import CoreMedia
import Foundation

struct CaptureConfiguration: Equatable, Sendable {
    var lens: Lens
    var width: Int
    var height: Int
    var fps: Int
    var zoom: Double
    var torch: Bool
    var exposureBias: Double
    var manualExposure: Bool
    var iso: Double
    var shutterSeconds: Double
    var manualWhiteBalance: Bool
    var kelvin: Double
    var manualFocus: Bool
    var lensPosition: Double
    var flip180: Bool

    init(settings: StudioSettings) {
        lens = settings.lens
        width = settings.width
        height = settings.height
        fps = settings.fps
        zoom = settings.zoom
        torch = settings.torch
        exposureBias = settings.exposureBias
        manualExposure = settings.manualExposure
        iso = settings.iso
        shutterSeconds = settings.shutterSeconds
        manualWhiteBalance = settings.manualWhiteBalance
        kelvin = settings.kelvin
        manualFocus = settings.manualFocus
        lensPosition = settings.lensPosition
        flip180 = settings.flip180
    }
}
