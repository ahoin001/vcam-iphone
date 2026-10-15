import Foundation

enum DeviceName {
    private static let marketingNames = [
        "iPhone17,1": "iPhone 16 Pro",
        "iPhone17,2": "iPhone 16 Pro Max",
        "iPhone17,3": "iPhone 16",
        "iPhone17,4": "iPhone 16 Plus"
    ]

    static var current: String {
        var system = utsname()
        uname(&system)
        let machine = withUnsafePointer(to: &system.machine) {
            $0.withMemoryRebound(to: CChar.self, capacity: 1) {
                String(cString: $0)
            }
        }
        let model = marketingNames[machine] ?? "iPhone"
        return "Apple \(model)"
    }
}
