# VCam iPhone

Use an iPhone as a Windows camera named **VCamdroid**. NVIDIA Broadcast, OBS, Zoom, Teams, Meet, and Discord select that camera.

The phone app is named **VCamdroid**. It previews the shot, encodes H.264, and listens on port **4747**. The Windows program is **VCamPanel**. It connects over Wi-Fi or USB, decodes the picture once, and publishes the camera. Leave VCamPanel running while you use the picture. Shot controls (zoom, exposure, white balance, focus, torch, flip, portrait) live in that same window. OBS is optional.

Windows 11 is required for **VCamdroid** to appear in the camera list. On Windows 10, VCamPanel still finds the phone and opens the desk, and its **This PC** card says this PC is older than Windows 11, so other apps cannot select VCamdroid. The optional DroidCam OBS source is the picture path on Windows 10.

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
| **Windows companion** | On the Windows PC: this repo’s **[latest release](https://github.com/ahoin001/vcam-iphone/releases/latest)** → download **VCamdroid-Windows-x64.zip**, or the direct link [VCamdroid-Windows-x64.zip](https://github.com/ahoin001/vcam-iphone/releases/latest/download/VCamdroid-Windows-x64.zip). Unzip it and keep `VCamPanel.exe` and `VCamSource.dll` in the same folder. Do not use an older Android-only Windows zip from another VCamdroid fork. Later updates install from **Setup** in VCamPanel. | Connecting the iPhone and publishing the VCamdroid camera. Required for USB and for Wi-Fi. |
| **Apple Devices** (or iTunes) | Microsoft Store, on the Windows PC. Open it once after install. | USB only. It starts Apple Mobile Device Service, which the cable path needs. Wi-Fi does not need it. |
| **WebView2 Runtime** | [Microsoft Edge WebView2](https://developer.microsoft.com/microsoft-edge/webview2/). | The companion window. Current Windows 11 already includes it. |
| **OBS Studio** | On the Windows PC: [obsproject.com/download](https://obsproject.com/download). | Optional. Use it when you want the iPhone inside OBS. Add a **Video Capture Device** and choose **VCamdroid**. |
| **DroidCam OBS** | [dev47apps.com/obs](https://www.dev47apps.com/obs/), then quit and reopen OBS. That page currently asks for OBS 32 or newer. | Optional direct source, in section 7. OBS does not include that source by itself. Skip it when you select the VCamdroid camera. |

Clone on the Mac, if you use git:

```bash
git clone https://github.com/ahoin001/vcam-iphone.git
```

The project folder after that is `vcam-iphone`. The iPhone project is `vcam-iphone/ios/VCamdroid.xcodeproj`.

## 1. Put the app on the iPhone

Do this on the Mac, with the iPhone in your hand.

1. Install Xcode from the Mac App Store.
2. Open Xcode once and let it finish installing extra components. Sign in with your Apple ID when it asks: **Xcode → Settings → Accounts → +**.
3. Clone the repository, or download the ZIP from GitHub and unzip it.
4. In Finder, open `ios/VCamdroid.xcodeproj`. Xcode opens.
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
15. The first launch is a four-step walkthrough: what the iPhone does, how to get **VCamPanel** on the PC, USB or Wi-Fi, then camera and local network. On the last step, tap **Start** and tap **Allow** for the camera. If you will use Wi-Fi and iOS asks about the local network, tap **Allow**.
16. After that, opening VCamdroid goes straight to the camera screen. It is already waiting for the PC. **Help** on that screen replays the walkthrough.

**Check:** VCamdroid is on the iPhone home screen. The first open is the walkthrough. Later opens go to the camera screen, with an address such as `192.168.1.20:4747` and the line **Open VCamPanel on your PC and click Connect**. There is no **OBS Studio** button.

A free Apple ID install expires after about 7 days. When the phone refuses to open the app, plug it into the Mac and press **Run** in Xcode again. A paid Apple Developer account removes that 7-day limit.

You can unplug the phone from the Mac after this. The Mac is finished unless you need to reinstall the app.

## 2. Install the companion on the PC

Do this on the Windows PC.

1. From this repo’s **[latest release](https://github.com/ahoin001/vcam-iphone/releases/latest)**, download **[VCamdroid-Windows-x64.zip](https://github.com/ahoin001/vcam-iphone/releases/latest/download/VCamdroid-Windows-x64.zip)**. Unzip it somewhere you will keep, such as `Documents\VCamdroid`.
2. Confirm `VCamPanel.exe` and `VCamSource.dll` are in that folder. The camera cannot be installed if the DLL is somewhere else.
3. Run `VCamPanel.exe`. The window stays on top. The pin button at the top right turns that off.
4. On Windows 11, the **This PC** card shows **Install camera** on the **Camera listed as VCamdroid** row the first time. Click it and approve the Windows prompt. Windows loads cameras from its own system services, and those services cannot read your user folders, so VCamPanel copies the camera component to `C:\Program Files\VCamdroid` and registers it for this PC. You approve this once. A new release that changes the camera asks again. The camera remains after a reboot.
5. Read the **This PC** card at the top left. If it says this PC is older than Windows 11, this PC cannot publish VCamdroid. The desk still works. Use section 7 for the picture. A checked **Windows 11** row with a blocked **Camera listed as VCamdroid** row means the camera is not in the list yet. That row names the reason and, when a Windows error occurred, its code.

**Check:** VCamPanel shows **Connect your iPhone**. On Windows 11 the card says **This PC is ready** once the camera is listed. On Windows 10 it says this PC is older than Windows 11.

After that first unzip, you do not need GitHub for companion updates. VCamPanel checks for a newer zip when it opens. If one is ready, **This PC** and **Setup** show **A newer VCamPanel is ready**. Click **Update**. The picture stops, the window restarts, and the new `VCamPanel.exe` and `VCamSource.dll` replace the ones in that same folder. If the camera component changed, click **Install camera** again. **Setup → Check for updates** looks again without waiting. The phone app still updates with **Run** in Xcode.

To build the companion yourself instead of downloading it, open `windows/VCamdroid.sln` in Visual Studio 2022 (**Desktop development with C++**), set **Release | x64**, and build **VCamSource** and **VCamPanel**. Copy `VCamSource.dll` next to `VCamPanel.exe`.

## 3. Connect over Wi-Fi

Use this when the phone and the PC are on the same Wi-Fi and you do not want the cable in the shot. Skip to section 4 if you want USB instead.

1. Join the iPhone and the Windows PC to the same Wi-Fi network. A 5 GHz network is the steadier choice.
2. On the router, turn off client isolation and AP isolation. With either one on, the PC cannot reach the phone.
3. Turn off any VPN on the phone and on the PC for the first connection.
4. On the iPhone, open **VCamdroid**. It opens on the camera screen.
5. If iOS asks to find and connect to devices on the local network, tap **Allow**.
6. Leave the camera screen in front. The status reads **Open VCamPanel on your PC and click Connect**. It also shows an address such as `192.168.1.20:4747`, and a line for USB or Wi-Fi.
   - The number before the colon is the IP address. In the example, that is `192.168.1.20`.
   - The number after the colon is the port. The default is **4747**.
7. On the PC, in **VCamPanel**, the phone appears under **Phones** within a few seconds. Click **Refresh** to look again right away. Pick the Wi-Fi phone, or type the IP from the phone under **Wi-Fi address**, then click **Connect**.

**Check:** On Windows 11, the phone changes from **Open VCamPanel on your PC and click Connect** to **The camera is live. Choose VCamdroid in your app.** Later connections say **Streaming**. The badge reads **Wi-Fi**, and the VCamPanel toolbar says **Streaming over Wi-Fi** and to choose **VCamdroid** as the camera. On Windows 10 the phone stays on that connect line until section 7, and the toolbar says this PC is older than Windows 11.

Keep the phone on that studio screen. The side button and the app switcher stop the camera.

## 4. Connect over USB

Use this when you want the cable instead of Wi-Fi. VCamPanel finds the iPhone and keeps the USB tunnel open.

1. On the PC, install **Apple Devices** or iTunes from the Microsoft Store and open it once.
2. Run `VCamPanel.exe` from the folder in section 2 if it is not already open.
3. Plug the iPhone into the PC with a data-capable USB-C cable. Unlock the phone and tap **Trust This Computer** if it asks.
4. On the iPhone, open **VCamdroid** and leave the camera screen in front. The status can say **Open VCamPanel on your PC and click Connect**.
5. **VCamPanel** shows one **USB** row per iPhone. It connects to that phone by itself when it starts, and again when you plug one in. If more than one phone is listed, click the one you want, then **Connect**. A second copy of the same phone is not listed. If it does not connect, click **Refresh**, select the **USB** phone, and click **Connect**. When USB cannot connect, the toolbar says **Couldn’t connect** and names the usual cause: Apple Mobile Device Service is not running, the cable is not a data cable, Trust was not accepted, or VCamdroid is not in front.
6. Leave the companion open while you use the camera.

**Check:** On Windows 11, the phone says **The camera is live. Choose VCamdroid in your app** the first time, then **Streaming**. The badge reads **USB**, and the VCamPanel toolbar says **Streaming over USB** and to choose **VCamdroid** as the camera. On Windows 10 the phone stays on **Open VCamPanel on your PC and click Connect** until section 7, and the toolbar says this PC is older than Windows 11.

If you changed **Port** in the phone’s Settings, type that port in the companion’s **Wi-Fi address** field (for example `127.0.0.1:4748`), then select the USB phone again and click **Connect**.

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
3. If the camera is missing, click **Setup** in the VCamPanel toolbar, or read the **This PC** card before you connect. A missing `VCamSource.dll`, or Windows 10, is the usual cause. The card opens by itself while something is missing and folds to **This PC is ready** once Windows 11, the DLL, and the camera name are in place. Click it to show or hide the list. **Setup** also shows whether the picture is live. **Guide**, next to Setup, is the lighting and camera-settings reference.

For a clean background, leave **Portrait** off on the phone. Do the cut on the PC: in **NVIDIA Broadcast**, set Camera to **VCamdroid**, then pick NVIDIA Broadcast in OBS or the meeting app. In **OBS**, add the **VCamdroid** Video Capture Device, then add a Background Removal (or similar) filter on that source. Phone Portrait is a light blur if you need it without those apps, and it warms the iPhone. Do not run phone Portrait and a PC background filter together, or the edges stack.

**Check:** the app shows the iPhone, and the phone still says **Streaming**.

To record that picture in OBS without a second encode washing it out, set the **VCamdroid** Video Capture Device to a custom resolution of **1920×1080** at **30** fps, format **NV12**, color space **709**, color range **Limited**, and turn **Use buffering** off. In Settings → Video, set both the base and the output to **1920×1080** at **30** fps, with no rescale. Record to **MKV** (remux to MP4 afterwards) with NVENC constant quality **CQP 16–18**, or x264 **CRF 16**.

Closing VCamPanel, or leaving the phone app, stops the live picture. The camera can remain in the list and show black until you connect again. The phone has no microphone in this build, so the meeting uses the computer’s microphone or another mic you select in that app. Video from the phone arrives a little after that mic. Record a clap once, then in OBS open the mic’s Advanced Audio Properties and set **Sync Offset** to the gap between the sound and the hands. Keep the same cable and settings and that number stays valid.

## 6. Adjust the camera from the desk

After Connect, VCamPanel shows the camera desk under its toolbar. Finish section 3 or section 4 first, then leave `VCamPanel.exe` connected. The toolbar keeps the connection, the phone’s battery, and the resolution in view.

On a wide window the preview fills the left side and the shot controls sit on the right. Drag the thin grabber between them to grow the picture or the desk. A narrow window stacks the preview above the controls; the same grabber then resizes the split up and down. That preview is the same picture NVIDIA Broadcast, OBS, and the other apps receive: VCamPanel decodes the phone once and shows those frames at the stream rate. Moving a control updates the next frame here and in those apps.

While the picture is moving, the toolbar shows **Ready** when the delay is steady and under 150 ms, about 30 frames a second are decoding, and no warning is up. Before that settles it shows the delay as a small chip, such as **42 ms**. **Peaking** outlines sharp edges, **Zebra** marks highlights that are close to clipping, and **1:1** shows the center of the frame at full size. Those three change only the preview in this window. The recorded picture stays clean. If the picture drops after it has been moving, the toolbar says **Reconnecting** and VCamPanel keeps trying every few seconds until the picture is back. Click **Disconnect** when you want that to stop. A notice appears above the status line, and stays for about two seconds after the cause clears, when something is wrong: **Open Setup and click Install camera so other apps can select VCamdroid** means the picture is live and Windows has not listed the camera yet (the **This PC** card and **Setup** are where you click Install). The other notices are: **Running behind** means the picture is late or getting later; **Dropping frames** means the PC is decoding fewer than 27 frames a second; **USB couldn’t keep up** means the phone had to discard pictures; **Phone is hot, quality may drop** means the iPhone is thermally limited; **The recording app is getting repeated frames** means OBS or NVIDIA Broadcast asked again before a new frame arrived. If a drop notice lasts about five seconds, **See fix** appears next to it. That opens a short list of likely causes from the phone’s current settings and how busy this PC is. A button such as **Turn off Portrait** or **Use 1/60** changes that setting on the phone. **Not now** hides that tip until the next Connect. A delay that stays the same is corrected with the mic Sync Offset in section 5, not in VCamPanel.

An OBS browser dock of `/panel` is different. It has no companion preview, so **Show preview** there is a lighter glance from the phone. Leave it off if the phone is warming up. On a wide dock, drag the grabber beside the preview to resize the inspector. The dock remembers that width and the open sections for the address it was opened with. A new phone IP starts the dock at its defaults. The app you selected in section 5, and the preview inside VCamPanel, are the live picture.

The desk is an inspector: each section shows its current value on the closed row, so you can skim without scrolling. Lens and Exposure start open. White balance, focus, portrait, and stream stay folded until you need them. Open or close a section and it stays that way. Each tool is a name, its current value, and one line. **?** opens a short note. Exposure’s note is about shutter and ISO together: lock shutter at double the frame rate (1/60 at 30 fps), then raise ISO only if the face is still dark. The desk matches the phone: front or back camera, zoom stops 1× / 2× / 3× / 5× / 10× and a 1.0×–10.0× slider, torch, flip, exposure (auto EV, or manual ISO and shutter), white balance, focus, and portrait. If the selected camera cannot lock color, Daylight and the other color presets stay dim and the phone stays on Auto. If it cannot lock focus or exposure, Manual stays off. The torch stays off when that camera has no light. Moving a control on the computer moves it on the phone. Moving it on the phone updates the desk.

**Heat** in the VCamPanel toolbar, next to **Guide**, opens the heat tab: curtain, leaving Portrait off, torch and charging, and the lighter bitrate the phone uses when it warms up. The picture stays 1080p30. **Guide**, next to **Setup**, opens the longer reference: lighting, exposure, color, and starting points for different rooms. The Lighting tab follows Senpai’s lighting MasterClass (a big, soft key light, not lighting both sides the same, four two-light setups, and lighting yourself more than the room), with a real photo and a light-placement dial for each setup. **Watch the video** opens it in your browser. Photo credits are at the bottom of that tab and in `docs/GUIDE_PHOTOS.md`. The same **Guide** button on the desk opens it beside the picture, including in an OBS browser dock.

**Presets** at the top of the desk saves the current look on the iPhone. Use one for morning window light and another for evening lamps. Choosing a preset applies lens, zoom, exposure, white balance, focus, torch, flip, and portrait. **Delete** removes the selected one.

Leave Portrait off for meetings. NVIDIA Broadcast or OBS Background Removal on **VCamdroid** is the cleaner cut. Use the iPhone blur only when you do not have those PC tools.

The stream heading shows the rate actually in use. While VCamPanel is the video client, that starts at 1080p30 at **12 Mbps** on Wi-Fi and **30 Mbps** on USB. If the phone warms up, the heading drops to a lighter rate and the picture stays 1080p30. The bitrate slider under it stays on the saved rate for the optional OBS source in section 7 (2500–16000 kbps). Moving that slider does not change the VCamdroid picture. While VCamPanel is connected, the **Size** and **Frame rate** buttons are dimmed at 1080 and 30, with a note that VCamPanel holds 1080p30.

A desk control only changes when you change it, here or on the phone. A change made on the phone shows up on the desk within about a second.

**On Windows:**

1. Run `VCamPanel.exe` if it is not already open. Before Connect, the window shows **Connect your iPhone** with the phones it found, and the **This PC** card for anything still missing. After Connect, the live preview opens beside the controls, or above them in a narrow window. Drag the grabber between preview and desk to resize the split. The window itself also resizes from any edge.

VCamPanel remembers your layout between launches: window size and position (and whether it was maximized), the split for wide and for stacked windows separately, which desk sections are open, the pin, and the last phone address. That survives a new phone IP and switching between USB and Wi-Fi. The files live in `%APPDATA%\VCamdroid`. Close VCamPanel and delete the `panel-window.txt`, `panel-split.txt`, and `desk-layout.json` files there to return to the default layout. If a saved window position is on a monitor that is no longer connected, VCamPanel opens on the main screen instead.
2. Pick the USB iPhone or a Wi-Fi phone under **Phones**, then click **Connect**. If Wi-Fi discovery misses it, type the IP from the phone under **Wi-Fi address** and click **Connect**.
3. The window stays on top while you shoot. The pin button in the toolbar turns that off and on. **Copy** next to the OBS address in the toolbar puts that address on the clipboard.

**In OBS, on Windows,** you can also dock the same page. This is optional. The companion already shows the desk.

1. Open **Docks → Custom Browser Docks**.
2. Set the dock name to **VCamdroid**.
3. Set the URL to `http://` plus the address from the companion or the phone, plus `/panel`. USB: `http://127.0.0.1:4747/panel`. Wi-Fi example: `http://192.168.1.20:4747/panel`.
4. Click **Apply**. Leave the dock open while you shoot.

**Check:** moving Zoom in the companion changes the picture in the app that selected **VCamdroid**, and the zoom control on the phone moves with it. Saving a preset and choosing it again restores that look.

## 7. Optional: a DroidCam OBS source

Use this when you want OBS to connect to port 4747 itself, instead of selecting the VCamdroid camera. This is the picture path on Windows 10. It is also available on Windows 11.

The phone allows one video connection. Connect in VCamPanel always takes that socket, including on Windows 10. Disconnect VCamPanel before you add this source. If the source connects first, before VCamPanel has shown a picture, VCamPanel stops and says the video connection closed so the source can keep the phone. If VCamPanel already had a picture, it keeps trying until you click **Disconnect**. Click **Connect** in the toolbar after the source is gone when the toolbar says the connection closed.

1. Install [DroidCam OBS](https://www.dev47apps.com/obs/) and reopen OBS.
2. On Wi-Fi, leave VCamPanel disconnected. The source uses the IP on the phone and port **4747**.
3. On USB, click **Disconnect** in VCamPanel so it is not the video client. In the unzipped folder, run `usbmux_forward.exe` and leave that window open. The source uses **127.0.0.1** and port **4747**.
4. In OBS, click **+** in **Sources** and add **DroidCam OBS** (or **VCamdroid** if you already have that source). Enter the address from the step above. **Copy** in VCamPanel copies it when the companion is open. Click **OK**.

**Check:** the phone says **Streaming**, and the picture is in the OBS preview.

Leave phone **Portrait** off if you add Background Removal (or NVIDIA Broadcast after **Start Virtual Camera**) on this OBS path. The same stacking rule as section 5 applies.

Other apps still need a camera. With this optional source, that camera is OBS’s **Start Virtual Camera**, set to **OBS Virtual Camera** in the meeting app. The VCamdroid camera in section 5 is the path that does not need that step.

## While you are live

The studio screen always runs sideways (landscape), the same shape as the 16:9 picture the computer receives. Mount the phone sideways. Either side can be up: the picture stays upright and the phone rotates its controls to match. Use Flip 180° only if the computer's picture is still upside down. The preview shows exactly the frame that is sent, so a narrow black edge on the left and right is expected on taller iPhones.

On the phone, the picture fills the screen height and the address stays at the top. Tap the preview to focus and put the open control away. The bottom bar opens one tool at a time: switch camera, zoom (1×, 2×, 3×, 5×, 10×, or 1.0×–10.0×), exposure (auto EV, or manual ISO and shutter), white balance, focus, and portrait. Torch and Flip 180° toggle immediately. Moon and Settings sit together on the lower right, next to Stop. Tap the moon to cover the screen in black. The preview stops drawing while that cover is up, and the iPhone brightness stays where you set it. Tap the black screen to bring the controls back. Automatic, Always, and Off are in Settings. **Heat**, next to Help, is the same heat reference as the VCamPanel toolbar. Log is in Settings.

Portrait on the phone is a light background blur. Leave it off when NVIDIA Broadcast or OBS is already cutting the background. If you turn it on, the iPhone preview stays off, and the phone tells you to watch the picture on the computer. **Show on iPhone**, or the **iPhone preview** switch in the portrait tool, draws the blurred picture on the phone. Leave it off if the phone warms up.

From the computer, section 6 is that same camera desk in `VCamPanel.exe`. While VCamPanel is connected, the picture stays 1080p30. If the phone warms up, the status says the picture is at a lighter rate. The size and frame rate stay put, and exposure, color, focus, and zoom stay as you set them. Resolution and frame-rate changes wait until that client disconnects.

In **Settings** on the phone:

- **Camera screen:** curtain, confirm before stopping, and tally. Curtain is Automatic (covers the screen after 8 seconds), Always (covers as soon as the camera is on), or Off. The moon still covers the screen when Off. Tap the black screen to get the controls back. The cover does not change the iPhone’s brightness. Tally turns red while OBS is recording or streaming, and green while OBS is only in studio mode. In OBS, turn on the WebSocket server (Tools → WebSocket Server Settings). If that server has a password, type it under **Setup** in VCamPanel. A DroidCam OBS source can still send its own tally when that source, rather than VCamPanel, is the video client.
- **Portrait.**
- **Presets** saves the current look, and can apply, rename, or delete one. The desk has the same presets.
- **Connection:** port (4747 unless something else is using it) and discovery. Changing the port or discovery restarts the listener.
- **DroidCam OBS source:** resolution, frame rate, and bitrate. These are the saved rates for section 7. They do not change the VCamdroid picture while VCamPanel is connected.
- **Help**, **Replay setup**, and **Log**.

For a long session, use the curtain and leave Low Power Mode off. **Heat** on the phone, and **Heat** in the VCamPanel toolbar, explain the rest. If the phone warms up, the studio says the picture is at a lighter rate, whether or not Portrait is on. Charging while the phone is already warm adds heat, so that line says a long show runs cooler on battery. Pressing the side button pauses the stream. Unlock the phone and open **VCamdroid** again to continue.

## If the picture never appears

- The phone must stay on the camera screen. **Open VCamPanel on your PC and click Connect** means the app is ready and VCamPanel has not opened the picture yet. **Camera stopped** means you tapped Stop. Tap **Start** on that card to wait for the PC again.
- Connect starts the picture even if the camera-list warning is showing. While the picture is live, that install step stays a notice under the toolbar, and the title stays **Streaming over USB** or **Streaming over Wi-Fi**. Disconnect stops the picture. If that iPhone is still in the list, VCamPanel stays on **Disconnected** and does not connect again until you click **Connect**.
- If the cable comes out, or the phone leaves the Wi-Fi, VCamPanel leaves the connected screen and says **The iPhone disconnected**. Plug the same phone back in and it connects again. A picture that drops while the phone is still there comes back on its own: the toolbar says **Reconnecting** until the frames return. **The video connection closed** appears only when VCamPanel has given up, which happens when another app took the phone before any picture arrived. Click **Connect** after that.
- VCamPanel must stay running. The picture goes black when it closes, even if **VCamdroid** is still listed.
- `VCamSource.dll` must sit in the same folder as `VCamPanel.exe`. If it is missing, the **This PC** card and **Setup** say so.
- Windows 11 is required for the camera to appear in the list. The **This PC** card names that, a missing DLL, or another registration failure. On Windows 10 use section 7.
- **The Windows camera services could not load VCamdroid (error 80070003, 80004002, or 80070005)** means the camera component is not installed where those services can read it, or the installed copy is from an older build. Click **Install camera** on that row and approve the Windows prompt. If the error stays, close VCamPanel, replace both `VCamPanel.exe` and `VCamSource.dll` with the latest release, restart the PC, and click **Install camera** again.
- If the Windows prompt never appears, your account may not be allowed to approve administrator changes. Ask someone with an administrator account to click **Install camera** on this PC once.
- In the meeting app or OBS, the device name is **VCamdroid**.
- The phone allows one video connection. A DroidCam OBS source connected at the same time takes the picture. If VCamPanel never showed a frame, it says the video connection closed so the source can keep the phone. If it already had a picture, it keeps reconnecting until you click **Disconnect**. Disconnect the source and click **Connect** when the toolbar says the connection closed.
- Wi-Fi: the PC and the phone are on the same network, and the IP in VCamPanel is exactly the address on the phone. A VPN on either device can hide them from each other. Client isolation on the router blocks the connection.
- USB: **VCamPanel** is still connected. When Connect fails, its toolbar names the usual cause: Apple Mobile Device Service is not running, the cable is not a data cable, Trust was not accepted, or the phone app is not in front. If the toolbar says **USB is connected, but VCamdroid is not on the camera screen**, the cable is fine and the phone app is not showing the camera. Open that screen and leave it in front.
- If the toolbar says **The PC is receiving the picture, but it cannot decode it**, the phone is sending and VCamPanel has not produced a frame. The phone shows the same sentence. Leave the camera screen open and click **Connect** again. Replace `VCamPanel.exe` with the latest release if it still says that.
- The optional OBS source address for USB is **127.0.0.1** port **4747**, with the tunnel connected.
- **That port is already in use** on the phone means something else has port 4747. Change **Port** in the phone’s Settings, then use the new port in VCamPanel and, if you use section 7, in OBS.
- **Camera access is off** means the iPhone blocked the camera. Turn it on in **Settings → Privacy & Security → Camera → VCamdroid**.
- Before Connect, **VCamPanel** should show **Connect your iPhone**. If the window stays empty, the WebView2 Runtime is missing, and VCamPanel shows a message that says so. Install it, then reopen the companion.
- If **Update** fails, the folder may not be writable, or this PC cannot reach GitHub. Move the unzipped companion to a folder you own, such as `Documents\VCamdroid`, then click **Check for updates** again. You can still replace the files from the [latest release](https://github.com/ahoin001/vcam-iphone/releases/latest) zip.
- After Connect, the desk column says **Opening the camera desk** until it can load the phone’s `/panel` page. It retries by itself. Keep the phone on the studio screen, and check the Wi-Fi notes above if it never opens.
- After Connect, the preview and the toolbar tell you to leave **VCamdroid** open on the camera screen. The picture replaces that message once frames arrive. That preview matches the camera. In an OBS browser dock, **Show preview** is a lighter glance and can stay off; the app that selected **VCamdroid** still shows the picture.
