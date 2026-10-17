import CoreMedia
import Foundation

actor CaptureEngine {
    private let core: CaptureCore
    private var configuration = CaptureConfiguration(settings: StudioSettings(), rotationAngle: 0)

    init(previewSession: PreviewSession) {
        core = CaptureCore(previewSession: previewSession)
    }

    func setFrameHandler(_ handler: @escaping @Sendable (CVPixelBuffer, CMTime) -> Void) {
        core.setFrameHandler(handler)
    }

    func setPortrait(_ command: PortraitCommand) {
        core.setPortrait(command)
    }

    func start(_ configuration: CaptureConfiguration) async throws {
        self.configuration = configuration
        let core = core
        try await core.perform {
            try core.start(configuration)
        }
    }

    func stop() async {
        let core = core
        try? await core.perform {
            core.stop()
        }
    }

    func update(_ configuration: CaptureConfiguration) async throws {
        let rebuild = configuration.lens != self.configuration.lens
            || configuration.width != self.configuration.width
            || configuration.height != self.configuration.height
            || configuration.fps != self.configuration.fps
        self.configuration = configuration
        let core = core
        try await core.perform {
            try core.update(configuration, rebuild: rebuild)
        }
    }

    func focus(at devicePoint: CGPoint) async {
        let core = core
        try? await core.perform {
            core.focus(at: devicePoint)
        }
    }
}
