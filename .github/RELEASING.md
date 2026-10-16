# Cutting a Windows release

The root workflow [`.github/workflows/release-windows.yml`](workflows/release-windows.yml) builds `VCamPanel.exe`, `VCamSource.dll`, and `usbmux_forward.exe`, zips them as **`VCamdroid-Windows-x64.zip`**, and publishes a GitHub Release. Keep that zip name stable so the README download link does not need a version bump. `VCamSource.dll` has to stay next to `VCamPanel.exe`.

## One-line release (preferred)

Commit everything on `main`, then:

```bash
./scripts/release
```

That bumps the patch (`v0.1.0` → `v0.1.1`), pushes the branch, and pushes the tag. Actions builds and attaches the zip.

Exact version:

```bash
./scripts/release v0.2.0
```

Users always download:

https://github.com/ahoin001/vcam-iphone/releases/latest/download/VCamdroid-Windows-x64.zip

## After the release

Confirm the release page lists **VCamdroid-Windows-x64.zip**. The root README already points at that latest-download URL. Only change the README if the asset name or product flow changes.
