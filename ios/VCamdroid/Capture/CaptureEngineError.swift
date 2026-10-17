import Foundation

enum CaptureEngineError: Error, Equatable {
    case cameraUnavailable
    case cannotAddInput
    case formatUnavailable
}
