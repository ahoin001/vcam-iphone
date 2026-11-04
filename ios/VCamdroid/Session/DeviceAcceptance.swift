enum DeviceAcceptance {
    /// The ten checks from the product spec. Protocol checks run in this test target.
    /// The capture, USB, and thermal checks run on an iPhone 16 Pro with OBS.
    static let checks = [
        "Wi-Fi OBS source reaches the phone IP on port 4747 and tally follows program, preview, and idle",
        "usbmux forward lets OBS connect to 127.0.0.1:4747, and pulling the cable returns the phone to waiting",
        "H.264 packets are Annex-B, Baseline, no B-frames, with a NO_PTS codec-config packet before picture data",
        "1080p30 stays up for 30 minutes over USB with the curtain on and the UI still opens",
        "1080p60 stays out of the picker until a 30-minute device run passes",
        "PUT /v1/session changes lens and fps and OBS sees the new frame size",
        "Manual exposure, white balance, focus, zoom, and torch change the stream without a reconnect",
        "The side button pauses the stream and the copy does not promise power-button streaming",
        "Denying Local Network still allows USB and a manual IP",
        "Front and back previews keep square pixels in the preview and in OBS"
    ]
}
