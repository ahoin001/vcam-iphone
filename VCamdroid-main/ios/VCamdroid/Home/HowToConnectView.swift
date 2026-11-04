import SwiftUI

struct HowToConnectView: View {
    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 18) {
                helpBlock(title: .howToConnectWiFiTitle, body: .howToConnectWiFi)
                helpBlock(title: .howToConnectUSBTitle, body: .howToConnectUSB)
                helpBlock(title: .howToConnectDeskTitle, body: .howToConnectDesk)
                helpBlock(title: .howToConnectOptionalTitle, body: .howToConnectOptional)
                helpBlock(title: .howToConnectLongTitle, body: .howToConnectCurtain)
            }
            .padding(24)
            .frame(maxWidth: .infinity, alignment: .leading)
        }
        .background(StudioTheme.paper)
        .navigationTitle(Text(.howToConnectTitle))
        .scrollIndicators(.hidden)
    }

    private func helpBlock(title: LocalizedStringResource, body: LocalizedStringResource) -> some View {
        VStack(alignment: .leading, spacing: 6) {
            Text(title)
                .font(.headline)
            Text(body)
                .foregroundStyle(StudioTheme.ink2)
                .fixedSize(horizontal: false, vertical: true)
        }
        .padding(22)
        .frame(maxWidth: .infinity, alignment: .leading)
        .background(StudioTheme.surface, in: .rect(cornerRadius: 28))
        .accessibilityElement(children: .combine)
    }
}
