import SwiftUI

struct ControlRack: View {
    var body: some View {
        VStack(alignment: .leading) {
            LensControl()
            ZoomControl()
            ExposureControl()
            WhiteBalanceControl()
            FocusControl()
        }
    }
}
