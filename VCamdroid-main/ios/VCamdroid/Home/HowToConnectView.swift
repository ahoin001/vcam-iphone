import SwiftUI

struct HowToConnectView: View {
    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 20) {
                helpBlock(title: .howToConnectWiFiTitle, body: .howToConnectWiFi)
                helpBlock(title: .howToConnectUSBTitle, body: .howToConnectUSB)
                helpBlock(title: .howToConnectDeskTitle, body: .howToConnectDesk)
                helpBlock(title: .howToConnectLongTitle, body: .howToConnectCurtain)
            }
            .padding()
            .frame(maxWidth: .infinity, alignment: .leading)
        }
        .navigationTitle(Text(.howToConnectTitle))
        .scrollIndicators(.hidden)
    }

    private func helpBlock(title: LocalizedStringResource, body: LocalizedStringResource) -> some View {
        VStack(alignment: .leading, spacing: 6) {
            Text(title)
                .font(.headline)
            Text(body)
                .foregroundStyle(.secondary)
                .fixedSize(horizontal: false, vertical: true)
        }
        .accessibilityElement(children: .combine)
    }
}
