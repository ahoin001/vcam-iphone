import Network

enum DiscoveryAdvertiser {
    static func service(name: String) -> NWListener.Service {
        let record = NWTXTRecord(["name": name])
        return NWListener.Service(name: name, type: "_droidcamobs._tcp", domain: nil, txtRecord: record)
    }
}
