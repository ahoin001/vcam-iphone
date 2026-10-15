import Foundation

enum DroidCamServerError: Error, Equatable {
    case invalidPort
    case portInUse
}
