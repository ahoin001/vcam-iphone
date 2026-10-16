# VCam iPhone

Use an iPhone as a Windows camera named **VCamdroid**. NVIDIA Broadcast, OBS, Zoom, Teams, Meet, and Discord select that camera.

The phone app is named **VCamdroid**. It previews the shot, encodes H.264, and listens on port **4747**. The Windows program is **VCamPanel**. It connects over Wi-Fi or USB, decodes the picture once, and publishes the camera. Leave VCamPanel running while you use the picture. Shot controls (zoom, exposure, white balance, focus, torch, flip, portrait) live in that same window. OBS is optional.

Windows 11 is required for **VCamdroid** to appear in the camera list. On Windows 10, VCamPanel still finds the phone and opens the desk, and its status says the camera list needs Windows 11. The optional DroidCam OBS source is the picture path on Windows 10.

Do the sections in order. Each one ends with a check so you know it worked before you continue.

## What you need

| Piece | Requirement |
| --- | --- |
| iPhone | iOS 26 or later. A physical phone. The iOS Simulator cannot be the camera. |
| Mac | Used once, to install the app onto the iPhone with Xcode. |
| Windows PC | 64-bit Windows 11 for the VCamdroid camera. Windows 10 can open the desk and use the optional OBS source. |
| Network or cable | The same Wi-Fi network for a wireless connection, or a data-capable USB-C cable for USB. A charge-only cable carries power only and will not carry video. |
| Apple ID | Signed in to Xcode so the app can be installed on your phone. A free Apple ID works. |

You do this on two computers:

1. The **Mac** installs VCamdroid onto the iPhone.
2. The **Windows PC** runs VCamPanel and uses the camera.

## What to download

Download these before you start. Skip an item only when its “Needed for” line does not match the connection you will use.

| Download | Where | Needed for |
| --- | --- | --- |
| **Xcode** | Mac App Store. Open it once after install so it can finish installing components. You need the version that includes the iOS 26 SDK. | Putting the app on the iPhone. |
| **This repository** | On the Mac: [github.com/ahoin001/vcam-iphone](https://github.com/ahoin001/vcam-iphone). Clone it or download the ZIP. | The iPhone app only. You do not need the repo on the PC if you use the Windows companion release below. |
| **Windows companion** | On the Windows PC: this repo’s **[latest release](https://github.com/ahoin001/vcam-iphone/releases/latest)** → download **VCamdroid-Windows-x64.zip**, or the direct link [VCamdroid-Windows-x64.zip](https://github.com/ahoin001/vcam-iphone/releases/latest/download/VCamdroid-Windows-x64.zip). Unzip it and keep `VCamPanel.exe` and `VCamSource.dll` in the same folder. Do not use an older Android-only Windows zip from another VCamdroid fork. | Connecting the iPhone and publishing the VCamdroid camera. Required for USB and for Wi-Fi. |
| **Apple Devices** (or iTunes) | Microsoft Store, on the Windows PC. Open it once after install. | USB only. It starts Apple Mobile Device Service, which the cable path needs. Wi-Fi does not need it. |
| **WebView2 Runtime** | [Microsoft Edge WebView2](https://developer.microsoft.com/microsoft-edge/webview2/). | The companion window. Current Windows 11 already includes it. |
| **OBS Studio** | On the Windows PC: [obsproject.com/download](https://obsproject.com/download). | Optional. Use it when you want the iPhone inside OBS. Add a **Video Capture Device** and choose **VCamdroid**. |
| **DroidCam OBS** | [dev47apps.com/obs](https://www.dev47apps.com/obs/), then quit and reopen OBS. That page currently asks for OBS 32 or newer. | Optional direct source, in section 7. OBS does not include that source by itself. Skip it when you select the VCamdroid camera. |

Clone on the Mac, if you use git:

```bash
git clone https://github.com/ahoin001/vcam-iphone.git
```

The project folder after that is `vcam-iphone`. The iPhone project is `vcam-iphone/VCamdroid-main/ios/VCamdroid.xcodeproj`.

## 1. Put the app on the iPhone

Do this on the Mac, with the iPhone in your hand.

1. Install Xcode from the Mac App Store.
2. Open Xcode once and let it finish installing extra components. Sign in with your Apple ID when it asks: **Xcode → Settings → Accounts → +**.
3. Clone the repository, or download the ZIP from GitHub and unzip it.
4. In Finder, open `VCamdroid-main/ios/VCamdroid.xcodeproj`. Xcode opens.
5. Plug the iPhone into the Mac with a data cable. Unlock the phone. If it asks **Trust This Computer**, tap **Trust** and enter the phone passcode.
6. At the top of the Xcode window, open the run-destination menu (it may say a simulator name). Choose your iPhone. Leave every simulator unselected.
7. In the left sidebar, click the blue **VCamdroid** project, then the **VCamdroid** target under **TARGETS**.
8. Open the **Signing & Capabilities** tab.
9. Check **Automatically manage signing**.
10. In **Team**, choose your Apple ID. It is listed as a Personal Team if you do not have a paid developer account.
11. If Xcode says the bundle id is already registered, change **Bundle Identifier** from `com.darusc.vcamdroid.ios` to something only you use, such as `com.yourname.vcamdroid`.
12. Press the **Run** button (the play triangle), or press **Command-R**.
13. Wait until the app installs and opens on the phone. The first build can take several minutes.
14. If the phone says the developer is not trusted, stay on the phone and open **Settings → General → VPN & Device Management**. Select your Apple ID and tap **Trust**, then open **VCamdroid** again.
15. On the **Camera access** screen, tap **Continue**, then tap **Allow** on the iOS prompt.
16. On the **Local network** screen, tap **Continue** if you will use Wi-Fi. Tap **Not now** if you will use only the USB cable.

**Check:** VCamdroid is on the iPhone home screen and opens to a screen titled **VCamdroid** with a button labeled **OBS Studio**.

A free Apple ID install expires after about 7 days. When the phone refuses to open the app, plug it into the Mac and press **Run** in Xcode again. A paid Apple Developer account removes that 7-day limit.

You can unplug the phone from the Mac after this. The Mac is finished unless you need to reinstall the app.

## 2. Install the companion on the PC

Do this on the Windows PC.

1. From this repo’s **[latest release](https://github.com/ahoin001/vcam-iphone/releases/latest)**, download **[VCamdroid-Windows-x64.zip](https://github.com/ahoin001/vcam-iphone/releases/latest/download/VCamdroid-Windows-x64.zip)**. Unzip it somewhere you will keep, such as `Documents\VCamdroid`.
2. Confirm `VCamPanel.exe` and `VCamSource.dll` are in that folder. The camera does not register if the DLL is somewhere else.
3. Run `VCamPanel.exe`. The window stays on top. The first launch registers the **VCamdroid** camera for your Windows user. That does not need an administrator install, and the camera remains after a reboot.
4. If the status says the camera list needs Windows 11, this PC cannot publish VCamdroid. The desk still works. Use section 7 for the picture.

**Check:** VCamPanel is open. On Windows 11 it does not report that the camera is missing. On Windows 10 it says the camera list needs Windows 11.

To build the companion yourself instead of downloading it, open `VCamdroid-main/windows/VCamdroid.sln` in Visual Studio 2022 (**Desktop development with C++**), set **Release | x64**, and build **VCamSource** and **VCamPanel**. Copy `VCamSource.dll` next to `VCamPanel.exe`.

## 3. Connect over Wi-Fi

Use this when the phone and the PC are on the same Wi-Fi and you do not want the cable in the shot. Skip to section 4 if you want USB instead.

1. Join the iPhone and the Windows PC to the same Wi-Fi network. A 5 GHz network is the steadier choice.
2. On the router, turn off client isolation and AP isolation. With either one on, the PC cannot reach the phone.
3. Turn off any VPN on the phone and on the PC for the first connection.
4. On the iPhone, open **VCamdroid**.
5. Tap **OBS Studio**.
6. If iOS asks to find and connect to devices on the local network, tap **Allow**.
7. Leave this studio screen in front. The status reads **Waiting for the computer**. It also shows an address such as `192.168.1.20:4747`.
   - The number before the colon is the IP address. In the example, that is `192.168.1.20`.
   - The number after the colon is the port. The default is **4747**.
8. On the PC, in **VCamPanel**, click **Refresh**. Pick the Wi-Fi phone, or type the IP from the phone, then click **Connect**.

**Check:** On Windows 11, the phone changes from **Waiting for the computer** to **Streaming**, the badge reads **Wi-Fi**, and VCamPanel says to choose **VCamdroid** as the camera. On Windows 10 the phone stays on **Waiting for the computer** until section 7, and VCamPanel says the camera list needs Windows 11.

Keep the phone on that studio screen. The side button and the app switcher stop the camera.

## 4. Connect over USB

Use this when you want the cable instead of Wi-Fi. VCamPanel finds the iPhone and keeps the USB tunnel open.

1. On the PC, install **Apple Devices** or iTunes from the Microsoft Store and open it once.
2. Run `VCamPanel.exe` from the folder in section 2 if it is not already open.
3. Plug the iPhone into the PC with a data-capable USB-C cable. Unlock the phone and tap **Trust This Computer** if it asks.
4. On the iPhone, open **VCamdroid**, tap **OBS Studio**, and leave that screen in front. The status can say **Waiting for the computer**.
5. In **VCamPanel**, click **Refresh**. The iPhone should appear as **USB**. Select it and click **Connect**. USB wins when a phone is plugged in. The status line names the usual failures if it cannot: Apple Mobile Device Service is not running, the cable is not a data cable, Trust was not accepted, or VCamdroid is not in front.
6. Leave the companion open while you use the camera.

**Check:** On Windows 11, the phone says **Streaming**, the badge reads **USB**, and VCamPanel says to choose **VCamdroid** as the camera. On Windows 10 the phone stays on **Waiting for the computer** until section 7, and VCamPanel says the camera list needs Windows 11.

If you changed **Port** in the phone’s Settings, type that port in the companion’s address field before Connect.

The zip also includes `usbmux_forward.exe` if you want a console USB forwarder and no companion. You do not need it when `VCamPanel.exe` is connected over USB. That forwarder does not publish the VCamdroid camera.

## 5. Choose VCamdroid in an app

Finish section 3 or section 4 first. VCamPanel has to stay running. The phone stays on the studio screen.

1. Open the app that needs the camera.
2. Set the camera to **VCamdroid**:
   - **NVIDIA Broadcast:** Camera → **VCamdroid**. You can then select NVIDIA Broadcast in OBS or in a meeting app.
   - **OBS:** in **Sources**, click **+**, choose **Video Capture Device**, and set the device to **VCamdroid**.
   - **Zoom:** Settings → Video → Camera → **VCamdroid**.
   - **Microsoft Teams:** Settings → Devices → Camera → **VCamdroid**.
   - **Google Meet:** in the meeting, open the camera menu (the arrow next to the camera button) and choose **VCamdroid**. You can also set it under Settings → Video before you join.
   - **Discord:** User Settings → Voice & Video → Camera → **VCamdroid**.
3. If the camera is missing, reopen VCamPanel and read the status. A missing `VCamSource.dll`, or Windows 10, is the usual cause.

For a clean background, leave **Portrait** off on the phone. Do the cut on the PC: in **NVIDIA Broadcast**, set Camera to **VCamdroid**, then pick NVIDIA Broadcast in OBS or the meeting app. In **OBS**, add the **VCamdroid** Video Capture Device, then add a Background Removal (or similar) filter on that source. Phone Portrait is a light blur if you need it without those apps, and it warms the iPhone. Do not run phone Portrait and a PC background filter together, or the edges stack.

**Check:** the app shows the iPhone, and the phone still says **Streaming**.

Closing VCamPanel, or leaving the phone app, stops the live picture. The camera can remain in the list and show black until you connect again. The phone has no microphone in this build, so the meeting uses the computer’s microphone or another mic you select in that app.

## 6. Adjust the camera from the desk

The lower part of VCamPanel is the camera desk. Finish section 3 or section 4 first, then leave `VCamPanel.exe` connected.

On a wide window the preview fills the left side and the shot controls sit on the right. A narrow OBS dock still stacks them. **Show preview** is on after Connect so you can frame in the companion. Leave it off if the phone is warming up. The app you selected in section 5 is the live picture.

The desk matches the phone: front or back camera, zoom stops 1× / 2× / 3× / 5× / 10× and a 1.0×–10.0× slider, torch, flip, then one tool at a time for exposure (auto EV, or manual ISO and shutter), white balance, focus, and portrait. Moving a control on the computer moves it on the phone. Moving it on the phone updates the desk.

The Portrait tool on the desk says to leave Portrait off for meetings. NVIDIA Broadcast or OBS Background Removal on **VCamdroid** is the cleaner cut. Use the iPhone blur only when you do not have those PC tools.

The stream heading shows the rate actually in use. While VCamPanel is the video client, that is 1080p30 at **12 Mbps** on Wi-Fi and **30 Mbps** on USB. The bitrate slider under it stays on the saved rate for the optional OBS source in section 7 (2500–16000 kbps). Moving that slider does not change the VCamdroid picture.

**On Windows:**

1. Run `VCamPanel.exe` if it is not already open. Before Connect, the lower area shows a “Connect a phone” card with the preview space on the left.
2. Click **Refresh**. Pick the USB iPhone or a Wi-Fi phone from the list. If Wi-Fi discovery misses it, type the IP from the phone and Connect.
3. Leave the window on top while you shoot. **Copy** puts the optional OBS address on the clipboard.

**In OBS, on Windows,** you can also dock the same page. This is optional. The companion already shows the desk.

1. Open **Docks → Custom Browser Docks**.
2. Set the dock name to **VCamdroid**.
3. Set the URL to `http://` plus the address from the companion or the phone, plus `/panel`. USB: `http://127.0.0.1:4747/panel`. Wi-Fi example: `http://192.168.1.20:4747/panel`.
4. Click **Apply**. Leave the dock open while you shoot.

**Check:** moving Zoom in the companion changes the picture in the app that selected **VCamdroid**, and the zoom control on the phone moves with it.

## 7. Optional: a DroidCam OBS source

Use this when you want OBS to connect to port 4747 itself, instead of selecting the VCamdroid camera. This is the picture path on Windows 10. It is also available on Windows 11.

The phone allows one video connection. Connect in VCamPanel always takes that socket, including on Windows 10. Disconnect VCamPanel before you add this source. If the source connects first, VCamPanel retries a few times, then says the video connection closed. Click **Connect** after the source is gone.

1. Install [DroidCam OBS](https://www.dev47apps.com/obs/) and reopen OBS.
2. On Wi-Fi, leave VCamPanel disconnected. The source uses the IP on the phone and port **4747**.
3. On USB, click **Disconnect** in VCamPanel so it is not the video client. In the unzipped folder, run `usbmux_forward.exe` and leave that window open. The source uses **127.0.0.1** and port **4747**.
4. In OBS, click **+** in **Sources** and add **DroidCam OBS** (or **VCamdroid** if you already have that source). Enter the address from the step above. **Copy** in VCamPanel copies it when the companion is open. Click **OK**.

**Check:** the phone says **Streaming**, and the picture is in the OBS preview.

Leave phone **Portrait** off if you add Background Removal (or NVIDIA Broadcast after **Start Virtual Camera**) on this OBS path. The same stacking rule as section 5 applies.

Other apps still need a camera. With this optional source, that camera is OBS’s **Start Virtual Camera**, set to **OBS Virtual Camera** in the meeting app. The VCamdroid camera in section 5 is the path that does not need that step.

## While you are live

On the phone, the picture fills the screen and the address stays at the top. Tap the preview to focus and put the open control away. The bottom bar opens one tool at a time: switch camera, zoom (1×, 2×, 3×, 5×, 10×, or 1.0×–10.0×), exposure (auto EV, or manual ISO and shutter), white balance, focus, and portrait. Torch and Flip 180° toggle immediately. Moon and Settings sit together on the lower right, next to Stop. Tap the moon to dim the screen now. Long-press it for Automatic, Always dim, or Off. Log is in Settings.

Portrait on the phone is a light background blur. Leave it off when NVIDIA Broadcast or OBS is already cutting the background. If you turn it on, the iPhone preview stays off, and the phone tells you to watch the picture on the computer. **Show on iPhone**, or the **iPhone preview** switch in the portrait tool, draws the blurred picture on the phone. Leave it off if the phone warms up.

From the computer, section 6 is that same camera desk in `VCamPanel.exe`. While VCamPanel is connected, the picture stays 1080p30 at the Wi-Fi or USB rate above. Resolution and frame-rate changes wait until that client disconnects.

In **Settings** on the phone:

- **Curtain** is Automatic (dims after 8 seconds), Always dim, or Off. The moon still works when Off. Tap the black screen to get the controls back.
- **Show tally** follows a DroidCam OBS source when that source is the video client. The VCamdroid camera does not send tally.
- **Confirm before stopping**.
- **Port** is 4747 unless something else is using it.
- Resolution, frame rate, and bitrate under **For a DroidCam OBS source** are the saved rates for section 7. They do not change the VCamdroid picture while VCamPanel is connected.

For a long session, keep the phone plugged in, leave Low Power Mode off, and use the curtain. If the phone gets hot, the studio warns you. Pressing the side button pauses the stream. Unlock the phone and open **VCamdroid** again to continue.

## If the picture never appears

- The phone must stay on the studio screen. **Waiting for the computer** means the app is ready and VCamPanel has not opened the picture yet.
- Connect starts the picture even if the camera-list warning is showing. Disconnect stops it.
- VCamPanel must stay running. The picture goes black when it closes, even if **VCamdroid** is still listed.
- `VCamSource.dll` must sit in the same folder as `VCamPanel.exe`. If it is missing, the status says so.
- Windows 11 is required for the camera to appear in the list. The status names that, a missing DLL, or another registration failure. On Windows 10 use section 7.
- In the meeting app or OBS, the device name is **VCamdroid**.
- The phone allows one video connection. A DroidCam OBS source connected at the same time takes the picture. VCamPanel then says the video connection closed. Disconnect the source and click **Connect**.
- Wi-Fi: the PC and the phone are on the same network, and the IP in VCamPanel is exactly the address on the phone. A VPN on either device can hide them from each other. Client isolation on the router blocks the connection.
- USB: **VCamPanel** is still connected. Its status line names the usual failures: Apple Mobile Device Service is not running, the cable is not a data cable, Trust was not accepted, or the phone app is not in front.
- The optional OBS source address for USB is **127.0.0.1** port **4747**, with the tunnel connected.
- **That port is already in use** on the phone means something else has port 4747. Change **Port** in the phone’s Settings, then use the new port in VCamPanel and, if you use section 7, in OBS.
- **Camera access is off** means the iPhone blocked the camera. Turn it on in **Settings → Privacy & Security → Camera → VCamdroid**.
- Before Connect, **VCamPanel** should show a “Connect a phone” card in the lower area. If that area is empty black, the WebView2 Runtime is missing. Install it, then reopen the companion.
- After Connect, the camera desk is blank when the phone has left the studio screen, when the phone is not connected in **VCamPanel**, or when WebView2 failed to load the phone’s `/panel` page.
- Desk **Show preview** stays dark until the phone is on the studio screen and sending frames. The app that selected **VCamdroid** can still show the picture while desk preview is off.
