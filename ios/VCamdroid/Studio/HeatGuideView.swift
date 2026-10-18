import SwiftUI

struct HeatGuideView: View {
    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 20) {
                block(title: .heatCurtainTitle, body: .heatCurtainBody)
                block(title: .heatPortraitTitle, body: .heatPortraitBody)
                block(title: .heatTorchTitle, body: .heatTorchBody)
                block(title: .heatRateTitle, body: .heatRateBody)
                block(title: .heatSideTitle, body: .heatSideBody)
            }
            .padding()
            .frame(maxWidth: .infinity, alignment: .leading)
        }
        .navigationTitle(Text(.heatGuideTitle))
        .navigationBarTitleDisplayMode(.inline)
        .scrollIndicators(.hidden)
    }

    private func block(title: LocalizedStringResource, body: LocalizedStringResource) -> some View {
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
