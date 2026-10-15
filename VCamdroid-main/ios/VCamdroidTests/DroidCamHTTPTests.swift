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

@Test func sessionUpdateQuery() {
    let request = DroidCamHTTP.parse(
        firstLine: "PUT /v1/session?width=1280&height=720&fps=30&lens=front&format=avc HTTP/1.1",
        fallback: VideoMode(width: 1920, height: 1080, fps: 30)
    )
    #expect(request == .update(SessionUpdate(width: 1280, height: 720, fps: 30, format: .avc, lens: .front)))
}

@Test func tallyRoute() {
    let request = DroidCamHTTP.parse(firstLine: "PUT /v1/tally/program/ HTTP/1.1", fallback: VideoMode(width: 1920, height: 1080, fps: 30))
    #expect(request == .tally(.program))
}
