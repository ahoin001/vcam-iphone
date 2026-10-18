import Foundation
import Testing
@testable import VCamdroid

@Test(arguments: [
    "GET /ping HTTP/1.1",
    "GET /resolutions HTTP/1.1",
    "GET /v1/capabilities HTTP/1.1",
    "GET /v1/session HTTP/1.1",
    "GET /battery HTTP/1.1",
    "GET /v2/audio HTTP/1.1",
    "GET /unknown HTTP/1.1"
])
func parsesControlRoutes(_ line: String) {
    let request = DroidCamHTTP.parse(firstLine: line, fallback: VideoMode(width: 1920, height: 1080, fps: 30))
    switch line {
    case "GET /ping HTTP/1.1":
        #expect(request == .ping)
        let body = DroidCamHTTP.reply(status: 200, body: Data("OK".utf8), contentType: nil)
        #expect(String(decoding: body, as: UTF8.self).hasSuffix("OK"))
    case "GET /resolutions HTTP/1.1":
        #expect(request == .resolutions)
    case "GET /v1/capabilities HTTP/1.1":
        #expect(request == .capabilities)
    case "GET /v1/session HTTP/1.1":
        #expect(request == .session)
    case "GET /battery HTTP/1.1":
        #expect(request == .battery)
    case "GET /v2/audio HTTP/1.1":
        #expect(request == .audio)
        let body = DroidCamHTTP.reply(status: 404, body: Data(), contentType: nil)
        #expect(String(decoding: body, as: UTF8.self).contains("404"))
    default:
        #expect(request == .unknown)
        let body = DroidCamHTTP.reply(status: 200, body: Data(), contentType: nil)
        #expect(String(decoding: body, as: UTF8.self).contains("Content-Length: 0"))
    }
}

@Test func videoRequestHijacksSocketWithoutHTTP() {
    let request = DroidCamHTTP.parse(
        firstLine: "GET /v5/video/hevc/1920x1080 HTTP/1.1",
        fallback: VideoMode(width: 640, height: 480, fps: 30)
    )
    #expect(request == .video(VideoRequest(format: .hevc, width: 1920, height: 1080)))
}

@Test func keywordResolutions() {
    let fallback = VideoMode(width: 1920, height: 1080, fps: 30)
    #expect(VideoRequestParser.parse(line: "GET /video/4k HTTP/1.1", fallback: fallback).width == 3840)
    #expect(VideoRequestParser.parse(line: "GET /video/720p HTTP/1.1", fallback: fallback).height == 720)
    #expect(VideoRequestParser.parse(line: "GET /video HTTP/1.1", fallback: fallback) == VideoRequest(format: .avc, width: 1920, height: 1080))
}

@Test func panelClientSelectsTheSystemCameraRequest() {
    let fallback = VideoMode(width: 1280, height: 720, fps: 60)
    let request = VideoRequestParser.parse(
        line: "GET /video?1920x1080&client=panel HTTP/1.1",
        fallback: fallback
    )
    #expect(request == VideoRequest(format: .avc, width: 1920, height: 1080, client: .panel))
    let legacy = VideoRequestParser.parse(line: "GET /video?1280x720 HTTP/1.1", fallback: fallback)
    #expect(legacy.client == .legacy)
}

@Test func sessionUpdateQuery() {
    let request = DroidCamHTTP.parse(
        firstLine: "PUT /v1/session?width=1280&height=720&fps=30&lens=front&format=avc HTTP/1.1",
        fallback: VideoMode(width: 1920, height: 1080, fps: 30)
    )
    #expect(request == .update(SessionUpdate(width: 1280, height: 720, fps: 30, format: .avc, lens: .front)))
}

@Test func linkHealthRoute() {
    let request = DroidCamHTTP.parse(
        firstLine: "PUT /v1/session?link=cannotDecode HTTP/1.1",
        fallback: VideoMode(width: 1920, height: 1080, fps: 30)
    )
    #expect(request == .update(SessionUpdate(link: .cannotDecode)))
}

@Test func tallyRoute() {
    let request = DroidCamHTTP.parse(firstLine: "PUT /v1/tally/program/ HTTP/1.1", fallback: VideoMode(width: 1920, height: 1080, fps: 30))
    #expect(request == .tally(.program))
}

@Test func controlsReadRoute() {
    let request = DroidCamHTTP.parse(
        firstLine: "GET /v1/controls HTTP/1.1",
        fallback: VideoMode(width: 1920, height: 1080, fps: 30)
    )
    #expect(request == .controls)
}

@Test func controlUpdateIsPartial() {
    let request = DroidCamHTTP.parse(
        firstLine: "PUT /v1/controls?zoom=2.4&torch=1 HTTP/1.1",
        fallback: VideoMode(width: 1920, height: 1080, fps: 30)
    )
    #expect(request == .applyControls(ControlUpdate(zoom: 2.4, torch: true)))
}

@Test func controlUpdateReadsStudioFields() {
    let request = DroidCamHTTP.parse(
        firstLine: "PUT /v1/controls?lens=front&manualExposure=0&exposureBias=-1.5&whiteBalance=daylight&manualFocus=1&focus=0.25&flip=false HTTP/1.1",
        fallback: VideoMode(width: 1920, height: 1080, fps: 30)
    )
    #expect(request == .applyControls(ControlUpdate(
        lens: .front,
        manualExposure: false,
        exposureBias: -1.5,
        whiteBalance: .daylight,
        manualFocus: true,
        focus: 0.25,
        flip: false
    )))
}

@Test func controlUpdateReadsBitrate() {
    let request = DroidCamHTTP.parse(
        firstLine: "PUT /v1/controls?bitrateKbps=6000 HTTP/1.1",
        fallback: VideoMode(width: 1920, height: 1080, fps: 30)
    )
    #expect(request == .applyControls(ControlUpdate(bitrateKbps: 6000)))
}

@Test func unknownControlValuesAreIgnored() {
    let request = DroidCamHTTP.parse(
        firstLine: "PUT /v1/controls?whiteBalance=sepia&zoom=nan HTTP/1.1",
        fallback: VideoMode(width: 1920, height: 1080, fps: 30)
    )
    #expect(request == .applyControls(ControlUpdate()))
}

@Test func presetCommands() {
    let fallback = VideoMode(width: 1920, height: 1080, fps: 30)
    let save = DroidCamHTTP.parse(firstLine: "PUT /v1/presets?name=Evening%20lamps HTTP/1.1", fallback: fallback)
    #expect(save == .presetCommand(.save("Evening lamps")))
    let apply = DroidCamHTTP.parse(firstLine: "PUT /v1/presets?apply=abc HTTP/1.1", fallback: fallback)
    #expect(apply == .presetCommand(.apply("abc")))
    let list = DroidCamHTTP.parse(firstLine: "GET /v1/presets HTTP/1.1", fallback: fallback)
    #expect(list == .presets)
    let rename = DroidCamHTTP.parse(firstLine: "PUT /v1/presets?rename=abc&name=Morning%20window HTTP/1.1", fallback: fallback)
    #expect(rename == .presetCommand(.rename("abc", "Morning window")))
}

@Test func panelIsNotAVideoStream() {
    let request = DroidCamHTTP.parse(
        firstLine: "GET /panel HTTP/1.1",
        fallback: VideoMode(width: 1920, height: 1080, fps: 30)
    )
    #expect(request == .panel)
    let body = DroidCamHTTP.htmlReply(ControlPanelPage.html)
    let text = String(decoding: body, as: UTF8.self)
    #expect(text.contains("Content-Type: text/html"))
    #expect(text.contains("/v1/controls"))
    #expect(text.contains("/v1/session"))
    #expect(text.contains("/v1/preview.jpg"))
    #expect(text.contains("NVIDIA Broadcast"))
    #expect(text.contains("Leave Portrait off"))
    #expect(!text.contains("GET /video"))
}

@Test func guidePhotoRouteAcceptsOnlyPlainNames() {
    let fallback = VideoMode(width: 1920, height: 1080, fps: 30)
    #expect(DroidCamHTTP.parse(firstLine: "GET /guide/key-fill.jpg HTTP/1.1", fallback: fallback) == .guidePhoto("key-fill"))
    #expect(DroidCamHTTP.parse(firstLine: "GET /guide/../Info.plist HTTP/1.1", fallback: fallback) == .guidePhoto(""))
    #expect(DroidCamHTTP.parse(firstLine: "GET /guide/Edge.jpg HTTP/1.1", fallback: fallback) == .guidePhoto(""))
}

@Test func previewIsNotAVideoStream() {
    let request = DroidCamHTTP.parse(
        firstLine: "GET /v1/preview.jpg HTTP/1.1",
        fallback: VideoMode(width: 1920, height: 1080, fps: 30)
    )
    #expect(request == .preview)
    let jpeg = DroidCamHTTP.reply(status: 200, body: Data([0xFF, 0xD8, 0xFF]), contentType: "image/jpeg")
    let text = String(decoding: jpeg, as: UTF8.self)
    #expect(text.contains("Content-Type: image/jpeg"))
    #expect(text.contains("Cache-Control: no-store"))
    #expect(request != .video(VideoRequest(format: .avc, width: 1920, height: 1080)))
}
