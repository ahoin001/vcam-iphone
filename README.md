# VCam iPhone

Use an iPhone as the camera in OBS Studio, then send that same picture to Zoom, Teams, Meet, or Discord through OBS’s virtual camera.

The phone app is named **VCamdroid**. It previews the shot, encodes H.264, and listens on port **4747**. OBS connects to that port. The iPhone does not show up by itself in another app’s camera list.

## What you need

| Piece | Requirement |
| --- | --- |
| iPhone | iOS 26 or later. A physical phone. The simulator cannot be the camera. |
| Mac | Used once, to install the app onto the iPhone with Xcode. |
| Windows PC | Windows 10 or 11, 64-bit. This is the machine that runs OBS. |
| Network or cable | The same Wi-Fi network for a wireless connection, or a data-capable USB-C cable for USB. A charge-only cable will not carry video. |
| Apple ID | Signed in to Xcode so the app can be installed on your phone. |

## What to download

1. **Xcode**, from the Mac App Store. You need the version that includes the iOS 26 SDK.
2. **This repository**, on the Mac: [github.com/ahoin001/vcam-iphone](https://github.com/ahoin001/vcam-iphone).
3. **OBS Studio** for Windows, from [obsproject.com/download](https://obsproject.com/download).
4. **The OBS source** that talks to the phone. Install one of these, then restart OBS:
   - [VCamdroid OBS plugin](https://github.com/ShadowPlague21/VCamdroid-obs-plugin/releases/tag/latest), or
   - [DroidCam OBS](https://www.dev47apps.com/obs/).
5. **Apple Devices** (or iTunes), only if you will use the USB cable. Install it from the Microsoft Store, then open it once so Apple Mobile Device Service is running.
6. **Visual Studio 2022** with the **Desktop development with C++** workload, only if you will use the USB cable. It builds `usbmux_forward`, the small program that carries the stream over the cable. Wi-Fi does not need it.

## 1. Put the app on the iPhone

Do this on the Mac.

1. Install Xcode and open it once so it can finish installing its components.
2. Clone the repository, or download the ZIP and unzip it.
3. Open `VCamdroid-main/ios/VCamdroid.xcodeproj`.
4. Connect the iPhone with a data cable. Unlock the phone and tap **Trust This Computer**.
5. In Xcode’s toolbar, set the run destination to your iPhone. Leave the simulator unselected.
6. Select the **VCamdroid** target, open **Signing & Capabilities**, and choose your Apple ID as the Team. If signing fails because the bundle id is already registered, change the bundle id to something you own, such as `com.yourname.vcamdroid`.
7. Press **Run**.
8. If the phone says the developer is not trusted, open **Settings → General → VPN & Device Management**, select your Apple ID, and tap **Trust**.
9. Open **VCamdroid** on the phone.
10. Tap **Continue** on the camera screen and allow camera access.
11. On the local-network screen, tap **Continue** if you will use Wi-Fi. USB still works if you tap **Not now**.

A free Apple ID install expires after about 7 days. Run the project from Xcode again when the phone refuses to open the app. A paid Apple Developer account removes that limit.

## 2. Install OBS on the PC

1. Download OBS Studio from [obsproject.com/download](https://obsproject.com/download) and run the installer.
2. Install the VCamdroid OBS plugin or DroidCam OBS, then quit and reopen OBS.
3. If Windows Firewall asks, allow OBS on **Private** networks.

## 3. Use the iPhone as the camera in OBS over Wi-Fi

1. Join the iPhone and the PC to the same Wi-Fi network. A 5 GHz network is the steadier choice. Turn off client isolation or AP isolation on the router, or the PC cannot reach the phone.
2. On the iPhone, open **VCamdroid** and tap **OBS Studio**.
3. Allow the local-network prompt if iOS shows it.
4. Leave the studio screen open. It says **Waiting for OBS** and shows an address such as `192.168.1.20:4747`. The number before the colon is the IP. The number after it is the port. The default port is **4747**.
5. In OBS, find the **Sources** panel and click **+**.
6. Choose **VCamdroid** or **DroidCam OBS**.
7. In the source properties, enter the IP from the phone and set the port to **4747**.
8. Confirm. The phone changes to **Streaming**, and the preview appears in OBS.

The badge on the phone reads **Wi-Fi** while OBS is connected this way. **PROGRAM**, **PREVIEW**, and **STANDBY** follow the OBS tally when **Show tally** is on in the phone’s Settings.

## 4. Use the iPhone as the camera in OBS over USB

Use this when you want the cable instead of Wi-Fi. Build the forwarder once, then leave it running whenever you stream.

1. On the PC, install **Apple Devices** or iTunes and open it once.
2. Install Visual Studio 2022 with **Desktop development with C++**.
3. Open `VCamdroid-main/windows/VCamdroid.sln`.
4. Set the configuration to **Release** and the platform to **x64**.
5. Build the **usbmux_forward** project. The program is `VCamdroid-main/windows/x64/Release/usbmux_forward.exe`.
6. Plug the iPhone into the PC with a data-capable USB-C cable. Unlock the phone and tap **Trust This Computer**.
7. On the iPhone, open **VCamdroid**, tap **OBS Studio**, and leave that screen in front. The side button and the app switcher stop the camera.
8. On the PC, open Command Prompt in the folder that contains `usbmux_forward.exe` and run:

   ```bat
   usbmux_forward.exe 4747
   ```

   Leave that window open. Closing it disconnects the cable path.
9. In OBS, add a **VCamdroid** or **DroidCam OBS** source.
10. Enter the IP **127.0.0.1** and the port **4747**.
11. Confirm. The phone says **Streaming**, and the connection badge reads **USB**.

`usbmux_forward.exe` with no arguments uses port 4747. If you change the port in the phone’s Settings, pass that same port to the program and to OBS.

## 5. Use the iPhone as a webcam in Zoom, Teams, Meet, or Discord

Those apps do not connect to the phone directly. OBS has to be receiving the picture first, and then OBS publishes a camera they can select.

1. Finish section 3 or section 4 so the iPhone picture is visible in the OBS preview.
2. In OBS, in the **Controls** dock, click **Start Virtual Camera**.
3. Open the other app and choose **OBS Virtual Camera** as the camera:
   - **Zoom:** Settings → Video → Camera.
   - **Teams:** Settings → Devices → Camera.
   - **Google Meet:** the camera menu in the call, or Settings → Video.
   - **Discord:** Settings → Voice & Video → Camera.
4. Leave OBS open, leave **VCamdroid** open on the studio screen, and leave `usbmux_forward.exe` running if you are on USB.

Stopping the virtual camera, closing OBS, or leaving the phone app stops the picture in the meeting.

## While you are live

On the studio screen you can change the shot without reconnecting OBS:

- **Back** and **Front** cameras.
- **Zoom**, from 1.0× to 10.0×.
- **Exposure.** Auto exposure uses the EV slider. Manual exposure sets ISO and shutter together.
- **White balance:** Auto, Daylight, Cloudy, Fluorescent, or Incandescent.
- **Focus.** Auto, or a manual pull from near to infinity.
- **Torch** and **Flip 180°** for an upside-down mount.

In **Settings** on the phone:

- Resolution is **1280×720** or **1920×1080**. Frame rate is **30**. 1080p60 appears only after that phone has been allowed to offer it.
- Bitrate runs from **2500** to **16000** kbps. The default is **4000**.
- **Curtain** can be Automatic, Always on, or Off. Tap **Curtain** for a black screen that keeps the stream going and saves the OLED panel. Tap the black screen to get the controls back.
- **Show tally** and **Confirm before stopping**.

The preview on the phone matches the picture OBS receives. There is no microphone in this build, so a meeting still needs the computer’s mic or another mic.

For a long session, keep the phone plugged in, leave Low Power Mode off, and use the curtain. If the phone gets hot, the studio warns you. Pressing the side button pauses the stream. Unlock the phone and open **VCamdroid** again to continue.

## If the picture never appears

- The phone must stay on the studio screen. **Waiting for OBS** means the app is ready and OBS has not connected yet.
- Wi-Fi: the PC and the phone are on the same network, and the IP in OBS is exactly the address on the phone. A VPN on either device can hide them from each other.
- USB: `usbmux_forward.exe` is still running. Its window names the usual failures: Apple Mobile Device Service is not running, the cable is not a data cable, Trust was not accepted, or the phone app is not in front.
- The OBS source address for USB is **127.0.0.1** port **4747**, with the forwarder running.
- **That port is already in use** on the phone means something else has port 4747. Change **Port** in the phone’s Settings, then use the new port in OBS and, for USB, in `usbmux_forward.exe`.
- **Camera access is off** means the iPhone blocked the camera. Turn it on in iOS Settings → Privacy & Security → Camera → VCamdroid.
