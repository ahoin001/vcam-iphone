import Foundation

extension StudioSession {
    func selectWhiteBalance(_ preset: WhiteBalancePreset) {
        if let kelvin = preset.kelvin {
            settings.manualWhiteBalance = true
            settings.kelvin = kelvin
        } else {
            settings.manualWhiteBalance = false
        }
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
