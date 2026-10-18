import Foundation

extension StudioSession {
    func selectWhiteBalance(_ preset: WhiteBalancePreset) {
        var next = settings
        if let kelvin = preset.kelvin {
            next.manualWhiteBalance = true
            next.kelvin = kelvin
        } else {
            next.manualWhiteBalance = false
        }
        settings = next
        applyControls()
    }

    func setManualExposure(_ enabled: Bool) {
        settings.manualExposure = enabled
        applyControls()
    }

    func setManualFocus(_ enabled: Bool) {
        settings.manualFocus = enabled
        applyControls()
    }

    func toggleTorch() {
        settings.torch.toggle()
        applyControls()
    }

    func toggleFlip() {
        settings.flip180.toggle()
        applyControls()
    }

    var selectedMode: VideoMode {
        get { settings.mode }
        set { setMode(newValue) }
    }

    var framesPerSecond: Int {
        get { settings.fps }
        set { updateFrameRate(newValue) }
    }

    var bitrate: Double {
        get { Double(settings.bitrateKbps) }
        set { setBitrate(Int(newValue.rounded())) }
    }
}
