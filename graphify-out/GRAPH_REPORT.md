# Graph Report - .  (2026-10-18)

## Corpus Check
- cluster-only mode — file stats not available

## Summary
- 1440 nodes · 3041 edges · 87 communities (76 shown, 11 thin omitted)
- Extraction: 94% EXTRACTED · 6% INFERRED · 0% AMBIGUOUS · INFERRED: 187 edges (avg confidence: 0.8)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `b51fc2e7`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- Community 0
- Community 1
- Community 2
- Community 3
- Community 4
- Community 5
- Community 6
- Community 7
- Community 8
- Community 9
- Community 10
- Community 11
- Community 12
- Community 13
- Community 14
- Community 15
- Community 16
- Community 17
- Community 18
- Community 19
- Community 20
- Community 21
- Community 22
- Community 23
- Community 24
- Community 25
- Community 26
- Community 27
- Community 28
- Community 29
- Community 30
- Community 31
- Community 32
- Community 33
- Community 34
- Community 35
- Community 36
- Community 37
- Community 38
- Community 39
- Community 40
- Community 41
- Community 42
- Community 43
- Community 44
- Community 45
- Community 46
- Community 47
- Community 48
- Community 49
- Community 50
- Community 51
- Community 52
- Community 53
- Community 54
- Community 55
- Community 56
- Community 57
- Community 58
- Community 59
- Community 60
- Community 61
- Community 62
- Community 63
- Community 64
- Community 65
- Community 66
- Community 67
- Community 68
- Community 69
- Community 70
- Community 71
- Community 72
- Community 73
- Community 74
- Community 75
- Community 76
- Community 77
- Community 78
- Community 79
- Community 80
- Community 81
- Community 82
- Community 83
- Community 84
- Community 85
- Community 86

## God Nodes (most connected - your core abstractions)
1. `StudioSession` - 68 edges
2. `MediaStream` - 45 edges
3. `SwiftUI` - 43 edges
4. `SourceActivate` - 41 edges
5. `StudioSettings` - 37 edges
6. `MediaSource` - 37 edges
7. `VideoMode` - 35 edges
8. `PreviewSurface` - 34 edges
9. `Foundation` - 33 edges
10. `CodingKeys` - 31 edges

## Surprising Connections (you probably didn't know these)
- `CaptureCore` --calls--> `CaptureVideoSink`  [INFERRED]
  ios/VCamdroid/Capture/CaptureCore.swift → ios/VCamdroid/Capture/CaptureVideoSink.swift
- `CaptureCore` --calls--> `PortraitProcessor`  [INFERRED]
  ios/VCamdroid/Capture/CaptureCore.swift → ios/VCamdroid/Capture/PortraitProcessor.swift
- `CaptureEngine` --calls--> `StudioSettings`  [INFERRED]
  ios/VCamdroid/Capture/CaptureEngine.swift → ios/VCamdroid/Session/StudioSettings.swift
- `StudioSession` --calls--> `PortraitPreviewSink`  [INFERRED]
  ios/VCamdroid/Session/StudioSession.swift → ios/VCamdroid/Capture/PortraitPreviewSink.swift
- `StudioSession` --calls--> `PreviewJPEGSink`  [INFERRED]
  ios/VCamdroid/Session/StudioSession.swift → ios/VCamdroid/Capture/PreviewJPEGSink.swift

## Import Cycles
- None detected.

## Communities (87 total, 11 thin omitted)

### Community 0 - "Community 0"
Cohesion: 0.06
Nodes (60): optional, acceptLoop(), BinaryPlist, bytes, decoded, objects, offsets, refSize (+52 more)

### Community 1 - "Community 1"
Cohesion: 0.06
Nodes (60): IMFSample, IMFTransform, SECURITY_ATTRIBUTES, SECURITY_DESCRIPTOR, wchar_t, FrameBus, Alive, base_ (+52 more)

### Community 2 - "Community 2"
Cohesion: 0.09
Nodes (59): IMFActivate, LPCWSTR, LPWSTR, MF_ATTRIBUTE_TYPE, MF_ATTRIBUTES_MATCH_TYPE, REFPROPVARIANT, BOOL, GUID (+51 more)

### Community 3 - "Community 3"
Cohesion: 0.07
Nodes (35): CVPixelBufferPool, Darwin, PortraitCare, eased, heavierAndBattery, heavierMode, hidden, lowBattery (+27 more)

### Community 4 - "Community 4"
Cohesion: 0.07
Nodes (43): HBITMAP, HFONT, HGDIOBJ, ClampByte(), HDC, HWND, LPARAM, LRESULT (+35 more)

### Community 5 - "Community 5"
Cohesion: 0.09
Nodes (16): AVCaptureDevice, CaptureConfiguration, Bool, CGFloat, Double, Int, CaptureCore, Bool (+8 more)

### Community 6 - "Community 6"
Cohesion: 0.09
Nodes (34): CaseIterable, Codable, Equatable, Identifiable, ControlUpdate, SessionUpdate, Int, VideoClientKind (+26 more)

### Community 7 - "Community 7"
Cohesion: 0.11
Nodes (20): ControlState, Bool, Double, String, DeviceAcceptance, StudioSettings, Bool, Double (+12 more)

### Community 8 - "Community 8"
Cohesion: 0.08
Nodes (18): CFString, CFTypeRef, CMFormatDescription, Encoder, CMTime, CVPixelBuffer, Int, Sendable (+10 more)

### Community 9 - "Community 9"
Cohesion: 0.07
Nodes (30): CodingKey, CodingKeys, allowsHighFrameRate, bitrateKbps, confirmStop, curtainMode, discoveryEnabled, explainedLocalNetwork (+22 more)

### Community 10 - "Community 10"
Cohesion: 0.13
Nodes (15): async, DiscoveryAdvertiser, NWListener, String, DroidCamServer, Bool, Data, Int (+7 more)

### Community 11 - "Community 11"
Cohesion: 0.09
Nodes (27): HANDLE, IMFMediaStream2, queue, MF_STREAM_STATE, IMFStreamDescriptor, IUnknown, MF_STREAM_STATE, mutex (+19 more)

### Community 12 - "Community 12"
Cohesion: 0.15
Nodes (23): Int, String, VideoRequestParser, Int, String, VideoMode, controlsReadRoute(), controlUpdateIsPartial() (+15 more)

### Community 13 - "Community 13"
Cohesion: 0.08
Nodes (18): ControlRack, StudioChrome, Animation, Bool, Void, StudioCompactDock, Void, StudioTool (+10 more)

### Community 14 - "Community 14"
Cohesion: 0.08
Nodes (12): App, HomeView, LocalNetworkPermissionView, NavigationPath, LogView, PortraitPreviewNotice, StudioScrim, StudioStatusBar (+4 more)

### Community 15 - "Community 15"
Cohesion: 0.14
Nodes (17): HMODULE, IClassFactory, REFCLSID, STDAPI, ClassFactory, refCount_, BOOL, DWORD (+9 more)

### Community 16 - "Community 16"
Cohesion: 0.15
Nodes (7): StudioSession, Bool, CGFloat, LocalizedStringResource, String, Void, Never

### Community 17 - "Community 17"
Cohesion: 0.15
Nodes (21): COREWEBVIEW2_COLOR, ICoreWebView2, ICoreWebView2Controller, ICoreWebView2Environment, PWSTR, sockaddr_in, ApplyWindowMaterial(), ConfigureWebView() (+13 more)

### Community 18 - "Community 18"
Cohesion: 0.09
Nodes (15): CameraDeniedView, LogRow, CurtainOverlay, FocusControl, PortraitCareBanner, Color, PortraitControl, String (+7 more)

### Community 19 - "Community 19"
Cohesion: 0.11
Nodes (21): IMFGetService, IMFMediaEventQueue, IMFMediaSourceEx, IMFPresentationDescriptor, KsControl, IMFAttributes, IMFAttributes, MediaSource (+13 more)

### Community 20 - "Community 20"
Cohesion: 0.15
Nodes (20): IMFAsyncResult, IMFMediaEvent, IMFMediaSource, DWORD, IMFStreamDescriptor, LPVOID, REFIID, STDMETHODIMP (+12 more)

### Community 21 - "Community 21"
Cohesion: 0.16
Nodes (17): atomic, CloseLive(), SOCKET, string, vector, Worker, MediaFoundationSession, com (+9 more)

### Community 22 - "Community 22"
Cohesion: 0.20
Nodes (17): IMFMediaType, MediaSource, ULONG, CreateMediaType(), KsEvent, KsMethod, KsProperty, MediaSource::MediaSource() (+9 more)

### Community 23 - "Community 23"
Cohesion: 0.25
Nodes (6): DroidCamHTTP, Bool, Data, Double, Int, String

### Community 24 - "Community 24"
Cohesion: 0.11
Nodes (18): DroidCamRequest, applyControls, audio, battery, capabilities, controls, guidePhoto, panel (+10 more)

### Community 25 - "Community 25"
Cohesion: 0.16
Nodes (10): PresetCommand, apply, delete, rename, save, ShotPreset, ShotPresetStore, Bool (+2 more)

### Community 26 - "Community 26"
Cohesion: 0.30
Nodes (17): CameraIsListed(), ClassKey(), Code(), ContentTag(), HRESULT, wstring, ExeDirectory(), ExpectedInstallPath() (+9 more)

### Community 27 - "Community 27"
Cohesion: 0.24
Nodes (17): HINSTANCE, ClampDeskRatio(), HWND, LPARAM, LRESULT, RECT, UINT, WPARAM (+9 more)

### Community 28 - "Community 28"
Cohesion: 0.24
Nodes (16): map, browse(), string, vector, instanceName(), lower(), parsePacket(), readName() (+8 more)

### Community 29 - "Community 29"
Cohesion: 0.18
Nodes (13): LinkState, string, wstring, wstring, Pipeline, client_, Prepare, prepared_ (+5 more)

### Community 30 - "Community 30"
Cohesion: 0.16
Nodes (17): AppendJsonBool(), AppendJsonString(), BuildState(), Kind, string, DeviceKey(), FindDevice(), JsonString() (+9 more)

### Community 31 - "Community 31"
Cohesion: 0.24
Nodes (17): ConnectFromShell(), CopyText(), wchar_t, wstring, DeskTarget(), EnvFolder(), LoadSetting(), Narrow() (+9 more)

### Community 32 - "Community 32"
Cohesion: 0.12
Nodes (12): Hashable, AppRoute, cameraDenied, cameraPermission, howToConnect, localNetwork, settings, studio (+4 more)

### Community 33 - "Community 33"
Cohesion: 0.17
Nodes (3): ScreenCurtainController, CGFloat, ScenePhase

### Community 34 - "Community 34"
Cohesion: 0.14
Nodes (16): ApplySession(), BuildChecks(), vector, DecodedRecently(), JsonBool(), JsonInt(), ReceivingRecently(), ScanResult (+8 more)

### Community 35 - "Community 35"
Cohesion: 0.19
Nodes (10): CameraPreview, AVCaptureSession, Bool, CGFloat, CGPoint, Context, Void, PortraitPreviewSink (+2 more)

### Community 36 - "Community 36"
Cohesion: 0.13
Nodes (15): mutex, thread, worker_, bytesReceived, closedWithoutVideo, decodeFailures, globalBus, immediateCloses (+7 more)

### Community 37 - "Community 37"
Cohesion: 0.24
Nodes (8): AsyncStream, OrderedPacketForwarder, Data, UInt64, VideoPacket, packetsLeaveInArrivalOrder(), SentPackets, UInt64

### Community 38 - "Community 38"
Cohesion: 0.23
Nodes (4): AVFoundation, CoreMedia, Foundation, Observation

### Community 40 - "Community 40"
Cohesion: 0.25
Nodes (14): MediaEventType, GUID, HRESULT, PROPVARIANT, REFGUID, EmptyVariant(), QueueEvent, Start (+6 more)

### Community 41 - "Community 41"
Cohesion: 0.15
Nodes (13): LinkState, LinkState, Worker, LinkProgress, bytesReceived, closedWithoutVideo, decodeFailures, lastByteMs (+5 more)

### Community 42 - "Community 42"
Cohesion: 0.15
Nodes (10): Error, CaptureEngineError, cameraUnavailable, cannotAddInput, formatUnavailable, DroidCamServerError, invalidPort, portInUse (+2 more)

### Community 43 - "Community 43"
Cohesion: 0.24
Nodes (4): CGPoint, Int, UIInterfaceOrientation, Task

### Community 44 - "Community 44"
Cohesion: 0.27
Nodes (9): UInt32, DroidCamPacketFramer, Data, UInt64, annexBRewritesLengthPrefixes(), configPacketUsesNoPTS(), videoPacketRoundTrip(), Testing (+1 more)

### Community 45 - "Community 45"
Cohesion: 0.26
Nodes (7): StudioBindings, Binding, Double, Int, StudioSession, Value, WritableKeyPath

### Community 46 - "Community 46"
Cohesion: 0.24
Nodes (4): StudioSession, Bool, Double, Int

### Community 47 - "Community 47"
Cohesion: 0.33
Nodes (12): HRESULT, Disconnect(), FinishCameraInstall(), HandleShellMessage(), Layout(), PushState(), SetGuide(), SetOnTop() (+4 more)

### Community 48 - "Community 48"
Cohesion: 0.18
Nodes (7): ButtonStyle, PressableButtonStyle, Configuration, View, StudioDockButtonStyle, Configuration, View

### Community 49 - "Community 49"
Cohesion: 0.22
Nodes (7): HowToConnectView, LocalizedStringResource, Void, StudioReadout, LocalizedStringResource, String, Text

### Community 50 - "Community 50"
Cohesion: 0.22
Nodes (8): AppDelegate, OrientationLock, NSObject, UIApplication, UIApplicationDelegate, UIInterfaceOrientationMask, UIWindow, UIWindowScene

### Community 51 - "Community 51"
Cohesion: 0.25
Nodes (8): StudioOrientation, StudioOrientationView, Context, UIInterfaceOrientation, Void, NSKeyValueObservation, UIView, UIViewRepresentable

### Community 52 - "Community 52"
Cohesion: 0.20
Nodes (6): Accelerate, CoreGraphics, CoreImage, CoreVideo, VideoToolbox, Vision

### Community 53 - "Community 53"
Cohesion: 0.24
Nodes (7): CameraPreviewView, AVSampleBufferVideoRenderer, CGPoint, CGRect, NSCoder, Void, UITapGestureRecognizer

### Community 54 - "Community 54"
Cohesion: 0.29
Nodes (7): H264Profile, baseline, high, StreamQuality, Bool, Int, panelQualityFollowsTheTransport()

### Community 55 - "Community 55"
Cohesion: 0.27
Nodes (5): string, Service, ipv4, name, port

### Community 56 - "Community 56"
Cohesion: 0.22
Nodes (8): AVCaptureConnection, AVCaptureOutput, AVCaptureVideoDataOutputSampleBufferDelegate, CaptureVideoSink, CMSampleBuffer, CMTime, CVPixelBuffer, Void

### Community 57 - "Community 57"
Cohesion: 0.39
Nodes (9): unique_ptr, ApplyScan(), AutoConnect(), Connected(), ConnectTo(), SetStatus(), ShowIdleStatus(), StartPoll() (+1 more)

### Community 58 - "Community 58"
Cohesion: 0.22
Nodes (8): CameraSetup, detail, installed, listed, needsInstall, sourceDll, windows11, wstring

### Community 59 - "Community 59"
Cohesion: 0.25
Nodes (8): CheckState, Check, action, detail, pc, state, title, CheckName()

### Community 60 - "Community 60"
Cohesion: 0.36
Nodes (5): log, ExposureControl, Binding, Double, String

### Community 61 - "Community 61"
Cohesion: 0.25
Nodes (3): OnboardingView, LocalizedStringResource, String

### Community 62 - "Community 62"
Cohesion: 0.25
Nodes (5): SettingsView, Binding, Bool, Int, LocalizedStringResource

### Community 64 - "Community 64"
Cohesion: 0.52
Nodes (4): PreviewJPEGSink, CVPixelBuffer, Data, TimeInterval

### Community 65 - "Community 65"
Cohesion: 0.43
Nodes (5): SessionSnapshot, Bool, Int, String, sessionJSONFieldOrder()

### Community 66 - "Community 66"
Cohesion: 0.43
Nodes (4): Bool, Double, String, ZoomControl

### Community 67 - "Community 67"
Cohesion: 0.29
Nodes (7): Tone, StatusView, chips, detail, title, tone, ToneName()

### Community 68 - "Community 68"
Cohesion: 0.29
Nodes (6): Photo, chunks, count, name, GuidePhotoUri(), ShellHtml()

### Community 69 - "Community 69"
Cohesion: 0.53
Nodes (4): Date, LogEntry, String, UUID

### Community 70 - "Community 70"
Cohesion: 0.40
Nodes (6): IMFAsyncCallback, IUnknown, BeginGetEvent, SetD3DManager, BeginGetEvent, RequestSample

### Community 71 - "Community 71"
Cohesion: 0.53
Nodes (3): ResolutionCatalog, String, resolutionsJSONListsBothLenses()

### Community 72 - "Community 72"
Cohesion: 0.33
Nodes (5): ExposureValueSlider, Binding, Double, LocalizedStringResource, String

### Community 73 - "Community 73"
Cohesion: 0.33
Nodes (5): StudioGlassIcon, Color, LocalizedStringResource, String, Void

### Community 74 - "Community 74"
Cohesion: 0.33
Nodes (3): LocalizedStringResource, String, WhiteBalanceControl

### Community 76 - "Community 76"
Cohesion: 0.60
Nodes (3): CMSampleBuffer, CMTime, CVPixelBuffer

### Community 78 - "Community 78"
Cohesion: 0.40
Nodes (4): StudioDockButton, LocalizedStringResource, String, Void

### Community 80 - "Community 80"
Cohesion: 0.50
Nodes (3): usbmux_forward, VCamPanel, VCamSource

## Knowledge Gaps
- **243 isolated node(s):** `cameraUnavailable`, `cannotAddInput`, `formatUnavailable`, `off`, `balanced` (+238 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **11 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `StudioSession` connect `Community 16` to `Community 64`, `Community 65`, `Community 33`, `Community 35`, `Community 3`, `Community 5`, `Community 38`, `Community 69`, `Community 8`, `Community 37`, `Community 10`, `Community 39`, `Community 43`, `Community 7`, `Community 46`, `Community 25`?**
  _High betweenness centrality (0.062) - this node is a cross-community bridge._
- **Why does `SwiftUI` connect `Community 14` to `Community 13`, `Community 18`, `Community 32`, `Community 35`, `Community 38`, `Community 45`, `Community 48`, `Community 49`, `Community 50`, `Community 60`, `Community 61`, `Community 62`, `Community 63`, `Community 66`, `Community 72`, `Community 73`, `Community 74`, `Community 78`, `Community 79`, `Community 82`, `Community 83`?**
  _High betweenness centrality (0.053) - this node is a cross-community bridge._
- **Why does `Foundation` connect `Community 38` to `Community 3`, `Community 69`, `Community 6`, `Community 71`, `Community 37`, `Community 7`, `Community 10`, `Community 42`, `Community 12`, `Community 77`, `Community 46`, `Community 13`, `Community 44`, `Community 81`, `Community 52`, `Community 54`, `Community 25`?**
  _High betweenness centrality (0.045) - this node is a cross-community bridge._
- **Are the 3 inferred relationships involving `StudioSession` (e.g. with `PortraitPreviewSink` and `PreviewJPEGSink`) actually correct?**
  _`StudioSession` has 3 INFERRED edges - model-reasoned connections that need verification._
- **What connects `cameraUnavailable`, `cannotAddInput`, `formatUnavailable` to the rest of the system?**
  _243 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `Community 0` be split into smaller, more focused modules?**
  _Cohesion score 0.05894736842105263 - nodes in this community are weakly interconnected._
- **Should `Community 1` be split into smaller, more focused modules?**
  _Cohesion score 0.060153776571687016 - nodes in this community are weakly interconnected._