import SwiftUI

struct HowToConnectView: View {
    var body: some View {
        ScrollView {
            VStack(alignment: .leading) {
                Text(.howToConnectWiFi)
                Text(.howToConnectUSB)
                Text(.howToConnectCurtain)
            }
            .padding()
        }
        .navigationTitle(Text(.howToConnectTitle))
        .scrollIndicators(.hidden)
    }
}
