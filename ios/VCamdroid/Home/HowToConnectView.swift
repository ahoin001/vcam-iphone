import SwiftUI

struct HowToConnectView: View {
    var onReplay: () -> Void

    private let panelURL = URL(string: "https://github.com/ahoin001/vcam-iphone/releases/latest")!

    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 20) {
                helpBlock(title: .helpPanelTitle, body: .helpPanelBody)
                Link(panelURL.absoluteString, destination: panelURL)
                    .font(.footnote.monospaced())
                helpBlock(title: .howToConnectUSBTitle, body: .howToConnectUSB)
                helpBlock(title: .howToConnectWiFiTitle, body: .howToConnectWiFi)
                helpBlock(title: .howToConnectDeskTitle, body: .howToConnectDesk)
                helpBlock(title: .helpObsTitle, body: .helpObsBody)
                helpBlock(title: .howToConnectLongTitle, body: .howToConnectCurtain)
                Button(.replaySetup, systemImage: "arrow.counterclockwise", action: onReplay)
                    .buttonStyle(PressableButtonStyle(prominent: true))
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
        .padding(16)
        .frame(maxWidth: .infinity, alignment: .leading)
        .glassEffect(.regular, in: .rect(cornerRadius: 22))
        .accessibilityElement(children: .combine)
    }
}
