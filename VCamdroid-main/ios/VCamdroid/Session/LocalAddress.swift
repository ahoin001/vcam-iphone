import Darwin

enum LocalAddress {
    static func ipv4() -> String {
        var pointer: UnsafeMutablePointer<ifaddrs>?
        guard getifaddrs(&pointer) == 0, let first = pointer else { return "0.0.0.0" }
        defer { freeifaddrs(pointer) }

        var fallback = "0.0.0.0"
        var cursor: UnsafeMutablePointer<ifaddrs>? = first
        while let current = cursor {
            let interface = current.pointee
            cursor = interface.ifa_next
            guard let address = interface.ifa_addr, address.pointee.sa_family == UInt8(AF_INET) else { continue }
            let name = String(cString: interface.ifa_name)
            if name == "lo0" { continue }
            var host = [CChar](repeating: 0, count: Int(NI_MAXHOST))
            let result = getnameinfo(
                address,
                socklen_t(address.pointee.sa_len),
                &host,
                socklen_t(host.count),
                nil,
                0,
                NI_NUMERICHOST
            )
            guard result == 0 else { continue }
            let ip = String(cString: host)
            fallback = ip
            if name.hasPrefix("en") { return ip }
        }
        return fallback
    }
}
