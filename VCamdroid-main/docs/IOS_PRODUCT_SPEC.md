# VCamdroid for iPhone — Product and Engineering Spec

**Status:** Handoff for the iOS team. Decisions below are the agreed product direction as of 15 October 2026.
**Reference phone:** iPhone 16 Pro (USB-C, USB 3 up to 10 Gbps, OLED, virtual multi-camera).
**Companion platforms that must keep working unchanged:** the existing Windows desktop client, the VCamdroid OBS plugin, and legacy DroidCam OBS sources.

This document is the contract for a **separate Swift project**. Do not port Kotlin, Camera2, MediaCodec, RootEncoder, or Android services into Swift. Clone the wire protocols. Replace the capture and encode stack with AVFoundation and VideoToolbox.

The Android tree remains the source of truth for byte layouts. If this spec and the code disagree, the code wins, and this spec should be corrected.

---

## 1. Product goal

Ship an iPhone app that turns the phone into the same broadcast camera the Android app already is:

1. **OBS mode (required first).** The phone is a DroidCam-compatible streaming server. OBS on Windows connects over Wi-Fi or USB and receives H.264 (and later HEVC) with the existing tally and session controls.
2. **Windows camera.** VCamPanel on the PC is the only video client of that same port 4747 stream. It decodes once and publishes a Windows camera named **VCamdroid**. NVIDIA Broadcast, OBS, Zoom, Teams, and other apps select that camera. Windows 11 is required for the camera to appear. The direct DroidCam OBS source remains an optional client, and it cannot share the video socket with VCamPanel.

Optimize for **sustained broadcast stability** on iPhone 16 Pro: no dropped frames, no UI hitch, no stretched pixels, hours of streaming while plugged in. Experimental features stay off until they meet that bar. That is already the Android project's rule, and it carries over.

### Non-negotiable standards

- The phone UI stays responsive while encoding. A feature that causes stutter, exposure flicker, or thermal collapse is disabled by default or cut.
- Preview and stream share one geometry. Square pixels. No stretched faces.
- The OBS wire format is compatible with the plugin and with DroidCam OBS on port 4747. Do not invent a second video protocol for v1.
- HDR and Display P3 stay off on the encoded stream. OBS and DroidCam expect SDR 8-bit Rec.709.

---

## 2. What exists today

VCamdroid is two programs that share no UI code:

| Piece | Role |
| --- | --- |
| Android app (`android/`) | Camera, encode, and network server or client. minSdk 28, targetSdk 35. |
| Windows app (`windows/`) | TCP control server, FFmpeg decode, softcam DirectShow filter. |
| OBS plugin (separate repo) | Preferred OBS source. Also accepts a plain DroidCam OBS source. |

The phone is the capture device. Windows and OBS are the receivers. An iPhone build is a third capture client beside Android, not a rewrite of the PC.

### 2.1 Pipeline A — OBS / DroidCam (production path)

This is the path to clone first.

```
Camera2 (TEMPLATE_RECORD)
  → encoder Surface (MediaCodec, COLOR_FormatSurface, CBR, latency 0)
  → Annex-B NAL units
  → DroidCamServer TCP :4747
  → OBS (VCamdroid plugin or DroidCam OBS source)

Optional third surface, only if AI or gestures are on:
  ImageReader 640×480 YUV → ML Kit face + MediaPipe gestures → SCALER_CROP_REGION
```

Owners in the Android tree:

| Type | File | Job |
| --- | --- | --- |
| `StreamingService` | `service/StreamingService.kt` | Foreground camera service, wake locks, session observers, tally, client address. |
| `DroidCamStreamer` | `droidcam/DroidCamStreamer.kt` | Camera2 session, manual controls, encoder, AI crop. |
| `DroidCamServer` | `droidcam/DroidCamServer.kt` | HTTP control plane and the video socket. |
| `DroidCamPacketFramer` | `droidcam/DroidCamPacketFramer.kt` | 12-byte packet header. |
| `DroidCamSettings` | `droidcam/DroidCamSettings.kt` | Persisted studio settings. |
| `MdnsAdvertiser` | `droidcam/MdnsAdvertiser.kt` | Bonjour-style discovery. |
| `CameraCapabilityProbe` | `capabilities/CameraCapabilityProbe.kt` | HAL capability JSON for `/v1/capabilities`. |
| `PowerSaveManager` | `util/PowerSaveManager.kt` | Partial wake lock, low-latency Wi-Fi lock, screen on/off. |
| `AiTrackerEngine` | `ai/AiTrackerEngine.kt` | Face tracking and SmoothDamp crop. Off by default. |
| `GestureDetectorHelper` | `ai/GestureDetectorHelper.kt` | MediaPipe palm toggle and pinch zoom. Off by default. |

Audio is **not implemented**. `GET /v2/audio` returns HTTP 404. Do not treat the README's AAC mention as a shipped feature.

USB on this path is **not** a USB Video Class webcam. The phone listens on TCP 4747. The PC runs:

```text
adb forward tcp:4747 tcp:4747
```

OBS then connects to `127.0.0.1:4747`. The video bytes are the same DroidCam packets, carried over the cable instead of Wi-Fi.

Screen-off on Android is real: a camera foreground service, a partial wake lock (12 hour cap), and a low-latency Wi-Fi lock keep encoding after the power button. The preview surface is dropped while the encoder surface stays. That behavior does **not** exist for a third-party iOS app. See section 6.

### 2.2 Pipeline B — Windows virtual webcam (second path)

```
Phone TCP client → Windows server :6969
  phone sends DeviceDescriptor
  Windows sends PacketType control bytes

Phone RTSP server :8554 (Pedro RootEncoder / RtspServerCamera2, OpenGL filters)
  → Windows FFmpeg demux and decode
  → softcam.dll DirectShow
  → Zoom / Teams / Discord / Meet
```

USB for this path is also ADB, implemented in `windows/src/adb.h` and started from `windows/src/net/server.cpp`:

- `adb reverse tcp:6969 tcp:6969` so the phone can connect to the PC server through the cable.
- `adb forward tcp:8554 tcp:8554` so the PC can pull RTSP from the phone.
- Wi-Fi pairing is a QR code shown by the desktop app. The phone scans it and connects to the PC's IP on port 6969.

OpenGL filters (brightness, contrast, sepia, glitch, and the rest) exist only on this path, via RootEncoder. They are not part of the OBS encoder path.

### 2.3 Ports

| Port | Direction | Used by |
| --- | --- | --- |
| 4747 | PC connects to phone | OBS / DroidCam HTTP + video. Default, user-configurable on Android. |
| 6969 | Phone connects to PC | Desktop control channel. |
| 8554 | PC connects to phone | RTSP (`rtsp://<phone>:8554/live`). |

mDNS service type for OBS discovery: `_droidcamobs._tcp.` on port 4747. Service name pattern: `VCamdroid (<model>)`.

---

## 3. Decision: separate Swift app

A separate iOS project is the right build. Shared code with Android is the protocol, written down and tested, not a shared camera module.

Suggested module split inside the Swift app:

```
SessionStore          UserDefaults. Studio settings. Mirrors DroidCamSettings.
CaptureEngine         AVCaptureSession. Formats, zoom, exposure, focus, white balance, torch.
Encoder               VTCompressionSession. Annex-B NALs. No B-frames.
DroidCamServer        NWListener on 4747. HTTP routes + video socket. Same bytes as Android.
Discovery             NetService / NWListener advertisement of _droidcamobs._tcp.
ScreenCurtain         Black overlay, brightness 0, idle timer disabled. Foreground only.
UsbTransport          Not in the iOS app. VCamPanel forwards port 4747 over usbmux.
VisionTracker         Vision / Neural Engine. Must not be a capture output that can stall frames.
```

The iPhone does not add a port 6969 client or an RTSP server. Android's `VCamdroid.exe` still uses those.

The Windows OBS plugin and `VCamdroid.exe` stay the Android receivers. The iPhone's Windows client is VCamPanel plus `VCamSource.dll`. See section 5.

---

## 4. Protocol contract — OBS mode (v1 must match)

Implement these routes on a TCP listener with `TCP_NODELAY` (Nagle off). Android uses a 256 KB send buffer. Match that intent: do not let the socket coalesce video packets.

Read the request the way Android does: one read into a 2048-byte buffer, interpret the first line as US-ASCII. This is not a general HTTP stack. Keep that simplicity so behavior matches odd DroidCam clients.

### 4.1 HTTP routes

| Request | Response |
| --- | --- |
| `GET /ping` | `HTTP/1.1 200 OK`, body `OK`, `Content-Length: 2`, `Connection: close`. |
| `GET /resolutions`, `GET /probe`, `GET /v1/resolutions` | JSON. Android shape: `{"back":["WxH@fpsfps",...],"front":[...],"device":"<manufacturer> <model>","cameras":N,"fullReport":true}`. Close after write. |
| `GET /capabilities`, `GET /v1/capabilities`, `GET /hal` | Full capability JSON from the probe. Close after write. |
| `GET /v1/session` | Session JSON below. Close after write. |
| `PUT /v1/session` or `POST /v1/session` | Query: `width`/`w`, `height`/`h`, `fps`, `format`/`fmt`, `lens`/`camera`. A `WIDTHxHEIGHT` token in the path is also accepted. Apply the change, then return session JSON plus `"success": true/false`. |
| `GET /battery` | Body is the integer percent, nothing else. |
| `PUT /v1/tally/<program\|preview\|idle>/` | Update tally. `200` with `Content-Length: 0`. |
| Path contains `/video` | Hijack this socket. It becomes the video stream. Do not close it after the HTTP headers. |
| `GET /v2/audio` | `404` with empty body until audio is an explicit later milestone. Do not half-implement it. |
| Anything else | `200` with `Content-Length: 0` and `Connection: close`. Android does this for unknown GETs. Preserve it. |

Session JSON fields the plugin already reads:

```json
{
  "status": "waiting|streaming",
  "streaming": true,
  "width": 1920,
  "height": 1080,
  "fps": 30,
  "format": "avc",
  "lens": "back",
  "battery": 80,
  "tally": "idle"
}
```

`PUT /v1/session` adds `"success": true` or `false` to that object.

Video request parsing (`DroidCamServer.handleVideoStreamRequest`):

- Format is `hevc` if the line contains `hevc` or `h265` (case insensitive). Otherwise `avc`.
- Resolution is the first `(\d{3,4})x(\d{3,4})` match.
- Else keywords: `4k`/`2160p` → 3840×2160, `1440p` → 2560×1440, `1080p` → 1920×1080, `720p` → 1280×720, `480p` → 640×480.
- Else the phone's saved target resolution (default 1920×1080).

On video start: close any previous video socket, wrap the new socket in a large buffered stream, notify "client connected", start encode. Then block reading the socket until the client disconnects, and tear the encoder down.

USB detection on Android treats a peer address of loopback / ADB-forward as USB. Do the same: if the remote address is `127.0.0.1` (or the usbmux forward presents as loopback), mark the client as USB in the HUD.

### 4.2 Video packet framing

Every video write is one packet. Header is 12 bytes, then payload. Multi-byte integers are big-endian.

| Offset | Size | Meaning |
| --- | --- | --- |
| 0 | 8 | PTS in milliseconds, unsigned 64-bit. Use `0xFFFFFFFFFFFFFFFF` (`NO_PTS`, written as signed `-1`) for codec config (SPS/PPS or VPS/SPS/PPS). |
| 8 | 4 | Payload length in bytes, unsigned 32-bit. |
| 12 | N | Annex-B NAL bytes, or the codec-config blob. |

Rules copied from `DroidCamPacketFramer` and the MediaCodec callback:

- Codec config buffers (`BUFFER_FLAG_CODEC_CONFIG`) are sent with `NO_PTS` via `sendConfigPacket`.
- Other buffers with `size > 0` use `presentationTimeUs / 1000` as the PTS.
- Flush after each packet. Android flushes per packet on purpose so a client sees a frame boundary immediately.
- Do not send empty buffers.
- One active video socket. A new `/video` request replaces the old one.

### 4.3 Encoder bitstream requirements

These are compatibility requirements, not preferences. DroidCam and the OBS plugin expect low-latency Annex-B. VideoToolbox's defaults will break that.

| Setting | Required value |
| --- | --- |
| Codec v1 | H.264, `avc` in the session JSON. |
| Profile | Baseline or Constrained Baseline. Not High. |
| B-frames | Off. `kVTCompressionPropertyKey_AllowFrameReordering = false`. |
| Bitrate mode | Constant. Set average bit rate and data-rate limits so it cannot burst like unconstrained VBR. Android uses `BITRATE_MODE_CBR`. |
| Real-time | `kVTCompressionPropertyKey_RealTime = true`. |
| Keyframe interval | 1 second (Android default `keyFrameIntervalSec = 1`). |
| Frame rate | Locked to the selected fps. Set expected frame rate on the encoder and lock `activeVideoMinFrameDuration` equal to `activeVideoMaxFrameDuration` so auto-exposure cannot float the rate. |
| Color | Rec.709, 8-bit, legal range. HDR off. Display P3 off. |
| Output | Annex-B with start codes, not AVCC length prefixes, not a fragmented MP4. |
| Resolution change | Stop session, reconfigure, send a new codec-config packet before the next frame. Android restarts the stream for resolution changes. |
| Default bitrate | 4000 kbps. Android studio range in the README is about 2.5–16 Mbps. |
| Default fps | 30. Supported targets to aim for: 24, 30, 48, 60. Ship 30 and 60 first on 16 Pro. |
| Default size | 1920×1080. |

HEVC (`format=hevc`) is a fast-follow, not v1. When added: Main profile, still no B-frames, still Annex-B, VPS/SPS/PPS in the config packet.

### 4.4 Capability and resolutions JSON

Android's `/v1/capabilities` is a dump of Camera2 characteristics (facing, hardware level, logical multi-camera, stream sizes, fps ranges, zoom, ISO, focus). iOS should return the same top-level idea from `AVCaptureDevice.formats`, not fake Camera2 fields the phone does not have.

Minimum the OBS client needs from `/resolutions`:

- `back`: list of strings like `1920x1080@60fps`
- `front`: same
- `device`: human-readable name, for example `Apple iPhone 16 Pro`
- `cameras`: count
- `fullReport`: `true`

Only advertise formats the capture-plus-encode path has actually been timed on that model. A format that opens and then drops frames is worse than omitting it. See section 8 for the 16 Pro allowlist.

### 4.5 Discovery

Advertise `_droidcamobs._tcp.` on the session port (default 4747), name `VCamdroid (iPhone 16 Pro)` or the device marketing name. TXT record `name` = that same string, matching `MdnsAdvertiser`.

iOS will show a Local Network permission prompt. The UI must explain it before the prompt, and the app must surface a clear failure if the user denies it. Discovery is optional for a manual IP connection. Do not block manual IP entry on Bonjour.

---

## 5. USB on iPhone — required, and how it actually works

USB streaming **is in scope**. iPhone 16 Pro can carry this stream over the cable. Several shipping webcam apps do it (Camo, EpocCam, Iriun, iVCam). The mechanism is a TCP tunnel, which is the same idea as this project's ADB forward.

### 5.1 What it is

The iPhone app still listens on TCP 4747 inside the app. It does not become a USB gadget.

A Windows helper, using Apple's usbmux protocol (the channel iTunes / Finder / the Apple Devices app already use after "Trust This Computer"), maps:

```text
PC 127.0.0.1:4747  →  iPhone app port 4747
```

OBS keeps connecting to `127.0.0.1:4747` only on the optional direct-source path. The normal Windows client is VCamPanel, which opens that same address itself. The DroidCam bytes do not change. `adb.h` stays for Android. The usbmux forward lives in VCamPanel. `usbmux_forward.exe` is an optional console helper.

Practical requirements for the user:

- A data-capable USB-C cable. Charge-only cables will fail.
- iPhone 16 Pro supports USB 3 (up to 10 Gbps) with a USB 3 cable. The cable in the box is often USB 2. USB 2 is still enough: the stream is 2.5–16 Mbps, and USB 2 carries hundreds of megabits. Do not block the feature on a USB 3 cable. Prefer USB 3 when present.
- Apple Mobile Device Support installed on Windows (iTunes, or the Apple Devices app).
- The user has tapped Trust on the phone.
- The VCamdroid iOS app is in the foreground (see section 6).

### 5.2 What it is not

- It is not a generic UVC webcam. The iPhone does not appear in the camera list by itself. VCamPanel has to be running, and Windows 11 has to load the VCamdroid camera that VCamPanel registers.
- It is not Continuity Camera. Continuity Camera is Apple's Mac-only system feature. It does not feed this app, and this app does not run on macOS.
- It does not replace Wi-Fi. Wi-Fi to the phone's LAN IP on port 4747 remains the other supported path.

### 5.3 Windows work item

v1 iOS is not done until a Windows machine can run the equivalent of `adb forward tcp:4747 tcp:4747` against a paired iPhone and the existing OBS source connects to `127.0.0.1:4747` with no plugin changes.

Implement the forwarder with a maintained usbmux client (libimobiledevice's `iproxy` is the reference behavior). Ship it beside the existing `adb.exe` folder rather than overloading ADB. Log success and failure the same way `adb.h` logs `[ADB:<port>]`.

Failure copy for the user should name the three usual causes: cable is not data, Trust was not accepted, Apple Mobile Device Service is not running.

### 5.4 iPhone desktop camera

The iPhone does not use pipeline B. There is no RTSP server, no port 6969 client, and no DirectShow softcam on this path. `VCamdroid.exe` stays the Android desktop client and is left alone.

VCamPanel is the video client:

1. It opens the existing tunnel (USB wins when a phone is plugged in; otherwise it uses the last Wi-Fi address).
2. It sends `GET /video?1920x1080&client=panel` and reads the 12-byte DroidCam packets. The phone writes no HTTP body before those packets.
3. It decodes H.264 with the Windows Media Foundation decoder and publishes NV12 frames on a shared mapping.
4. `VCamSource.dll`, loaded by the Windows Frame Server, exposes one camera named **VCamdroid**: 1920×1080 NV12 at 30 fps. Apps select that camera. VCamPanel has to keep running; with no producer the camera repeats black frames and stays in the list.

`client=panel` selects a separate encode policy. A DroidCam OBS client stays on H.264 Baseline and the 2500–16000 kbps slider, with each second clamped at that average. The panel client is 1920×1080 at 30 fps, H.264 High, real-time, no frame reordering, no frame delay, Rec.709, 30 Mbps when the client address is loopback and 12 Mbps on Wi-Fi, with a one-second peak of twice that average. Those rates are not written through `StudioSettings`, which still clamps the saved slider. The desk shows the rate actually in use.

The phone allows one video socket. If a DroidCam OBS source connects while VCamPanel holds it, the picture moves and VCamPanel waits for Connect. It does not reconnect in a loop. Short desk requests stay on their own connections.

Windows 10 keeps the desk and the optional OBS source. VCamPanel does not take the video socket there, and it says the camera list needs Windows 11.

---

## 6. Screen, battery, and heat

### 6.1 The limit

Pressing the iPhone side button, or leaving the app, stops the camera. `AVCaptureSession` is interrupted with "video device not available in the background." There is no public background mode that means "keep encoding with the display powered down."

The multitasking-camera entitlement (`com.apple.developer.avfoundation.multitasking-camera-access`) is for VoIP apps in Picture in Picture. Do not plan on Apple granting it for a PC webcam, and do not pretend a silent audio background mode will keep the camera alive. App Review rejects that pattern, and the capture session still stops.

Continuity Camera can run with the panel off. That path is not available to this app.

Locking the phone is the equivalent of a lens cap. Say that in the UI when the session drops because the app resigned active.

### 6.2 What to build instead: Screen Curtain

This is the behavior Camo, EpocCam, and this Android app's black overlay already use. The app stays foreground. The panel looks off.

- Disable the idle timer for the whole streaming session so auto-lock does not fire.
- On the curtain action, show a true-black full-screen view (OLED pixels off) and set screen brightness to 0.
- Remember the previous brightness and restore it when the user taps to wake the HUD.
- Curtain is available while the rear camera is streaming. Default it on after a short idle, with an explicit toggle, matching Camo's Automatic / On / Off. Automatic is the right default.
- Tapping the black view restores the HUD. Do not require a force-quit to get the controls back.
- The status bar area can stay black. Avoid a bright preview, a clock, or a glowing tally on the curtain. A tiny dim tally is acceptable only if the user opted into "show tally" and even then prefer off during curtain to save the panel.
- Guided Access / Single App Mode is a user setup hint for a dedicated webcam phone, not an app feature we implement. Mention it in help text only.

### 6.3 What the curtain does and does not save

On iPhone 16 Pro OLED:

- A black curtain removes almost all display power and reduces burn-in and a bit of heat. Do it.
- The camera, the encoder, and USB or Wi-Fi stay on. That is the correct cost of a stable picture. The curtain does not throttle the encode.
- While plugged in over USB-C, charging covers the battery. The limit on a long 1080p60 session is heat, not percentage.
- Low Power Mode must be detected and the user warned. It will hitch the encode. Do not silently enable extra work while it is on.

Android's 12-hour wake lock and `WIFI_MODE_FULL_LOW_LATENCY` have no iOS equivalent. Do not promise "power button lock and the stream continues." The help text should say: leave the app open, use the curtain, keep the phone plugged in for long shows.

---

## 7. Camera controls — Android behavior and the iOS mapping

Implement controls on `AVCaptureDevice` under a lock. Reconfigure the session only when the format, fps, or lens identity actually changes. Zoom, exposure, focus, white balance, and torch should apply without a session restart.

| Control | Android today | iPhone 16 Pro implementation | v1 |
| --- | --- | --- | --- |
| Lens | Reopen Camera2 front/back. | Prefer the virtual multi-camera (`builtInTripleCamera` / `builtInDualWideCamera` when present) so zoom can cross ultra-wide, wide, and tele without reopening. Front camera is a separate device switch. | Yes. Front and back. Ultra-wide and tele may be zoom positions on the virtual device rather than separate buttons. |
| Zoom | `CONTROL_ZOOM_RATIO`, plus optional `SCALER_CROP_REGION` when AI is on. HUD range is about 1.0–10.0, clamped to the device. | `videoZoomFactor` from 1.0 to `maxAvailableVideoZoomFactor`, ramp for slider changes. Do not add a Metal crop in v1. | Yes. |
| ISO | Manual: AE off, `SENSOR_SENSITIVITY` plus last exposure time. Auto: AE on. Range from the HAL, often 100–6400. | `setExposureModeCustom(duration:iso:)`. Custom exposure sets ISO and shutter together. You cannot pin ISO and let shutter float the way Camera2 does. In manual mode, drive both from the slider pair (ISO + shutter) or lock a target ISO and compute shutter from a chosen EV. Label the UI honestly. | Yes, as explicit manual exposure, not a fake "ISO only" if shutter must move with it. |
| EV | `CONTROL_AE_EXPOSURE_COMPENSATION` in steps. | `exposureTargetBias` within `minExposureTargetBias`…`maxExposureTargetBias`. | Yes, in auto exposure. |
| AE lock | `CONTROL_AE_LOCK`. | `exposureMode = .locked` after adjusting, or set bias then lock per AVFoundation rules. Custom exposure and auto-bias are different modes. Switching modes must be explicit in the HUD. | Yes. |
| Focus | Manual: AF off, `LENS_FOCUS_DISTANCE` in diopters. Auto: `CONTINUOUS_VIDEO`. Tap to focus uses a metering rectangle. | `focusMode = .locked` and `lensPosition` 0…1 (not diopters). Auto: `.continuousAutoFocus`. Tap to focus: `focusPointOfInterest` when `isFocusPointOfInterestSupported`. The HUD scale is 0…1, macro to infinity, matching the Android labels even though the unit changed. | Yes. |
| White balance | Presets plus manual kelvin approximated with RGGB gains. | Use temperature and tint (`AVCaptureDevice.WhiteBalanceTemperatureAndTintValues`). This is cleaner than the Android kelvin-to-RGGB hack. Presets: Auto, Daylight ~5200 K, Cloudy, Fluorescent, Incandescent. Store last kelvin (Android default 5200). | Yes. |
| Torch | `FLASH_MODE_TORCH` if `FLASH_INFO_AVAILABLE`. | `torchMode` when `isTorchAvailable`. Torch and front camera: hide the control when unsupported. | Yes. |
| Anti-flicker 50/60 | `CONTROL_AE_ANTIBANDING_MODE`. | No reliable public 50/60 switch on all formats. Omit the control rather than showing a switch that does nothing. ISP handles a lot of this. | No. |
| 180° flip | `SCALER_ROTATE_AND_CROP` when the HAL supports it, else a preview transform. Android also has separate horizontal and vertical flips. | `AVCaptureConnection.videoRotationAngle` for 180, and mirror flags if a connection supports them. Default off. | Yes. |
| Stabilization | Exposed on the RTSP path. | `preferredVideoStabilizationMode`. Cinematic / standard stabilization adds latency. Default **off** for a low-latency webcam. Offer it later as an explicit "stable, higher latency" toggle. | Off. |
| FPS | AE target range plus `SENSOR_FRAME_DURATION`. | Lock min and max frame duration to `1/fps`. | Yes. |
| Bitrate | CBR on the encoder, default 4000 kbps. | VideoToolbox average bit rate + data rate limits. | Yes. |

Preview must be aspect-correct (the Android `AutoFitTextureView` rule). The on-screen preview shows the same crop the encoder sends. If the connection is mirrored for the front camera selfie look, the encoded stream and the preview must match, and the HUD should say when the broadcast is mirrored.

Tap-to-edit numeric entry for ISO, zoom, EV, and focus is part of the Android studio HUD. Bring that pattern to iOS. Sliders should be continuous, not snapped to a handful of stops.

---

## 8. Performance targets — iPhone 16 Pro

Test on a 16 Pro before enabling a mode in the picker. Advertise only modes that hold the target for at least 30 minutes while plugged in, curtain on, screen otherwise idle, room temperature, Low Power Mode off.

| Mode | v1 target | Notes |
| --- | --- | --- |
| 1920×1080 @ 30, H.264, 4–8 Mbps | Required. Default. | Must be boringly stable over USB and over 5 GHz Wi-Fi. |
| 1920×1080 @ 60, H.264, 8–16 Mbps | Required on 16 Pro if the format lock holds. | Lock frame duration. Watch thermals after 30 minutes. |
| 1280×720 @ 30 | Required as a fallback. | |
| 3840×2160 @ 30 | Not v1. | Add only after 1080p60 is clean. HEVC preferred when the plugin accepts it. |
| 4K60 | Out of scope until measured. | Likely too hot combined with a preview. |
| HEVC 1080p60 | Fast-follow. | Better bitrate, but only after the plugin path is verified. H.264 remains the default so existing OBS sources work. |

Pipeline rules:

- One capture pipeline. Preview and encoder share the camera's native buffers. Do not insert a CPU pixel copy on the encode path.
- v1 encode path is capture → VideoToolbox, zero extra GPU pass. Metal crops, filters, and LUTs are forbidden on this path until a later milestone, because they add a copy and heat.
- Drop late frames. Never queue a backlog. If the encoder falls behind, drop, do not accumulate latency.
- Inference, logging, and HUD updates stay off the camera queue.
- Do not run face detection, hand detection, or a third video output in v1.

Wi-Fi guidance in the UI: same LAN, 5 GHz, no client isolation. USB is the recommended path when the cable is available, same as Android.

---

## 9. Feature phases

### v1 — reliable set (build this, and stop)

iOS app:

- Foreground studio UI: preview, zoom, torch, EV, manual exposure (ISO + shutter), white balance, focus, front/back, flip, fps, bitrate, resolution.
- Screen curtain and disabled idle timer.
- `DroidCamServer` on 4747 with the routes and packet layout in section 4.
- H.264 Baseline, no B-frames, Rec.709, 1080p30 and 1080p60 (60 where the 16 Pro allowlist passed).
- Bonjour `_droidcamobs._tcp.`
- On-screen IP and port. Manual connection does not depend on Bonjour.
- Tally badge from `PUT /v1/tally/...` (`program`, `preview`, `idle`).
- Battery in `/v1/session` and `/battery`.
- Session updates from `PUT /v1/session` (resolution, fps, format, lens) applied live.
- Local Network permission UX.

Windows:

- usbmux forward of 4747 so existing OBS sources use `127.0.0.1` the same way they use ADB today.

Explicitly not in v1: locked-screen streaming, UVC gadget mode, HEVC, 4K, AI tracking, gestures, audio, RTSP, OpenGL/Metal filter pack, desktop QR mode, antibanding switch.

### v1.1

- HEVC Main, no B-frames, gated on a plugin test.
- 4K30 only for formats that passed the 30-minute test.
- Lens labels for ultra-wide and tele if the virtual device exposes them cleanly.

### v2 — vision, after encode has been stable in real use

- Face tracking on the Neural Engine (Vision), 10–15 Hz, never as an `AVCaptureVideoDataOutput` that can block the encoder.
- Electronic pan/tilt/zoom via `videoZoomFactor` and a smoothed center, or a single Metal pass into VideoToolbox if zoom-factor alone cannot frame the shot. Metal is allowed here only because v1 already shipped without it.
- Physics: keep the Android intent (SmoothDamp, dead zone, no AE flicker). Do not port the Camera2 crop rectangle literally.
- Hand gestures (palm toggle, pinch zoom) via Vision hand pose. Off by default.
- Person segmentation is allowed if it is cheaper and steadier than a face box. It is still off by default.

Android's tracker is experimental on purpose: a 640×480 `ImageReader` plus crop updates can starve the camera and flicker exposure. The iPhone version should avoid that class of bug by keeping inference off the capture deadline. If it is not stable, it stays off.

### v3 — iPhone system camera

Shipped as the Windows client of the port 4747 stream, not as an RTSP port of pipeline B:

- VCamPanel decodes that stream once.
- `VCamSource.dll` is the Media Foundation camera named VCamdroid on Windows 11.
- `client=panel` uses the High-profile transport rates in section 5.4. Legacy OBS clients stay on Baseline and the saved slider.

The Android RTSP / DirectShow path in section 2.2 and section 10 is unchanged and is not the iPhone camera.

### v4 — audio, only with a confirmed framing spec

Android returns 404 for `GET /v2/audio`. Implement AAC only after the OBS plugin's expected ADTS (or other) layout is written down and tested. `AVAudioEngine` is the iOS source. Do not design a new audio protocol.

---

## 10. Protocol contract — Android desktop client

This is the Android `VCamdroid.exe` contract (pipeline B). The iPhone camera does not use it. The iPhone desktop path is the port 4747 client in section 5.4.

### 10.1 Device descriptor (phone → PC, big-endian)

From `DeviceDescriptor.serialize()`:

1. `uint16` byte length, then UTF-8 device name.
2. `uint16` byte length, then UTF-8 RTSP URL (`rtsp://<ip>:8554/live`).
3. `uint16` count of **front** resolutions, then that many `uint16 width`, `uint16 height`.
4. `uint16` count of **back** resolutions, then that many `uint16 width`, `uint16 height`.
5. `uint16` filter count, then for each filter: `uint16` name length, UTF-8 name, `uint8` category ordinal.

Category ordinals from `FilterRepository.Category`: `0 NONE`, `1 CORRECTION`, `2 EFFECT`, `3 DISTORTION`, `4 ARTISTIC`.

If v3 ships with no artistic filters, send an honest short list (at least `None`). Do not advertise filter names the phone will ignore.

The Windows header comment in `devicedescriptor.h` is older and incomplete (it omits filters and swaps some wording). The Android serializer and the Windows deserializer in `serializer.cpp` are the source of truth. Verify against `serializer.cpp` before shipping v3.

### 10.2 Control packets (PC → phone)

First byte is `PacketType`:

| Byte | Name |
| --- | --- |
| `0x00` | FRAME |
| `0x01` | RESOLUTION |
| `0x02` | ACTIVATION |
| `0x03` | CAMERA |
| `0x04` | QUALITY |
| `0x05` | CORRECTION_FILTER |
| `0x06` | EFFECT_FILTER |
| `0x07` | ROTATION |
| `0x08` | BITRATE |
| `0x09` | ADAPTIVE_BITRATE |
| `0x0A` | STABILIZATION |
| `0x0B` | FLASH |
| `0x0C` | FOCUS |
| `0x0D` | CODEC |
| `0x0E` | FPS |
| `0x0F` | ZOOM |
| `0x10` | FLIP |

`ACTIVATION` is handled inside the connection layer and is not forwarded to the streamer. The initial `StreamOptions` blob is big-endian and parsed in `StreamOptions.deserialize` (fps, width, height, camera, adaptive bitrate, bitrates, stabilization, flash, h265, focus, then filter records). Match that layout if desktop mode sends a full options snapshot.

### 10.3 Error reports (phone → PC)

`uint8` severity (`0` warning, `1` error), then a `uint16`-length UTF-8 title, then a `uint16`-length UTF-8 description. Big-endian.

---

## 11. iOS improvements worth taking

These are reasons to write Swift rather than only copying Android limits.

- **One camera family.** Ship a tested allowlist instead of a Camera2 HAL probe that has to survive every vendor. Capability JSON still exists so OBS can ask, but the contents come from formats you have timed.
- **VideoToolbox.** H.264 and HEVC are always hardware. Behavior is consistent across 16 Pro units in a way MediaCodec is not across Android phones.
- **Virtual multi-camera zoom.** Ultra-wide → wide → tele without tearing down the session. Android lens switches reopen the camera. Use this for zoom. It is the reliable version of "pro lenses."
- **White balance in temperature and tint.** Better color control than the Android RGGB approximation.
- **Rec.709 lock.** A visible quality win in OBS. Do it in v1.
- **Neural Engine later.** Face and hands can run beside the camera instead of as an extra camera surface. That is the fix for the Android ePTZ failure mode, and it is why tracking is v2 rather than v1.
- **USB 3 headroom on 16 Pro.** The tunnel is not bandwidth-starved at webcam bitrates even on a USB 2 cable. USB 3 is extra margin, not a requirement.

Do not chase these for v1: ProRAW, Log, cinematic mode, LiDAR, Desk View, Center Stage as a system effect (you cannot embed Center Stage into the DroidCam elementary stream), dual simultaneous cameras, HDR streaming.

---

## 12. What iOS does not get

State these in user-facing help so support does not treat them as bugs.

| Android capability | iPhone result |
| --- | --- |
| Power-button screen-off streaming via a foreground camera service | Not available. Curtain while the app is open is the product. |
| `adb forward` with no Apple driver | Replaced by usbmux plus Apple Mobile Device Support and Trust This Computer. |
| Phone appears as a driverless USB webcam | Not available. VCamPanel publishes a camera named VCamdroid on Windows 11. The phone itself is not a USB video device. |
| 12-hour CPU and Wi-Fi power-save override | Not available. Stay plugged in. Keep Low Power Mode off. |
| `SCALER_CROP_REGION` inside the ISP before a zero-copy encoder | Not available. v1 uses `videoZoomFactor` only. A Metal crop is a later, explicit cost. |
| Manual ISO with a fully independent auto shutter | Not the same API. Expose ISO and shutter together in manual mode. |
| 50/60 Hz antibanding switch | Omit. |
| RootEncoder filter pack | Not in v1. Optional and reduced in v3. |
| Sideload an APK | TestFlight or App Store. Paid Apple Developer account. Local Network permission is mandatory to explain. |
| Stream continues after force-quit | It will not. |

---

## 13. App structure and UX

Match the Android information architecture, not its layouts pixel for pixel.

1. **Home.** One action into the studio. Short "how to connect" for Wi-Fi and USB. Show the usbmux trust steps, not ADB commands. The Windows camera is VCamPanel, not a second pairing screen.
2. **OBS studio.** Live preview, IP:port, USB vs Wi-Fi badge, tally, curtain button, studio controls, settings for resolution, fps, bitrate, codec (H.264 only until HEVC is enabled).
3. **Settings.** Curtain behavior, keep-awake (idle timer), confirm before stop, show tally. No antibanding row.
4. **Permissions, in order, with rationale screens:** Camera, Microphone only when audio exists, Local Network.

Persist the same concepts as `DroidCamSettings`: resolution, fps, bitrate, keyframe interval, last zoom, last EV, last white balance, last focus mode, curtain, discovery, tally visibility, port. Use `UserDefaults`. Defaults: 1920×1080, 30 fps, 4000 kbps, 1 second GOP, zoom 1.0, EV 0, auto WB, continuous AF, curtain automatic, discovery on, tally on, port 4747.

The encode session and the HTTP server should live in types that are not owned by a SwiftUI view. A view disappearing for the curtain must not tear down the camera. Follow the Android split: UI observes a session object, the session owns capture, encoder, and server.

Log to an in-app log screen (Android has `LogActivity`). Include encoder errors, session interruptions, usbmux-relevant "client connected from 127.0.0.1", and format renegotiation. This will be the main support tool.

---

## 14. Interruptions the session must survive or report

Handle `AVCaptureSession` interruptions explicitly:

| Cause | Behavior |
| --- | --- |
| App resigned active / screen locked | Stop sending video, tell the HUD "stream paused — unlock and open VCamdroid". Do not crash the TCP server. A client may disconnect; that is acceptable. Resume cleanly when active again, including a new codec-config packet. |
| Phone call or another app took the camera | Same pause. Resume if the system says the interruption ended. |
| Thermal pressure | Drop fps or resolution along the allowlist before dropping frames silently. Surface a tally or HUD warning. Never keep a mode that is already missing frames. |
| USB cable pull | The video socket dies. Return to "waiting for OBS". Camera preview can stay up. |
| Format change from the OBS session PUT | Reconfigure, send codec config, then frames. The first decoded frame after the change is what the plugin uses to resize. |

---

## 15. Distribution and permissions

- Apple Developer Program account. TestFlight for the crew, App Store when the v1 checklist passes.
- Usage strings: camera for the broadcast preview and stream, local network for OBS on the same network, and microphone only once audio ships.
- Background modes: none in v1. Do not add `audio` or `voip` to keep the camera alive.
- The Windows usbmux helper needs a short install note: install Apple Devices (or iTunes), trust the phone, then connect. Do not require a full Xcode install for end users.

App Review will see a camera app that opens a listening socket. The review notes should say: local network only, video to the user's own OBS or desktop app, no account, no cloud upload.

---

## 16. Acceptance tests for v1

Run these on an iPhone 16 Pro and a Windows PC with the current OBS plugin. Android behavior is the oracle for packet compatibility.

1. Wi-Fi: phone and PC on the same 5 GHz LAN. OBS source to the phone IP port 4747. Picture appears, tally follows program/preview/idle, unplugging Wi-Fi returns the phone to waiting.
2. USB: usbmux forward 4747. OBS source to `127.0.0.1:4747`. Same picture. Pull the cable. Phone returns to waiting without a crash.
3. Bitstream: H.264, no B-frames, Annex-B, SPS/PPS arrives as a `NO_PTS` packet before picture data. GOP about 1 second. Rec.709. A known-good DroidCam OBS source plays it without a custom build.
4. 1080p30 for 30 minutes, curtain on, USB, plugged in. No growing latency, no session restart, UI still opens when the curtain is tapped.
5. 1080p60 for 30 minutes under the same conditions, or the mode is removed from the picker.
6. `PUT /v1/session` changes lens and fps. Phone HUD and encoder follow. OBS sees the new frame size on the first new frame.
7. Manual exposure, WB temperature, focus position, zoom, and torch change the stream without a reconnect.
8. Lock the phone with the side button. Stream stops. Help text matches section 6. Unlocking and opening the app resumes.
9. Deny Local Network. Manual IP still works on USB. Wi-Fi discovery fails with a visible explanation.
10. Front camera and back camera both maintain square pixels in preview and in OBS.

---

## 17. Source map (Android and Windows)

Use these when checking a behavior this spec summarizes.

| Concern | Where |
| --- | --- |
| OBS HTTP and video socket | `android/.../droidcam/DroidCamServer.kt` |
| Packet header | `android/.../droidcam/DroidCamPacketFramer.kt` |
| Camera2, encoder, manual controls, AI surface | `android/.../droidcam/DroidCamStreamer.kt` |
| Foreground service and tally observers | `android/.../service/StreamingService.kt` |
| Settings defaults | `android/.../droidcam/DroidCamSettings.kt` |
| mDNS | `android/.../droidcam/MdnsAdvertiser.kt` |
| Wake locks and screen broadcast | `android/.../util/PowerSaveManager.kt` |
| Face crop | `android/.../ai/AiTrackerEngine.kt` |
| Gestures | `android/.../ai/GestureDetectorHelper.kt` |
| Capability JSON | `android/.../capabilities/CameraCapabilityProbe.kt` |
| Desktop TCP client | `android/.../networking/ConnectionManager.kt` |
| Descriptor and error bytes | `android/.../networking/DeviceDescriptor.kt`, `ErrorReport.kt` |
| Control opcodes | `android/.../networking/PacketType.kt` |
| RTSP server and filters | `android/.../rtsp/Streamer.kt`, `StreamOptions.kt`, `video/filters/FilterRepository.kt` |
| ADB forward and reverse | `windows/src/adb.h`, called from `windows/src/net/server.cpp` |
| Desktop server port 6969 | `windows/src/application.cpp`, `windows/src/net/server.cpp` |

---

## 18. Summary for planning

Build a Swift capture app and a small Windows usbmux forwarder. Keep OBS and the DroidCam packet format. Ship 1080p30/60 H.264, studio controls, Wi-Fi, USB, and a black screen curtain on iPhone 16 Pro.

Do not spend v1 on locked-screen streaming, a UVC driver, AI tracking, filters, RTSP, or audio. Those are either impossible on iOS, or they are later milestones that must not compromise the encode path.
