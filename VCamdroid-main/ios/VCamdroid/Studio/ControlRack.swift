import SwiftUI

struct ControlRack: View {
    let tool: StudioTool

    var body: some View {
        switch tool {
        case .zoom:
            ZoomControl()
        case .exposure:
            ExposureControl()
        case .whiteBalance:
            WhiteBalanceControl()
        case .focus:
            FocusControl()
        }
    }
}
