# Future considerations

Work that is not part of the Windows camera client. The current path is the port 4747 H.264 stream, decoded once in VCamPanel, published as the VCamdroid camera on Windows 11.

## Picture

- **HEVC.** Smaller streams at the same quality, after a decoder and a camera type exist for it. The panel client is H.264 High today.
- **1080p60.** The panel client locks 30 fps so the camera type stays one format. A 60 fps camera needs a second media type and a phone path that is allowed to stay at 60.
- **4K.** Needs a capture mode that has been timed, a larger frame bus, and a camera type above 1920×1080.
- **MJPEG or uncompressed video on USB 3.** A way to skip H.264 when the cable has the bandwidth. The phone is still a TCP app on port 4747, not a USB video device.
- **A user bitrate override on the system-camera path.** The panel client uses 30 Mbps on USB and 12 Mbps on Wi-Fi, with a one-second peak of twice that average. The desk slider remains the rate for a DroidCam OBS client.

## Windows

- **A Windows 10 virtual camera, or a signed kernel driver.** Windows 10 keeps the desk and the optional DroidCam OBS source. `MFCreateVirtualCamera` needs Windows 11. A kernel camera would be a different product, with signing and an administrator install.
- **OBS tally while VCamdroid, rather than the DroidCam source, is the video client.** Tally today arrives on the DroidCam source connection. The system camera does not send it.
- **Starting the companion with Windows.** VCamPanel has to be open for the picture. Launching it at sign-in is a separate setup choice.

## Other clients

- **The phone microphone.** `GET /v2/audio` is still unused. Meeting apps use the computer's microphone.
- **A Mac client.** This camera is a Windows Media Foundation source. A Mac build would be a different capture path.
- **Multiple phones.** One VCamPanel connection and one camera name. More than one phone needs more than one camera.

## Rejected

- **The iPhone cannot be a driverless USB webcam.** iOS does not let this app appear as a USB video device. USB is a usbmux tunnel to port 4747, and VCamPanel publishes the camera.
- **Continuity Camera is Mac-only.** It is Apple's system feature for a Mac. It does not register a camera on Windows, and it does not use this app's encoder.
