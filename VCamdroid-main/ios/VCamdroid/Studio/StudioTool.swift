import Foundation

enum StudioTool: Hashable, CaseIterable, Identifiable {
    case zoom
    case exposure
    case whiteBalance
    case focus
    case portrait

    var id: Self { self }

    var label: LocalizedStringResource {
        switch self {
        case .zoom: .zoom
        case .exposure: .exposure
        case .whiteBalance: .whiteBalance
        case .focus: .focus
        case .portrait: .portrait
        }
    }

    var systemImage: String {
        switch self {
        case .zoom: "plus.magnifyingglass"
        case .exposure: "sun.max"
        case .whiteBalance: "circle.lefthalf.filled"
        case .focus: "scope"
        case .portrait: "person.and.background.dotted"
        }
    }
}
