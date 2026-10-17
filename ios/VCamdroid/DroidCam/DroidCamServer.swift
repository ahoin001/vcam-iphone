import Foundation
import Network

actor DroidCamServer {
    private let queue = DispatchQueue(label: "com.darusc.vcamdroid.droidcam")
    private var listener: NWListener?
    private var videoConnection: NWConnection?
    private var snapshotProvider: (@Sendable () async -> SessionSnapshot)?
    private var onVideo: (@Sendable (VideoRequest) async -> Void)?
    private var onVideoStopped: (@Sendable () async -> Void)?
    private var onUpdate: (@Sendable (SessionUpdate) async -> Bool)?
    private var controlsProvider: (@Sendable () async -> ControlState)?
    private var onControls: (@Sendable (ControlUpdate) async -> Bool)?
    private var onTally: (@Sendable (TallyState) async -> Void)?
    private var onClient: (@Sendable (String?, Bool) async -> Void)?
    private var catalogProvider: (@Sendable () async -> ResolutionCatalog)?
    private var previewProvider: (@Sendable () async -> Data?)?
    private var onListenFailed: (@Sendable () async -> Void)?

    func configure(
        snapshot: @escaping @Sendable () async -> SessionSnapshot,
        catalog: @escaping @Sendable () async -> ResolutionCatalog,
        preview: @escaping @Sendable () async -> Data?,
        onVideo: @escaping @Sendable (VideoRequest) async -> Void,
        onVideoStopped: @escaping @Sendable () async -> Void,
        onUpdate: @escaping @Sendable (SessionUpdate) async -> Bool,
        controls: @escaping @Sendable () async -> ControlState,
        onControls: @escaping @Sendable (ControlUpdate) async -> Bool,
        onTally: @escaping @Sendable (TallyState) async -> Void,
        onClient: @escaping @Sendable (String?, Bool) async -> Void,
        onListenFailed: @escaping @Sendable () async -> Void
    ) {
        snapshotProvider = snapshot
        catalogProvider = catalog
        previewProvider = preview
        self.onVideo = onVideo
        self.onVideoStopped = onVideoStopped
        self.onUpdate = onUpdate
        controlsProvider = controls
        self.onControls = onControls
        self.onTally = onTally
        self.onClient = onClient
        self.onListenFailed = onListenFailed
    }

    func start(port: Int, advertise: Bool, deviceName: String) throws {
        stopListener()
        guard let exact = UInt16(exactly: port), let nwPort = NWEndpoint.Port(rawValue: exact) else {
            throw DroidCamServerError.invalidPort
        }
        let tcp = NWProtocolTCP.Options()
        tcp.noDelay = true
        let parameters = NWParameters(tls: nil, tcp: tcp)
        parameters.allowLocalEndpointReuse = true
        parameters.includePeerToPeer = true
        let listener = try NWListener(using: parameters, on: nwPort)
        if advertise {
            listener.service = DiscoveryAdvertiser.service(name: deviceName)
        }
        listener.newConnectionHandler = { [weak self] connection in
            guard let self else { return }
            Task { await self.accept(connection) }
        }
        listener.stateUpdateHandler = { [weak self] state in
            guard case .failed = state else { return }
            Task { await self?.onListenFailed?() }
        }
        listener.start(queue: queue)
        self.listener = listener
    }

    func stop() async {
        videoConnection?.cancel()
        videoConnection = nil
        stopListener()
        await onClient?(nil, false)
    }

    func dropVideo() {
        videoConnection?.cancel()
    }

    func send(pts: UInt64, payload: Data) async {
        guard let videoConnection else { return }
        let packet = DroidCamPacketFramer.frame(pts: pts, payload: payload)
        await send(packet, on: videoConnection)
    }

    private func stopListener() {
        listener?.cancel()
        listener = nil
    }

    private func accept(_ connection: NWConnection) async {
        connection.start(queue: queue)
        guard let data = await receive(connection, maximum: 2048) else {
            connection.cancel()
            return
        }
        let fallback = await snapshotProvider?().videoMode ?? VideoMode(width: 1920, height: 1080, fps: 30)
        let request = DroidCamHTTP.parse(buffer: data, fallback: fallback)
        switch request {
        case .video(let video):
            await replaceVideo(connection, request: video)
        case .update(let update):
            let success = await onUpdate?(update) ?? false
            let snapshot = await snapshotProvider?() ?? .waiting
            await send(DroidCamHTTP.jsonReply(snapshot.json(success: success)), on: connection)
            connection.cancel()
        case .applyControls(let update):
            let success = await onControls?(update) ?? false
            let state = await controlsProvider?() ?? ControlState(settings: StudioSettings())
            await send(DroidCamHTTP.jsonReply(state.json(success: success)), on: connection)
            connection.cancel()
        case .tally(let tally):
            await onTally?(tally)
            await send(DroidCamHTTP.reply(status: 200, body: Data(), contentType: nil), on: connection)
            connection.cancel()
        case .audio:
            await send(DroidCamHTTP.reply(status: 404, body: Data(), contentType: nil), on: connection)
            connection.cancel()
        default:
            let body = await replyBody(for: request)
            await send(body, on: connection)
            connection.cancel()
        }
    }

    private func replyBody(for request: DroidCamRequest) async -> Data {
        let snapshot = await snapshotProvider?() ?? .waiting
        let catalog = await catalogProvider?() ?? ResolutionCatalog(deviceName: "Apple iPhone", back: [], front: [])
        switch request {
        case .ping:
            return DroidCamHTTP.reply(status: 200, body: Data("OK".utf8), contentType: nil)
        case .resolutions:
            return DroidCamHTTP.jsonReply(catalog.json())
        case .capabilities:
            return DroidCamHTTP.jsonReply(catalog.capabilitiesJSON())
        case .session:
            return DroidCamHTTP.jsonReply(snapshot.json())
        case .controls:
            let state = await controlsProvider?() ?? ControlState(settings: StudioSettings())
            return DroidCamHTTP.jsonReply(state.json())
        case .panel:
            return DroidCamHTTP.htmlReply(ControlPanelPage.html)
        case .preview:
            if let jpeg = await previewProvider?() {
                return DroidCamHTTP.reply(status: 200, body: jpeg, contentType: "image/jpeg")
            }
            return DroidCamHTTP.reply(status: 204, body: Data(), contentType: nil)
        case .battery:
            return DroidCamHTTP.reply(status: 200, body: Data("\(snapshot.batteryPercent)".utf8), contentType: nil)
        case .unknown:
            return DroidCamHTTP.reply(status: 200, body: Data(), contentType: nil)
        default:
            return DroidCamHTTP.reply(status: 200, body: Data(), contentType: nil)
        }
    }

    private func replaceVideo(_ connection: NWConnection, request: VideoRequest) async {
        videoConnection?.cancel()
        videoConnection = connection
        let address = Self.address(of: connection)
        let usb = Self.isLoopback(address)
        await onClient?(address, usb)
        await onVideo?(request)
        while !Task.isCancelled && videoConnection === connection {
            let more = await receive(connection, maximum: 1024)
            if more == nil { break }
        }
        if videoConnection === connection {
            videoConnection = nil
            connection.cancel()
            await onVideoStopped?()
            await onClient?(nil, false)
        }
    }

    private func receive(_ connection: NWConnection, maximum: Int) async -> Data? {
        await withCheckedContinuation { continuation in
            connection.receive(minimumIncompleteLength: 1, maximumLength: maximum) { data, _, isComplete, _ in
                if let data, !data.isEmpty {
                    continuation.resume(returning: data)
                } else if isComplete {
                    continuation.resume(returning: nil)
                } else {
                    continuation.resume(returning: nil)
                }
            }
        }
    }

    private func send(_ data: Data, on connection: NWConnection) async {
        await withCheckedContinuation { (continuation: CheckedContinuation<Void, Never>) in
            connection.send(content: data, completion: .contentProcessed { _ in
                continuation.resume()
            })
        }
    }

    private static func address(of connection: NWConnection) -> String {
        guard case let .hostPort(host, _) = connection.endpoint else { return "unknown" }
        switch host {
        case .ipv4(let address):
            return "\(address)"
        case .ipv6(let address):
            return "\(address)"
        case .name(let name, _):
            return name
        @unknown default:
            return "unknown"
        }
    }

    private static func isLoopback(_ address: String) -> Bool {
        address == "127.0.0.1" || address == "::1" || address == "localhost"
    }
}
