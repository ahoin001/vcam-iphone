# VCam iPhone

Use an iPhone as the camera in OBS Studio, then send that same picture to Zoom, Teams, Meet, or Discord through OBS’s virtual camera.

The phone app is named **VCamdroid**. It previews the shot, encodes H.264, and listens on port **4747**. OBS connects to that port for the picture. Other apps see that picture only after OBS is already receiving it and **Start Virtual Camera** is on. Live camera tools (zoom, exposure, white balance, focus, torch, flip, portrait, resolution, frame rate, bitrate) live in the Windows companion, **VCamPanel**, or in an OBS Custom Browser Dock pointed at the phone’s `/panel` page. The stream stays connected.

Do the sections in order. Each one ends with a check so you know it worked before you continue.

## What you need

| Piece | Requirement |
| --- | --- |
| iPhone | iOS 26 or later. A physical phone. The iOS Simulator cannot be the camera. |
| Mac | Used once, to install the app onto the iPhone with Xcode. |
| Windows PC | Windows 10 or 11, 64-bit. This is the machine that runs OBS. |
| Network or cable | The same Wi-Fi network for a wireless connection, or a data-capable USB-C cable for USB. A charge-only cable carries power only and will not carry video. |
| Apple ID | Signed in to Xcode so the app can be installed on your phone. A free Apple ID works. |

You do this on two computers:

1. The **Mac** installs VCamdroid onto the iPhone.
2. The **Windows PC** runs OBS and receives the picture.

## What to download

Download these before you start. Skip an item only when its “Needed for” line does not match the connection you will use.

| Download | Where | Needed for |
| --- | --- | --- |
| **Xcode** | Mac App Store. Open it once after install so it can finish installing components. You need the version that includes the iOS 26 SDK. | Putting the app on the iPhone. |
| **This repository** | On the Mac: [github.com/ahoin001/vcam-iphone](https://github.com/ahoin001/vcam-iphone). Clone it or download the ZIP. | The iPhone app only. You do not need the repo on the PC if you use the Windows companion release below. |
| **Windows companion** | On the Windows PC: [latest release](https://github.com/ahoin001/vcam-iphone/releases/latest) → download **VCamdroid-Windows-x64.zip**, or the direct link [VCamdroid-Windows-x64.zip](https://github.com/ahoin001/vcam-iphone/releases/latest/download/VCamdroid-Windows-x64.zip). Unzip and keep `VCamPanel.exe`. | Finds the iPhone over USB or Wi-Fi and hosts the camera desk. Required for USB. Useful on Wi-Fi when you want the desk outside OBS. |
| **OBS Studio** | On the Windows PC: [obsproject.com/download](https://obsproject.com/download). | Seeing the iPhone picture, and publishing it as a webcam. |
| **An OBS source for the phone** | Install **one** of these on the Windows PC, then quit and reopen OBS. [VCamdroid OBS plugin](https://github.com/ShadowPlague21/VCamdroid-obs-plugin/releases/tag/latest), or [DroidCam OBS](https://www.dev47apps.com/obs/). The DroidCam OBS page currently asks for OBS 32 or newer. | The **VCamdroid** or **DroidCam OBS** item in OBS’s source list. OBS does not include that source by itself. |
| **Apple Devices** (or iTunes) | Microsoft Store, on the Windows PC. Open it once after install. | USB only. It starts Apple Mobile Device Service, which the cable path needs. Wi-Fi does not need it. |
| **WebView2 Runtime** | [Microsoft Edge WebView2](https://developer.microsoft.com/microsoft-edge/webview2/). | The companion window. Current Windows 11 already includes it. |

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

## 2. Install OBS and the phone source on the PC

Do this on the Windows PC.

1. Download OBS Studio from [obsproject.com/download](https://obsproject.com/download).
2. Run the installer and accept the default options.
3. Install one phone source, then fully quit OBS if it was open:
   - **VCamdroid OBS plugin:** open the [latest release](https://github.com/ShadowPlague21/VCamdroid-obs-plugin/releases/tag/latest), download the Windows installer or archive from that page, and run its installer.
   - **DroidCam OBS:** open [dev47apps.com/obs](https://www.dev47apps.com/obs/), download the Windows **DroidCam OBS Plugin**, and run that installer. Use this when the VCamdroid plugin release is missing a Windows build.
4. Open OBS.
5. If Windows Firewall asks, allow OBS on **Private** networks.
6. In the **Sources** panel, click **+**. Confirm that **VCamdroid** or **DroidCam OBS** is in the list. Click **Cancel** so you do not add a source yet.

**Check:** OBS is open, and the **+** menu in **Sources** contains **VCamdroid** or **DroidCam OBS**.

## 3. Use the iPhone as the camera in OBS over Wi-Fi

Use this when the phone and the PC are on the same Wi-Fi and you do not want the cable in the shot. Skip to section 4 if you want USB instead.

1. Join the iPhone and the Windows PC to the same Wi-Fi network. A 5 GHz network is the steadier choice.
2. On the router, turn off client isolation and AP isolation. With either one on, the PC cannot reach the phone.
3. Turn off any VPN on the phone and on the PC for the first connection.
4. On the iPhone, open **VCamdroid**.
5. Tap **OBS Studio**.
6. If iOS asks to find and connect to devices on the local network, tap **Allow**.
7. Leave this studio screen in front. The status reads **Waiting for OBS**. It also shows an address such as `192.168.1.20:4747`.
   - The number before the colon is the IP address. In the example, that is `192.168.1.20`.
   - The number after the colon is the port. The default is **4747**.
8. On the PC, in OBS, click **+** in the **Sources** panel.
9. Choose **VCamdroid** if it is listed. Otherwise choose **DroidCam OBS**.
10. Name the source if OBS asks, then click **OK**.
11. In the source properties, enter the IP from the phone and set the port to **4747**. If the dialog has a Wi-Fi / USB choice, choose Wi-Fi.
12. Click **OK**.

**Check:** the phone status changes from **Waiting for OBS** to **Streaming**, the badge reads **Wi-Fi**, and the iPhone picture is in the OBS preview.

OBS only needs this IP and port for the picture. For the camera desk on Wi-Fi, open **VCamPanel**, Refresh, pick the Wi-Fi phone (or type the IP), and Connect — or use section 6’s OBS dock. You do not need USB for that.

Keep the phone on that studio screen. The side button and the app switcher stop the camera. **PROGRAM**, **PREVIEW**, and **STANDBY** follow the OBS tally when **Show tally** is on in the phone’s Settings.

## 4. Use the iPhone as the camera in OBS over USB

Use this when you want the cable instead of Wi-Fi. The Windows companion finds the iPhone and keeps the USB tunnel open. OBS only receives the picture.

1. On the PC, install **Apple Devices** or iTunes from the Microsoft Store and open it once.
2. Download **[VCamdroid-Windows-x64.zip](https://github.com/ahoin001/vcam-iphone/releases/latest/download/VCamdroid-Windows-x64.zip)** from the [latest release](https://github.com/ahoin001/vcam-iphone/releases/latest). Unzip it. Run `VCamPanel.exe` from that folder. The window stays on top.
3. Plug the iPhone into the PC with a data-capable USB-C cable. Unlock the phone and tap **Trust This Computer** if it asks.
4. On the iPhone, open **VCamdroid**, tap **OBS Studio**, and leave that screen in front. The status can say **Waiting for OBS**.
5. In **VCamPanel**, click **Refresh**. The iPhone should appear as **USB**. Select it and click **Connect**. The status line names the usual failures if it cannot: Apple Mobile Device Service is not running, the cable is not a data cable, Trust was not accepted, or VCamdroid is not in front.
6. Click **Copy** for the OBS address. For USB that is **127.0.0.1:4747**.
7. In OBS, click **+** in **Sources** and add a **VCamdroid** or **DroidCam OBS** source.
8. Enter the IP **127.0.0.1** and the port **4747**. If the dialog has a USB choice, you can select that instead of typing the address.
9. Click **OK**. Leave the companion open while you stream.

**Check:** the phone says **Streaming**, the badge reads **USB**, and the picture is in the OBS preview.

If you changed **Port** in the phone’s Settings, type that port in the companion’s address field before Connect, and use the same port in OBS.

The zip also includes `usbmux_forward.exe` if you want a console USB forwarder and no companion. You do not need it when `VCamPanel.exe` is connected over USB.

To build the companion yourself instead of downloading it, open `VCamdroid-main/windows/VCamdroid.sln` in Visual Studio 2022 (**Desktop development with C++**), set **Release | x64**, and build the **VCamPanel** project.

## 5. Use the iPhone as a webcam in Zoom, Teams, Meet, or Discord

Those apps do not connect to the phone. They connect to a camera that OBS publishes. Finish section 3 or section 4 first, so the iPhone picture is already visible in the OBS preview.

1. Leave **VCamdroid** open on the phone’s studio screen.
2. Leave OBS open with the phone source showing a picture.
3. If you used USB, leave **VCamPanel** connected.
4. In OBS, find the **Controls** dock. It is usually at the lower right.
5. Click **Start Virtual Camera**. The button label changes to **Stop Virtual Camera** while it is on.
6. Open the meeting app and set the camera to **OBS Virtual Camera**:
   - **Zoom:** Settings → Video → Camera → **OBS Virtual Camera**.
   - **Microsoft Teams:** Settings → Devices → Camera → **OBS Virtual Camera**.
   - **Google Meet:** in the meeting, open the camera menu (the arrow next to the camera button) and choose **OBS Virtual Camera**. You can also set it under Settings → Video before you join.
   - **Discord:** User Settings → Voice & Video → Camera → **OBS Virtual Camera**.
7. Start or join the call. The other people see the OBS preview, which is the iPhone.

**Check:** the meeting preview shows the same picture as OBS, and the phone still says **Streaming**.

Stopping the virtual camera, closing OBS, or leaving the phone app stops the picture in the meeting. The phone has no microphone in this build, so the meeting uses the computer’s microphone or another mic you select in that app.

## 6. Adjust the camera from the desk

The Windows companion is the camera desk. OBS stays the picture. Finish section 3 or section 4 first, so the iPhone is in OBS, then leave `VCamPanel.exe` connected.

The desk matches the phone: front or back camera, zoom stops 1× / 2× / 3× / 5× / 10× and a 1.0×–10.0× slider, torch, flip, then one tool at a time for exposure (auto EV, or manual ISO and shutter), white balance, focus, and portrait. Stream settings on the desk are resolution, frame rate, and bitrate. Moving a control on the computer moves it on the phone. Moving it on the phone updates the desk.

**On Windows:**

1. Run `VCamPanel.exe` from the release zip (section 4, step 2) if it is not already open.
2. Click **Refresh**. Pick the USB iPhone or a Wi-Fi phone from the list. If Wi-Fi discovery misses it, type the IP from the phone and Connect.
3. Leave the window on top while you shoot. **Copy** puts the OBS address on the clipboard.

The meeting app still uses **OBS Virtual Camera**. `VCamPanel.exe` only finds the phone and changes the shot. It does not replace OBS.

**In OBS, on Windows or Mac,** you can also dock the same page:

1. Open **Docks → Custom Browser Docks**.
2. Set the dock name to **VCamdroid**.
3. Set the URL to `http://` plus the OBS address from the companion or the phone, plus `/panel`. USB: `http://127.0.0.1:4747/panel`. Wi-Fi example: `http://192.168.1.20:4747/panel`.
4. Click **Apply**. Leave the dock open while you shoot.

This dock is separate from the **Controls** dock that holds **Start Virtual Camera**. On a Mac, use the OBS dock, or open that same page in a browser.

**Check:** moving Zoom in the companion or in the dock changes the OBS preview, and the zoom control on the phone moves with it.

## While you are live

On the phone, the picture fills the screen and the OBS address stays at the top. Tap the preview to focus and put the open control away. The bottom bar opens one tool at a time: switch camera, zoom (1×, 2×, 3×, 5×, 10×, or 1.0×–10.0×), exposure (auto EV, or manual ISO and shutter), white balance, focus, and portrait. Torch and Flip 180° toggle immediately. Curtain, Settings, and Log are on the lower left. Stop is on the lower right.

Portrait blurs the background in the stream. The iPhone preview stays off for that, and the phone tells you to watch the picture in OBS. **Show on iPhone**, or the **iPhone preview** switch in the portrait tool, draws the blurred picture on the phone. Leave it off if the phone warms up.

From the computer, section 6 is that same camera desk in `VCamPanel.exe`, including resolution, frame rate, and bitrate. The OBS source stays the picture.

Resolution, frame rate, and bitrate are also on the desk in section 6. In **Settings** on the phone:

- Resolution is **1280×720** or **1920×1080**. Frame rate is **30**. 1080p60 appears only after that phone has been allowed to offer it.
- Bitrate runs from **2500** to **16000** kbps. The default is **4000**.
- **Curtain** can be Automatic, Always on, or Off. Tap **Curtain** for a black screen that keeps the stream going and saves the OLED panel. Tap the black screen to get the controls back. Curtain stays on the phone; it is not on the PC desk.
- **Show tally** and **Confirm before stopping**.

With the iPhone preview on, the phone matches the picture OBS receives. With portrait’s preview left off, OBS is the picture to watch.

For a long session, keep the phone plugged in, leave Low Power Mode off, and use the curtain. If the phone gets hot, the studio warns you. Pressing the side button pauses the stream. Unlock the phone and open **VCamdroid** again to continue.

## If the picture never appears

- The phone must stay on the studio screen. **Waiting for OBS** means the app is ready and OBS has not connected yet.
- Wi-Fi: the PC and the phone are on the same network, and the IP in OBS is exactly the address on the phone. A VPN on either device can hide them from each other. Client isolation on the router blocks the connection.
- USB: **VCamPanel** is still connected. Its status line names the usual failures: Apple Mobile Device Service is not running, the cable is not a data cable, Trust was not accepted, or the phone app is not in front.
- The OBS source address for USB is **127.0.0.1** port **4747**, with the companion connected.
- **That port is already in use** on the phone means something else has port 4747. Change **Port** in the phone’s Settings, then use the new port in OBS and in the companion.
- **Camera access is off** means the iPhone blocked the camera. Turn it on in **Settings → Privacy & Security → Camera → VCamdroid**.
- The meeting app shows a black camera or your old webcam when **OBS Virtual Camera** is not the selected camera, or when **Start Virtual Camera** is off in OBS.
- The camera desk is blank when the phone has left the studio screen, when USB is not connected in **VCamPanel**, or when the WebView2 Runtime is missing. `VCamPanel.exe` needs that runtime.
