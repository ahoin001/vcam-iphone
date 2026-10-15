struct SessionUpdate: Equatable, Sendable {
    var width: Int?
    var height: Int?
    var fps: Int?
    var format: StreamFormat?
    var lens: Lens?
}

struct VideoRequest: Equatable, Sendable {
    var format: StreamFormat
    var width: Int
    var height: Int
}

enum DroidCamRequest: Equatable, Sendable {
    case ping
    case resolutions
    case capabilities
    case session
    case update(SessionUpdate)
    case battery
    case tally(TallyState)
    case video(VideoRequest)
    case audio
    case unknown
}
