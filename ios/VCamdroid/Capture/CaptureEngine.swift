import CoreMedia
import Foundation

actor CaptureEngine {
    private nonisolated let core: CaptureCore
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

    nonisolated func droppedFrames() -> Int {
        core.droppedFrames()
    }

    func start(_ configuration: CaptureConfiguration) async throws {
        self.configuration = configuration
        let core = core
        try await core.perform {
            try core.start(configuration)
            return ()
        }
    }

    func stop() async {
        let core = core
        try? await core.perform {
            core.stop()
            return ()
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
            return ()
        }
    }

    func supportsCustomWhiteBalance() async -> Bool {
        let core = core
        return (try? await core.perform { core.supportsCustomWhiteBalance() }) ?? false
    }

    func supportsCustomFocus() async -> Bool {
        let core = core
        return (try? await core.perform { core.supportsCustomFocus() }) ?? false
    }

    func focus(at devicePoint: CGPoint) async {
        let core = core
        try? await core.perform {
            core.focus(at: devicePoint)
            return ()
        }
    }
}
