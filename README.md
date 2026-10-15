# VCam iPhone

Turn an iPhone into a broadcast camera for OBS Studio. The phone previews the shot, encodes H.264, and serves it on your network so OBS can use it as a VCamdroid or DroidCam source.

This repository is the iPhone app. An Android client and a Windows desktop client are still in the tree. The iPhone app is the one to use.

## What you can do

Open **OBS Studio** on the home screen. After camera access, the studio stays in the foreground and waits for OBS.

- **Live preview that matches the broadcast.** The picture on the phone is the same geometry OBS receives, with square pixels.
- **Front and back cameras.** The back camera uses the iPhone’s virtual camera so zoom can move across the lenses without restarting the shot. The front camera is a separate switch.
- **Zoom** from 1.0× to 10.0×.
- **Exposure.** Auto exposure with an EV slider, or manual exposure with ISO and shutter together.
- **White balance** presets: Auto, Daylight, Cloudy, Fluorescent, and Incandescent.
- **Focus.** Continuous autofocus, or a manual focus pull from near to infinity.
- **Torch** on the back camera, and a **180° flip** for an upside-down mount.
- **Resolution and frame rate.** 1280×720 at 30 fps, and 1920×1080 at 30 fps. 1080p60 stays out of the picker until that phone has held it for a long session.
- **Bitrate** from 2.5 to 16 Mbps. The default is 4 Mbps, constant bitrate, H.264 Baseline, Rec.709, one keyframe per second.
- **Tally.** OBS can mark the phone PROGRAM, PREVIEW, or STANDBY. You can hide the badge in Settings.
- **Battery and session.** OBS can read battery percent and change resolution, frame rate, and lens while you are live.
- **Bonjour.** The phone advertises itself as a DroidCam OBS camera so OBS can find it on the local network. You can still type the IP and port by hand.
- **On-screen address.** The studio shows the IP, port (default **4747**), and whether the connected client is on Wi-Fi or USB.
- **Screen curtain.** A black overlay turns the OLED panel off while the app stays open, so a long show does not burn in the display. Curtain can be automatic, always on, or off. Tap the black screen to bring the controls back.
- **In-app log** for encoder errors, disconnects, and format changes.

## Connect to OBS

### Wi-Fi

1. Put the iPhone and the PC on the same network. 5 GHz is the better choice. Client isolation on the router will block the connection.
2. Allow local network access when the app asks. USB still works if you skip that prompt.
3. In OBS, add a **VCamdroid** or **DroidCam OBS** source pointed at the IP and port shown on the phone.

### USB

1. Use a data-capable USB-C cable. A charge-only cable will not carry the stream.
2. Install Apple Devices or iTunes on the PC, plug in the phone, and tap **Trust This Computer**.
3. Run `usbmux_forward` for port 4747 so the PC’s `127.0.0.1:4747` reaches the app.
4. In OBS, connect to `127.0.0.1` on port 4747.

USB is the steadier path when a cable is available. The phone is not a driverless webcam. OBS (or the existing DroidCam source) is still required.

## Keep a long show stable

Leave the app open and use the curtain. The side button, the lock screen, and switching away from the app stop the camera. Unlock the phone and open VCam iPhone again to resume.

Stay plugged in for a long session, and leave Low Power Mode off. If the phone gets hot, the studio warns you before frames start dropping.

There is no microphone stream yet. Audio requests from OBS get a not-found response.

## Project layout

| Path | What it is |
| --- | --- |
| `VCamdroid-main/ios/` | The iPhone app (Swift, AVFoundation, VideoToolbox). |
| `VCamdroid-main/docs/IOS_PRODUCT_SPEC.md` | Product and protocol contract for the iPhone build. |
| `VCamdroid-main/android/` | The earlier Android client. Kept as the reference for the wire format. |
| `VCamdroid-main/windows/` | Windows client and the usbmux forwarder used for USB. |
