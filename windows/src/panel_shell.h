#pragma once

// The companion's toolbar, connect screen, and setup sheet. panel_main.cpp loads it with
// NavigateToString and drives it through chrome.webview messages.
// Keep this file ASCII (use HTML entities and \u escapes) and each literal under 16 KB for MSVC.

namespace panel_shell {

inline constexpr char kHead[] = R"SHELL(<!DOCTYPE html>
<html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>VCamdroid</title>
<style>
)SHELL";

inline constexpr char kStyleBase[] = R"SHELL(
:root {
  color-scheme: dark;
  --bg: #121416;
  --bg-soft: #171a1d;
  --surface: rgba(28, 32, 36, 0.88);
  --well: rgba(0, 0, 0, 0.2);
  --line: rgba(255, 255, 255, 0.07);
  --line-strong: rgba(255, 255, 255, 0.13);
  --hover: rgba(255, 255, 255, 0.06);
  --text: #f2efe8;
  --muted: #a39e95;
  --accent: #d9a04a;
  --accent-hover: #e4ae5e;
  --accent-soft: rgba(217, 160, 74, 0.15);
  --accent-ink: #1a1408;
  --live: #6fcf97;
  --warn: #e0a070;
  --warn-text: #f0c6a2;
  --error: #ef8170;
  --bar: 60px;
  --ease: cubic-bezier(0.22, 1, 0.36, 1);
  --font: "Segoe UI Variable Text", "Segoe UI Variable", "Segoe UI", system-ui, sans-serif;
  --font-display: "Segoe UI Variable Display", "Segoe UI Variable", "Segoe UI", system-ui, sans-serif;
}
html.mica { --surface: rgba(30, 33, 37, 0.72); --well: rgba(0, 0, 0, 0.24); }
* { box-sizing: border-box; }
html, body { height: 100%; margin: 0; }
body {
  overflow: hidden;
  background:
    radial-gradient(120% 80% at 50% -10%, rgba(217, 160, 74, 0.07), transparent 55%),
    linear-gradient(180deg, var(--bg-soft), var(--bg));
  color: var(--text);
  font: 14px/1.45 var(--font);
  font-variant-numeric: tabular-nums;
  -webkit-font-smoothing: antialiased;
  user-select: none;
  cursor: default;
}
html.mica body { background: transparent; }
h1, h2, p { margin: 0; }
b { font-weight: 600; color: var(--text); }
button, input { font: inherit; color: inherit; }
svg {
  width: 18px; height: 18px; flex: none;
  fill: none; stroke: currentColor; stroke-width: 1.7; stroke-linecap: round; stroke-linejoin: round;
}
[hidden] { display: none !important; }
:focus-visible { outline: 2px solid var(--accent); outline-offset: 2px; }

.bar {
  height: var(--bar);
  display: flex; align-items: center; gap: 16px;
  padding: 0 14px 0 20px;
  border-bottom: 1px solid var(--line);
}
.status { flex: 1; min-width: 0; display: flex; align-items: center; gap: 12px; }
.dot {
  width: 9px; height: 9px; flex: none; border-radius: 50%;
  background: var(--muted);
  transition: background-color 240ms var(--ease);
}
[data-tone="ready"] .dot, [data-tone="busy"] .dot { background: var(--accent); }
[data-tone="live"] .dot { background: var(--live); }
[data-tone="warn"] .dot { background: var(--warn); }
[data-tone="error"] .dot { background: var(--error); }
[data-tone="busy"] .dot { animation: breathe 1.6s ease-in-out infinite; }
.status-text { flex: 1; min-width: 0; display: grid; grid-template-columns: minmax(0, 1fr); gap: 1px; }
.status-line { display: flex; align-items: center; gap: 8px; min-width: 0; }
.status-line strong { font-size: 14px; font-weight: 600; white-space: nowrap; }
.chips { display: flex; gap: 6px; min-width: 0; overflow: hidden; }
.chip {
  padding: 0 8px; border-radius: 999px;
  background: var(--hover); color: var(--muted);
  font-size: 12px; line-height: 19px; white-space: nowrap;
}
.status-detail {
  color: var(--muted); font-size: 12px; line-height: 16px;
  display: -webkit-box; -webkit-line-clamp: 2; -webkit-box-orient: vertical; overflow: hidden;
}
.notice-row {
  display: flex; align-items: center; gap: 8px; min-width: 0;
}
.notice-row[hidden] { display: none; }
.notice {
  margin: 0; min-width: 0;
  color: var(--warn-text); font-size: 12px; font-weight: 600; line-height: 16px;
  white-space: nowrap; overflow: hidden; text-overflow: ellipsis;
}
.btn.link {
  height: auto; padding: 0; flex: none;
  border: 0; background: transparent; color: var(--accent);
  font-size: 12px; font-weight: 600; white-space: nowrap; cursor: pointer;
}
.btn.link:hover { background: transparent; color: var(--accent-hover); }
.btn.link:active { transform: none; }
.actions { display: flex; align-items: center; gap: 8px; flex: none; }
.monitors { display: flex; gap: 6px; }
.monitors[hidden] { display: none; }
.monitors .btn[aria-pressed="true"] { background: var(--accent-soft); color: var(--text); }
.obs {
  height: 34px; display: flex; align-items: center; gap: 8px;
  padding: 0 3px 0 12px; border-radius: 999px;
  border: 1px solid var(--line); background: var(--well);
}
.obs-label { color: var(--muted); font-size: 12px; font-weight: 600; letter-spacing: 0.04em; }
.obs-address { font-size: 12px; }

.btn {
  height: 34px; padding: 0 16px;
  display: inline-flex; align-items: center; justify-content: center; gap: 8px;
  border-radius: 999px; border: 1px solid var(--line-strong); background: var(--hover);
  font-size: 14px; font-weight: 600; white-space: nowrap; cursor: pointer;
  transition: background-color 160ms var(--ease), border-color 160ms var(--ease),
    transform 120ms var(--ease), opacity 160ms var(--ease);
}
.btn:hover { background: rgba(255, 255, 255, 0.1); }
.btn:active { transform: scale(0.97); }
.btn.primary { border-color: transparent; background: var(--accent); color: var(--accent-ink); }
.btn.primary:hover { background: var(--accent-hover); }
.btn.small { height: 26px; padding: 0 11px; font-size: 12px; }
.btn.wide { width: 100%; height: 44px; font-size: 14px; }
.btn:disabled, .btn.primary:disabled { opacity: 0.45; cursor: default; transform: none; background: var(--accent); }
.btn:not(.primary):disabled { background: var(--hover); }
.setup .badge { width: 7px; height: 7px; border-radius: 50%; background: var(--live); }
.setup[data-alert="true"] .badge { background: var(--warn); }
.icon-btn {
  width: 34px; height: 34px; display: grid; place-items: center;
  border: 0; border-radius: 999px; background: transparent; color: var(--muted); cursor: pointer;
  transition: background-color 160ms var(--ease), color 160ms var(--ease), transform 120ms var(--ease);
}
.icon-btn:hover { background: var(--hover); color: var(--text); }
.icon-btn:active { transform: scale(0.94); }
.icon-btn[aria-pressed="true"] { background: var(--accent-soft); color: var(--accent); }
.text-btn {
  height: 28px; padding: 0 10px;
  display: inline-flex; align-items: center; gap: 6px;
  border: 0; border-radius: 999px; background: transparent;
  color: var(--accent); font-size: 12px; font-weight: 600; cursor: pointer;
  transition: background-color 160ms var(--ease);
}
.text-btn:hover { background: var(--accent-soft); }
.text-btn svg { width: 15px; height: 15px; }
.text-btn.spin svg { animation: spin 900ms linear infinite; }

@keyframes breathe { 50% { opacity: 0.35; } }
@keyframes spin { to { transform: rotate(360deg); } }
@keyframes ripple { from { transform: scale(0.5); opacity: 0.6; } to { transform: scale(1.25); opacity: 0; } }
@keyframes rise { from { opacity: 0; transform: translateY(8px); } }
@media (prefers-reduced-motion: reduce) {
  *, *::before, *::after { animation: none !important; transition-duration: 0ms !important; }
}
@media (max-width: 1040px) { .obs-label { display: none; } }
)SHELL";

inline constexpr char kStyleViews[] = R"SHELL(
.idle {
  height: calc(100% - var(--bar));
  display: grid; grid-template-columns: minmax(0, 1fr) minmax(340px, 400px); gap: 16px;
  padding: 16px;
}
body.connected .idle, body.sheet-open .idle, body.guide-open .idle { display: none; }
.left { min-height: 0; display: flex; flex-direction: column; gap: 16px; }
.stage {
  position: relative; flex: 1; min-height: 160px;
  display: grid; place-content: center; justify-items: center; gap: 16px;
  overflow: hidden; border-radius: 18px; border: 1px solid var(--line);
  background: linear-gradient(180deg, rgba(20, 24, 28, 0.92), rgba(12, 14, 16, 0.96));
  color: var(--muted); font-size: 14px;
}
.stage::before {
  content: ""; position: absolute; inset: 0;
  background: radial-gradient(45% 50% at 50% 46%, rgba(217, 160, 74, 0.09), transparent 70%);
}
.stage-art, .stage p { position: relative; }
.stage-art svg { width: 168px; height: 108px; stroke-width: 1.4; color: rgba(242, 239, 232, 0.42); }

.pc, .panel, .sheet-card {
  border-radius: 18px; border: 1px solid var(--line); background: var(--surface);
  box-shadow: 0 1px 0 rgba(255, 255, 255, 0.04) inset, 0 16px 40px rgba(0, 0, 0, 0.28);
}
.pc { flex: none; overflow: hidden; }
.pc-head {
  width: 100%; display: flex; align-items: center; gap: 12px;
  padding: 14px 16px; border: 0; border-radius: 18px; background: transparent;
  text-align: left; cursor: pointer; transition: background-color 160ms var(--ease);
}
.pc-head:hover { background: rgba(255, 255, 255, 0.03); }
.pc-text { flex: 1; min-width: 0; display: grid; }
.pc-text strong { font-weight: 600; }
.pc-text span { color: var(--muted); font-size: 12px; white-space: nowrap; overflow: hidden; text-overflow: ellipsis; }
.pc-head > svg { color: var(--muted); transition: transform 200ms var(--ease); }
.pc-head[aria-expanded="true"] > svg { transform: rotate(180deg); }
.pc .checks { padding: 0 16px 12px 50px; }

.checks { list-style: none; margin: 0; padding: 0; display: grid; }
.check-row { display: flex; align-items: flex-start; gap: 12px; padding: 10px 0; border-top: 1px solid var(--line); }
.check-row:first-child { border-top: 0; }
.pc .check-row { gap: 10px; }
.check-text { min-width: 0; display: grid; gap: 2px; }
.check-text strong { font-size: 14px; font-weight: 600; }
.check-text span { color: var(--muted); font-size: 12px; line-height: 1.45; }
.check-row[data-state="blocked"] .check-text span { color: var(--warn-text); }
.check-text .btn { justify-self: start; margin-top: 6px; }
.mark {
  width: 22px; height: 22px; flex: none; display: grid; place-items: center;
  border-radius: 50%; border: 1.5px solid var(--line-strong); color: var(--muted);
}
.mark svg { width: 13px; height: 13px; stroke-width: 2.4; }
.mark[data-state="ready"] { border-color: transparent; background: var(--accent-soft); color: var(--accent); }
.mark[data-state="blocked"] { border-color: transparent; background: rgba(224, 160, 112, 0.16); color: var(--warn); }

.panel { min-height: 0; overflow: auto; display: flex; flex-direction: column; gap: 20px; padding: 24px; }
.panel h1, .sheet h1 { font: 600 22px/1.25 var(--font-display); letter-spacing: -0.01em; }
.lede { margin-top: 6px; color: var(--muted); font-size: 14px; }
.group { display: grid; gap: 8px; }
.group-head { display: flex; align-items: center; justify-content: space-between; margin-right: -10px; }
.group h2, .group label { color: var(--muted); font-size: 12px; font-weight: 600; }
.devices {
  list-style: none; margin: 0; padding: 4px; display: grid; gap: 2px;
  border-radius: 14px; border: 1px solid var(--line); background: var(--well);
}
.device {
  display: flex; align-items: center; gap: 12px; padding: 9px 10px;
  border-radius: 10px; cursor: pointer; outline-offset: -2px;
  transition: background-color 160ms var(--ease);
}
.device:hover { background: var(--hover); }
.device[aria-selected="true"] { background: var(--accent-soft); }
.device-icon {
  width: 34px; height: 34px; flex: none; display: grid; place-items: center;
  border-radius: 10px; background: var(--hover);
  transition: color 160ms var(--ease), background-color 160ms var(--ease);
}
.device[aria-selected="true"] .device-icon { background: rgba(217, 160, 74, 0.16); color: var(--accent); }
.device-text { flex: 1; min-width: 0; display: grid; }
.device-text strong { font-weight: 600; white-space: nowrap; overflow: hidden; text-overflow: ellipsis; }
.device-text span { color: var(--muted); font-size: 12px; }
.tick { color: var(--accent); opacity: 0; transition: opacity 160ms var(--ease); }
.device[aria-selected="true"] .tick { opacity: 1; }

.searching {
  display: grid; justify-items: center; gap: 12px; padding: 22px 18px 18px;
  border-radius: 14px; border: 1px solid var(--line); background: var(--well); text-align: center;
}
.searching strong { font-weight: 600; }
.radar { position: relative; width: 48px; height: 48px; display: grid; place-items: center; color: var(--accent); }
.radar::before, .radar::after {
  content: ""; position: absolute; inset: 0; border-radius: 50%;
  border: 1.5px solid var(--accent); opacity: 0;
  animation: ripple 2.4s var(--ease) infinite;
}
.radar::after { animation-delay: 1.2s; }
.tips { list-style: none; margin: 0; padding: 0; display: grid; gap: 6px; color: var(--muted); font-size: 12px; text-align: left; }
.tips li { display: flex; align-items: flex-start; gap: 8px; }
.tips svg { width: 16px; height: 16px; margin-top: 1px; }

.field {
  width: 100%; height: 40px; padding: 0 14px;
  border-radius: 12px; border: 1px solid var(--line-strong); background: var(--well);
  outline: none; user-select: text;
  transition: border-color 160ms var(--ease), box-shadow 160ms var(--ease);
}
.field::placeholder { color: #8c877f; }
.field:focus { border-color: var(--accent); box-shadow: 0 0 0 3px var(--accent-soft); }
.hint { color: var(--muted); font-size: 12px; }
.panel-foot { margin-top: auto; }

.sheet { height: calc(100% - var(--bar)); display: grid; place-items: start center; padding: 24px 16px; overflow: auto; }
.sheet-card { width: min(640px, 100%); display: grid; gap: 12px; padding: 24px; animation: rise 280ms var(--ease); }
.sheet-head { display: flex; align-items: flex-start; justify-content: space-between; gap: 16px; padding-bottom: 4px; }

.tips-tray {
  display: none;
  height: 168px;
  padding: 10px 20px 12px;
  border-bottom: 1px solid var(--line);
  background: var(--well);
  gap: 6px;
  overflow: auto;
  animation: rise 180ms var(--ease);
}
body.tips-open .tips-tray { display: grid; align-content: start; }
.tips-head {
  display: flex; align-items: center; justify-content: space-between; gap: 12px;
}
.tips-head h2 { font: 600 13px/1.2 var(--font-display); }
.tips-list { display: grid; gap: 8px; }
.tip {
  display: grid; gap: 4px;
  padding: 8px 0 0;
  border-top: 1px solid var(--line);
}
.tip-title { font-size: 13px; font-weight: 600; }
.tip-why { color: var(--muted); font-size: 12px; line-height: 16px; }
.tip-actions { display: flex; gap: 8px; flex-wrap: wrap; }
)SHELL";

inline constexpr char kBody[] = R"SHELL(</style></head>
<body>
<header class="bar">
  <div class="status" id="status" data-tone="idle">
    <span class="dot" aria-hidden="true"></span>
    <div class="status-text" role="status" aria-live="polite">
      <div class="status-line"><strong id="status-title">Starting</strong><span class="chips" id="chips"></span></div>
      <div class="notice-row" id="notice-row" hidden>
        <p class="notice" id="notice"></p>
        <button class="btn link" id="tips-toggle" type="button" hidden>See fix</button>
      </div>
      <p class="status-detail" id="status-detail"></p>
    </div>
  </div>
  <div class="actions">
    <div class="obs" title="Address for the optional DroidCam OBS source">
      <span class="obs-label">OBS</span>
      <span class="obs-address" id="obs-address">127.0.0.1:4747</span>
      <button class="btn small" id="copy" type="button">Copy</button>
    </div>
    <div class="monitors" id="monitors" hidden>
      <button class="btn small" id="peak" type="button" aria-pressed="false">Peaking</button>
      <button class="btn small" id="zebra" type="button" aria-pressed="false">Zebra</button>
      <button class="btn small" id="loupe" type="button" aria-pressed="false" title="Show the center at full size">1:1</button>
    </div>
    <button class="btn" id="heat-open" type="button">Heat</button>
    <button class="btn" id="guide-open" type="button">Guide</button>
    <button class="btn setup" id="setup" type="button" hidden><span class="badge" aria-hidden="true"></span>Setup</button>
    <button class="icon-btn" id="pin" type="button" aria-pressed="true" aria-label="Keep window on top" title="Keep window on top">
      <svg viewBox="0 0 24 24" aria-hidden="true"><path d="M9 3.5h6"/><path d="M10 3.5V9l-3 4h10l-3-4V3.5"/><path d="M12 13v7.5"/></svg>
    </button>
    <button class="btn primary" id="reconnect" type="button" hidden>Connect</button>
    <button class="btn" id="disconnect" type="button" hidden>Disconnect</button>
  </div>
</header>
<section class="tips-tray" id="tips" hidden>
  <div class="tips-head">
    <h2>Why frames are dropping</h2>
    <button class="btn small" id="tips-close" type="button">Close</button>
  </div>
  <div class="tips-list" id="tips-list"></div>
</section>

<main class="idle">
  <div class="left">
    <section class="pc">
      <button class="pc-head" id="pc-toggle" type="button" aria-expanded="false" aria-controls="pc-checks">
        <span class="mark" id="pc-mark"></span>
        <span class="pc-text"><strong id="pc-title">Checking this PC</strong><span id="pc-sub"></span></span>
        <svg viewBox="0 0 24 24" aria-hidden="true"><path d="m6 9 6 6 6-6"/></svg>
      </button>
      <ul class="checks" id="pc-checks" hidden></ul>
    </section>
    <section class="stage" aria-hidden="true">
      <div class="stage-art">
        <svg viewBox="0 0 160 104"><rect x="18" y="16" width="124" height="64" rx="13"/><circle cx="41" cy="48" r="10"/><circle cx="41" cy="48" r="3.5"/><path d="M129 38v20"/><path d="M80 80v12"/><path d="M62 98l18-6 18 6"/></svg>
      </div>
      <p>The picture opens here after you connect.</p>
    </section>
  </div>

  <section class="panel">
    <div>
      <h1>Connect your iPhone</h1>
      <p class="lede">Open VCamdroid on the phone and leave the camera screen in front. It shows up here over USB or Wi-Fi.</p>
    </div>
    <div class="group">
      <div class="group-head">
        <h2 id="phones-label">Phones</h2>
        <button class="text-btn" id="refresh" type="button">
          <svg viewBox="0 0 24 24" aria-hidden="true"><path d="M20 11.5A8 8 0 1 1 17.7 6"/><path d="M20 4v5h-5"/></svg><span>Refresh</span>
        </button>
      </div>
      <ul class="devices" id="devices" role="listbox" aria-labelledby="phones-label" hidden></ul>
      <div class="searching" id="searching">
        <span class="radar" aria-hidden="true">
          <svg viewBox="0 0 24 24"><rect x="7" y="2.5" width="10" height="19" rx="2.5"/><path d="M11 18.5h2"/></svg>
        </span>
        <strong>Looking for your iPhone&hellip;</strong>
        <ul class="tips">
          <li><svg viewBox="0 0 24 24" aria-hidden="true"><rect x="9" y="2.5" width="6" height="5" rx="1"/><path d="M7 7.5h10V12a5 5 0 0 1-10 0z"/><path d="M12 17v4.5"/></svg><span>Plug in a data cable, unlock the phone, and tap Trust.</span></li>
          <li><svg viewBox="0 0 24 24" aria-hidden="true"><path d="M2.5 9.2a14 14 0 0 1 19 0"/><path d="M5.6 12.6a9.5 9.5 0 0 1 12.8 0"/><path d="M8.8 15.9a5 5 0 0 1 6.4 0"/><path d="M12 19.3h.01"/></svg><span>Or keep the phone on the same Wi-Fi as this PC.</span></li>
        </ul>
      </div>
    </div>
    <form class="group" id="address-form" autocomplete="off">
      <label for="address">Wi-Fi address</label>
      <input class="field" id="address" type="text" placeholder="192.168.1.20:4747" spellcheck="false">
      <p class="hint">Type the address shown on the phone if it isn&rsquo;t listed.</p>
    </form>
    <div class="panel-foot">
      <button class="btn primary wide" id="connect" type="button">Connect</button>
    </div>
  </section>
</main>

<section class="sheet" id="sheet" hidden>
  <div class="sheet-card" role="dialog" aria-labelledby="sheet-title">
    <div class="sheet-head">
      <div>
        <h1 id="sheet-title">Setup check</h1>
        <p class="lede">What this PC needs before other apps can select VCamdroid.</p>
      </div>
      <button class="btn" id="sheet-done" type="button">Done</button>
    </div>
    <ul class="checks" id="sheet-checks"></ul>
    <div class="group" id="companion-update">
      <p class="hint" id="update-copy">This build has not checked for updates yet.</p>
      <button class="btn" id="check-update" type="button">Check for updates</button>
      <button class="btn primary" id="apply-update" type="button" hidden>Update</button>
    </div>
    <form class="group" id="obs-password-form" autocomplete="off">
      <label for="obs-password">OBS WebSocket password</label>
      <input class="field" id="obs-password" type="password" spellcheck="false" placeholder="Only if OBS asks for one">
      <p class="hint">In OBS: Tools, WebSocket Server Settings. Leave this blank when OBS has no password. While OBS is recording or streaming, the phone tally turns red. Studio mode alone turns it green.</p>
    </form>
  </div>
</section>
)SHELL";

inline constexpr char kScript[] = R"SHELL(<script>
(() => {
  const webview = window.chrome && window.chrome.webview;
  const post = (message) => { if (webview) webview.postMessage(message); };
  const $ = (id) => document.getElementById(id);
  const ICON = {
    usb: '<rect x="9" y="2.5" width="6" height="5" rx="1"/><path d="M7 7.5h10V12a5 5 0 0 1-10 0z"/><path d="M12 17v4.5"/>',
    wifi: '<path d="M2.5 9.2a14 14 0 0 1 19 0"/><path d="M5.6 12.6a9.5 9.5 0 0 1 12.8 0"/><path d="M8.8 15.9a5 5 0 0 1 6.4 0"/><path d="M12 19.3h.01"/>',
    check: '<path d="m5 12.5 4.5 4.5L19 7.5"/>',
    alert: '<path d="M12 7v6"/><path d="M12 17h.01"/>',
    wait: '<path d="M12 12h.01"/>'
  };
  const addressInput = $("address");
  const connectButton = $("connect");
  const deviceList = $("devices");
  let selected = "";
  let signature = "";
  let lastAddress = null;
  let typed = false;
  let pending = false;
  let manualScan = false;
  let pcOpen = null;
  let copyTimer = 0;
  let tipsWereOpen = false;

  function icon(name) {
    const holder = document.createElement("span");
    holder.innerHTML = '<svg viewBox="0 0 24 24" aria-hidden="true">' + ICON[name] + "</svg>";
    return holder.firstChild;
  }

  function node(tag, className, text) {
    const element = document.createElement(tag);
    if (className) element.className = className;
    if (text !== undefined) element.textContent = text;
    return element;
  }

  function mark(state, id) {
    const element = node("span", "mark");
    if (id) element.id = id;
    element.dataset.state = state;
    element.append(icon(state === "ready" ? "check" : state === "blocked" ? "alert" : "wait"));
    return element;
  }

  function renderChecks(list, checks) {
    list.replaceChildren(...checks.map((check) => {
      const item = node("li", "check-row");
      item.dataset.state = check.state;
      const text = node("span", "check-text");
      text.append(node("strong", "", check.title), node("span", "", check.detail));
      if (check.action === "install") {
        const button = node("button", "btn primary small", "Install camera");
        button.type = "button";
        button.dataset.action = "install";
        text.append(button);
      } else if (check.action === "update") {
        const button = node("button", "btn primary small", "Update");
        button.type = "button";
        button.dataset.action = "update";
        text.append(button);
      }
      item.append(mark(check.state), text);
      return item;
    }));
  }

  function renderPc(checks) {
    const pc = checks.filter((check) => check.pc);
    const ready = pc.length > 0 && pc.every((check) => check.state === "ready");
    const blocked = pc.some((check) => check.state === "blocked");
    const first = pc.find((check) => check.state !== "ready");
    $("pc-title").textContent = ready ? "This PC is ready" : blocked ? "This PC needs attention" : "Checking this PC";
    $("pc-sub").textContent = ready ? "Other apps can select the VCamdroid camera." : (first ? first.detail : "");
    $("pc-mark").replaceWith(mark(ready ? "ready" : blocked ? "blocked" : "waiting", "pc-mark"));
    const open = pcOpen === null ? !ready : pcOpen;
    $("pc-toggle").setAttribute("aria-expanded", String(open));
    $("pc-checks").hidden = !open;
    renderChecks($("pc-checks"), pc);
  }

  function renderTipList(tips, justOpened) {
    const list = $("tips-list");
    list.replaceChildren(...(tips || []).map((tip) => {
      const row = node("div", "tip");
      row.dataset.id = tip.id || "";
      const actions = node("div", "tip-actions");
      if (tip.action) {
        const fix = node("button", "btn small primary tip-fix", tip.done ? "Done" : tip.action);
        fix.type = "button";
        fix.disabled = !!tip.done;
        fix.addEventListener("click", () => post({ type: "tipAction", id: tip.id }));
        actions.append(fix);
      }
      if (!tip.done) {
        const skip = node("button", "btn small", "Not now");
        skip.type = "button";
        skip.addEventListener("click", () => post({ type: "tipDismiss", id: tip.id }));
        actions.append(skip);
      }
      row.append(node("div", "tip-title", tip.title || ""), node("p", "tip-why", tip.why || ""), actions);
      return row;
    }));
    if (justOpened) {
      const first = list.querySelector(".tip-fix") || $("tips-close");
      if (first) first.focus();
    }
  }

  function deviceRow(device) {
    const item = node("li", "device");
    item.setAttribute("role", "option");
    item.dataset.key = device.key;
    const badge = node("span", "device-icon");
    badge.append(icon(device.kind === "usb" ? "usb" : "wifi"));
    const text = node("span", "device-text");
    text.append(node("strong", "", device.name), node("span", "", device.detail));
    const tick = node("span", "tick");
    tick.append(icon("check"));
    item.append(badge, text, tick);
    return item;
  }

  function updateConnect() {
    connectButton.disabled = pending || (!selected && !addressInput.value.trim());
    connectButton.textContent = pending ? "Connecting\u2026" : "Connect";
  }

  function paintSelection() {
    const first = deviceList.firstElementChild;
    for (const item of deviceList.children) {
      const on = item.dataset.key === selected;
      item.setAttribute("aria-selected", String(on));
      item.tabIndex = on || (!selected && item === first) ? 0 : -1;
    }
    updateConnect();
  }

  function renderDevices(devices, selectedKey) {
    const next = JSON.stringify(devices);
    if (next !== signature) {
      signature = next;
      deviceList.replaceChildren(...devices.map(deviceRow));
    }
    selected = devices.some((device) => device.key === selectedKey) ? selectedKey : "";
    deviceList.hidden = devices.length === 0;
    $("searching").hidden = devices.length > 0;
    paintSelection();
  }

  function select(key) {
    if (key === selected) return;
    selected = key;
    typed = false;
    paintSelection();
    post({ type: "select", key });
  }

  function connect() {
    const address = addressInput.value.trim();
    if (!selected && !address) {
      addressInput.focus();
      return;
    }
    pending = true;
    updateConnect();
    post({ type: "connect", key: selected, address });
  }

  function render(state) {
    pending = false;
    const connected = !!state.connected;
    const status = state.status || {};
    const checks = state.checks || [];
    document.documentElement.classList.toggle("mica", !!state.mica);
    document.body.classList.toggle("connected", connected);
    document.body.classList.toggle("sheet-open", !!state.sheet);
    document.body.classList.toggle("guide-open", !!state.guide);
    $("guide").hidden = !state.guide;

    $("status").dataset.tone = status.tone || "idle";
    const noticeText = status.notice || "";
    $("notice").textContent = noticeText;
    $("notice-row").hidden = !noticeText && !state.tipsOpen;
    $("tips-toggle").hidden = !state.tipsReady;
    $("status-title").textContent = status.title || "";
    const detail = $("status-detail");
    detail.textContent = status.detail || "";
    detail.title = status.detail || "";
    $("chips").replaceChildren(...(status.chips || []).map((chip) => node("span", "chip", chip)));
    $("obs-address").textContent = state.obs || "";
    $("pin").setAttribute("aria-pressed", String(!!state.onTop));
    $("disconnect").hidden = !connected;
    $("monitors").hidden = !connected;
    $("peak").setAttribute("aria-pressed", String(!!state.peaking));
    $("zebra").setAttribute("aria-pressed", String(!!state.zebra));
    $("loupe").setAttribute("aria-pressed", String(!!state.loupe));
    if (document.activeElement !== $("obs-password") && state.obsPassword !== undefined) {
      $("obs-password").value = state.obsPassword || "";
    }
    $("reconnect").hidden = !(connected && status.tone === "error");

    const setup = $("setup");
    setup.hidden = !connected;
    setup.dataset.alert = String(checks.some((check) => check.state === "blocked"));
    $("sheet").hidden = !state.sheet;
    renderChecks($("sheet-checks"), checks);
    renderPc(checks);
    const update = state.update || {};
    $("update-copy").textContent = update.detail || ("This build is " + (update.current || "dev") + ".");
    $("check-update").disabled = !!update.busy;
    $("apply-update").hidden = !update.available;
    $("apply-update").disabled = !!update.busy;

    if (state.address !== lastAddress) {
      lastAddress = state.address;
      if (document.activeElement !== addressInput) addressInput.value = state.address || "";
    }
    if (!state.scanning) manualScan = false;
    $("refresh").classList.toggle("spin", manualScan);
    renderDevices(state.devices || [], state.selected || "");
    document.body.classList.toggle("tips-open", !!state.tipsOpen);
    $("tips").hidden = !state.tipsOpen;
    renderTipList(state.tips || [], !!state.tipsOpen && !tipsWereOpen);
    tipsWereOpen = !!state.tipsOpen;
  }

  deviceList.addEventListener("click", (event) => {
    const item = event.target.closest(".device");
    if (item) select(item.dataset.key);
  });
  deviceList.addEventListener("dblclick", (event) => {
    if (event.target.closest(".device")) connect();
  });
  deviceList.addEventListener("keydown", (event) => {
    const items = [...deviceList.children];
    if (event.key === "ArrowDown" || event.key === "ArrowUp") {
      event.preventDefault();
      const index = items.findIndex((item) => item.dataset.key === selected);
      const step = event.key === "ArrowDown" ? 1 : -1;
      const next = items[Math.min(items.length - 1, Math.max(0, index + step))];
      if (next) {
        select(next.dataset.key);
        next.focus();
      }
    } else if (event.key === " ") {
      event.preventDefault();
      const item = event.target.closest(".device");
      if (item) select(item.dataset.key);
    } else if (event.key === "Enter") {
      event.preventDefault();
      connect();
    }
  });
  addressInput.addEventListener("input", () => {
    typed = true;
    if (addressInput.value.trim() && selected) {
      selected = "";
      paintSelection();
      post({ type: "select", key: "" });
    } else {
      updateConnect();
    }
  });
  $("address-form").addEventListener("submit", (event) => {
    event.preventDefault();
    connect();
  });
  connectButton.addEventListener("click", connect);
  $("refresh").addEventListener("click", () => {
    manualScan = true;
    $("refresh").classList.add("spin");
    post({ type: "refresh" });
  });
  $("copy").addEventListener("click", () => {
    post({ type: "copy" });
    const button = $("copy");
    button.textContent = "Copied";
    clearTimeout(copyTimer);
    copyTimer = setTimeout(() => { button.textContent = "Copy"; }, 1600);
  });
  $("pin").addEventListener("click", () => {
    post({ type: "onTop", value: $("pin").getAttribute("aria-pressed") !== "true" });
  });
  $("disconnect").addEventListener("click", () => post({ type: "disconnect" }));
  $("reconnect").addEventListener("click", () => post({ type: "reconnect" }));
  $("tips-toggle").addEventListener("click", () => post({ type: "tips", open: $("tips").hidden }));
  $("tips-close").addEventListener("click", () => post({ type: "tips", open: false }));
  $("setup").addEventListener("click", () => post({ type: "sheet", open: true }));
  $("sheet-done").addEventListener("click", () => post({ type: "sheet", open: false }));
  $("check-update").addEventListener("click", () => post({ type: "checkUpdate" }));
  $("apply-update").addEventListener("click", () => post({ type: "applyUpdate" }));
  document.addEventListener("click", (event) => {
    const button = event.target.closest("[data-action]");
    if (!button) return;
    if (button.dataset.action === "install") {
      button.disabled = true;
      post({ type: "installCamera" });
    } else if (button.dataset.action === "update") {
      post({ type: "applyUpdate" });
    }
  });
  function clockTime(hour) {
    const whole = Math.floor(hour);
    return (whole === 0 ? 12 : whole) + (hour % 1 ? ":30" : " o'clock");
  }
  document.querySelectorAll("svg.clock").forEach((svg) => {
    const lights = svg.dataset.lights.split(",").map((part) => part.trim().split(" "));
    let beams = "";
    let lamps = "";
    for (const [hour, kind] of lights) {
      const angle = (Number(hour) / 12) * Math.PI * 2;
      const x = (32 + Math.sin(angle) * 22).toFixed(1);
      const y = (32 - Math.cos(angle) * 22).toFixed(1);
      beams += '<line class="beam ' + kind + '" x1="' + x + '" y1="' + y + '" x2="32" y2="32"/>';
      lamps += '<circle class="lamp ' + kind + '" cx="' + x + '" cy="' + y + '" r="4.5"/>';
    }
    svg.setAttribute("viewBox", "0 0 64 64");
    svg.setAttribute("role", "img");
    svg.setAttribute("aria-label", lights.map(([hour, kind]) => (kind === "key" ? "Key" : "Second light") + " at " + clockTime(Number(hour))).join(", "));
    svg.innerHTML = '<circle class="dial" cx="32" cy="32" r="27"/>' + beams + lamps
      + '<rect class="cam" x="28" y="2.5" width="8" height="5" rx="1.5"/><circle class="you" cx="32" cy="32" r="5"/>';
  });
  function monitor(id, tool) {
    $(id).addEventListener("click", () => {
      post({ type: "monitor", tool, value: $(id).getAttribute("aria-pressed") !== "true" });
    });
  }
  monitor("peak", "peaking");
  monitor("zebra", "zebra");
  monitor("loupe", "loupe");
  $("obs-password").addEventListener("change", () => {
    post({ type: "obsPassword", value: $("obs-password").value });
  });
  function showGuideTab(name) {
    document.querySelectorAll("#guide .tabs button").forEach((item) => item.classList.toggle("on", item.dataset.tab === name));
    document.querySelectorAll("#guide .guide-panel").forEach((panel) => {
      panel.classList.toggle("on", panel.dataset.panel === name);
    });
  }
  $("guide-open").addEventListener("click", () => {
    showGuideTab("light");
    post({ type: "guide", open: true });
  });
  $("heat-open").addEventListener("click", () => {
    showGuideTab("heat");
    post({ type: "guide", open: true });
  });
  $("guide-done").addEventListener("click", () => post({ type: "guide", open: false }));
  document.querySelectorAll("#guide .tabs button").forEach((button) => {
    button.addEventListener("click", () => showGuideTab(button.dataset.tab));
  });
  $("pc-toggle").addEventListener("click", () => {
    pcOpen = $("pc-toggle").getAttribute("aria-expanded") !== "true";
    $("pc-toggle").setAttribute("aria-expanded", String(pcOpen));
    $("pc-checks").hidden = !pcOpen;
  });
  document.addEventListener("keydown", (event) => {
    if (event.key === "Escape" && !$("tips").hidden) {
      post({ type: "tips", open: false });
      return;
    }
    if (event.key === "Escape" && !$("sheet").hidden) post({ type: "sheet", open: false });
    if (event.key === "Escape" && !$("guide").hidden) post({ type: "guide", open: false });
  });
  document.addEventListener("contextmenu", (event) => {
    if (!event.target.closest("input")) event.preventDefault();
  });

  if (webview) {
    webview.addEventListener("message", (event) => render(event.data));
    post({ type: "ready" });
  }
})();
</script>
</body></html>
)SHELL";

inline constexpr char kGuideStyle[] = R"SHELL(
#guide {
  position: absolute;
  top: var(--bar);
  left: 0;
  right: 0;
  bottom: 0;
  overflow: auto;
  padding: 22px 20px 32px;
}
.guide-wrap { width: min(880px, 100%); margin: 0 auto; display: grid; gap: 16px; animation: rise 280ms var(--ease); }
.guide-head { display: flex; align-items: flex-end; justify-content: space-between; gap: 16px; }
.guide-head h1 { font: 600 28px/1.15 var(--font-display); letter-spacing: -0.02em; }
.guide-head p { color: var(--muted); font-size: 13px; margin-top: 4px; }
.tabs { display: flex; gap: 6px; flex-wrap: wrap; }
.tabs button.on { background: var(--accent); border-color: transparent; color: var(--accent-ink); }
.guide-panel { display: none; gap: 12px; }
.guide-panel.on { display: grid; }
.guide-copy { color: var(--muted); font-size: 14px; max-width: 62ch; }
.lights { display: grid; grid-template-columns: 240px minmax(0, 1fr); gap: 16px; align-items: center; }
.lights svg { width: 100%; height: auto; display: block; }
.notes { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; }
.note {
  padding: 14px 16px; border-radius: 14px;
  border: 1px solid var(--line); background: var(--surface);
}
.note strong { display: block; margin-bottom: 4px; font-size: 14px; }
.note p { color: var(--muted); font-size: 13px; }
.lesson {
  display: grid; grid-template-columns: minmax(0, 300px) minmax(0, 1fr); gap: 18px; align-items: center;
  padding: 14px; border-radius: 16px; border: 1px solid var(--line); background: var(--surface);
}
.shot { width: 100%; aspect-ratio: 4 / 3; object-fit: cover; border-radius: 12px; display: block; background: #000; }
.shot.whole { object-fit: contain; }
.lesson h2, .group-title { font: 600 18px/1.3 var(--font-display); letter-spacing: -0.01em; }
.lesson p { color: var(--muted); font-size: 14px; margin-top: 6px; }
.ways { margin: 10px 0 0; padding: 0; list-style: none; display: grid; gap: 6px; color: var(--muted); font-size: 14px; }
.ways b { color: var(--text); font-weight: 600; }
.callout { padding: 14px 16px; border-radius: 14px; border: 1px solid rgba(224, 160, 112, 0.3); background: rgba(224, 160, 112, 0.08); }
.callout strong { display: block; margin-bottom: 4px; color: var(--warn-text); }
.callout p { color: var(--muted); font-size: 14px; }
.group-title { margin-top: 6px; }
.setups { display: grid; grid-template-columns: repeat(4, minmax(0, 1fr)); gap: 12px; }
.setup {
  display: grid; gap: 8px; align-content: start;
  padding: 10px; border-radius: 16px; border: 1px solid var(--line); background: var(--surface);
}
.setup figure { position: relative; margin: 0; }
.setup img { width: 100%; aspect-ratio: 4 / 5; object-fit: cover; border-radius: 10px; display: block; background: #000; }
.setup h3 { margin: 2px 2px 0; font-size: 14px; font-weight: 600; }
.setup p { margin: 0 2px 2px; color: var(--muted); font-size: 12px; line-height: 1.5; }
.clock {
  position: absolute; right: 6px; bottom: 6px; width: 56px; height: 56px;
  border-radius: 50%; background: rgba(10, 12, 14, 0.8);
}
.clock .dial { fill: none; stroke: rgba(255, 255, 255, 0.2); stroke-width: 1; }
.clock .you { fill: #f2efe8; }
.clock .cam { fill: #a39e95; }
.clock .beam { stroke-width: 1.5; opacity: 0.65; }
.clock .key { fill: var(--accent); stroke: var(--accent); }
.clock .second { fill: #8aa0b4; stroke: #8aa0b4; }
.clock .lamp { stroke: none; }
.credits { color: var(--muted); font-size: 12px; line-height: 1.6; }
.credits a, .source { color: var(--accent); text-decoration: none; }
.credits a:hover, .source:hover { text-decoration: underline; }
@media (max-width: 980px) { .setups { grid-template-columns: repeat(2, minmax(0, 1fr)); } }
@media (max-width: 760px) { .lights, .notes, .lesson { grid-template-columns: 1fr; } }
)SHELL";

inline constexpr char kGuideBody[] = R"SHELL(
<section id="guide" hidden>
  <div class="guide-wrap">
    <div class="guide-head">
      <div>
        <h1>Guide</h1>
        <p>How streamers light a face, and which camera controls to lock.</p>
      </div>
      <button class="btn" id="guide-done" type="button">Done</button>
    </div>
    <div class="tabs" role="tablist">
      <button class="btn on" type="button" data-tab="light">Lighting</button>
      <button class="btn" type="button" data-tab="exposure">Exposure</button>
      <button class="btn" type="button" data-tab="color">Color</button>
      <button class="btn" type="button" data-tab="situations">Situations</button>
      <button class="btn" type="button" data-tab="heat">Heat</button>
    </div>
    <section class="guide-panel on" data-panel="light">
      <p class="guide-copy">Lighting does more for the picture than the camera. These are the setups from Senpai&rsquo;s lighting MasterClass, each with a real photo of the look. <a class="source" href="https://www.youtube.com/watch?v=mCxWTtDH-f4" target="_blank" rel="noopener">Watch the video</a></p>

      <article class="lesson">
        <img class="shot whole" src="{{guide:diffusion}}" alt="The same model under harsh sun on the left and a softbox on the right" loading="lazy">
        <div>
          <h2>Make the key light big and soft</h2>
          <p>The key is the light on your face. A bigger light, or one closer to you, wraps around your face and looks softer, and it can run dimmer. Left: a small, hard source. Right: the same scene with a softbox.</p>
          <ul class="ways">
            <li><b>A bigger panel.</b> The simplest fix, and the most expensive.</li>
            <li><b>A diffuser.</b> Cheap and huge, but you need somewhere to hang it in front of the light.</li>
            <li><b>Bounce it.</b> Turn a small key light toward a white wall or a hung white sheet. It needs more power, and the wall tints the light: on a tan wall, set the light cooler.</li>
          </ul>
        </div>
      </article>

      <article class="callout">
        <strong>Don&rsquo;t light both sides the same</strong>
        <p>Two equal lights, one on each side, is the setup most streaming ads show. It erases every shadow and flattens your face. Shadows at the nose, eyes, and mouth give it depth, so keep one side a little darker.</p>
      </article>

      <h2 class="group-title">Four two-light setups</h2>
      <p class="guide-copy">Picture a clock around you with 12 at the camera. The dial on each photo shows where the lights go.</p>
      <div class="setups">
        <article class="setup">
          <figure><img src="{{guide:key-fill}}" style="object-position: 50% 22%" alt="A man lit from the front left, with the right side of his face in soft shadow" loading="lazy"><svg class="clock" data-lights="10.5 key,9 second"></svg></figure>
          <h3>Key and fill</h3>
          <p>Key at 10 to 11 o&rsquo;clock. Second light at 9, turned down, so the shadow side stays darker. The everyday default, and a help in a dark room.</p>
        </article>
        <article class="setup">
          <figure><img src="{{guide:edge}}" style="object-position: 40% 30%" alt="A profile outlined by a bright rim of light against a dark background" loading="lazy"><svg class="clock" data-lights="10.5 key,4.5 second"></svg></figure>
          <h3>Key and edge</h3>
          <p>Second light behind you, opposite the key, high and out of shot. It draws a rim on hair and shoulders so you don&rsquo;t melt into a dark room. The photo shows the edge light alone.</p>
        </article>
        <article class="setup">
          <figure><img src="{{guide:butterfly}}" style="object-position: 50% 14%" alt="A portrait lit from above and in front, with a soft shadow under the chin" loading="lazy"><svg class="clock" data-lights="12 key,6 second"></svg></figure>
          <h3>Butterfly</h3>
          <p>Key straight above the camera, aimed down. Second light behind you fills what a straight-on key leaves and adds separation. A beauty look that brings out cheekbones.</p>
        </article>
        <article class="setup">
          <figure><img src="{{guide:short}}" style="object-position: 45% 40%" alt="A man looking down, lit on the side of his face turned away from the camera" loading="lazy"><svg class="clock" data-lights="3.5 key"></svg></figure>
          <h3>Short lighting</h3>
          <p>While you look at your game screen, light the side of your face turned away from the camera, at 3 or 4 o&rsquo;clock. Moody, and the light in front of your eyes can stay low.</p>
        </article>
      </div>

      <article class="lesson">
        <img class="shot" src="{{guide:window}}" style="object-position: 40% 50%" alt="A person in silhouette against a bright window" loading="lazy">
        <div>
          <h2>Light yourself more than the room</h2>
          <p>When the window or the room behind you is brighter than your face, the camera exposes for the background and you turn into a silhouette.</p>
          <ul class="ways">
            <li><b>Avoid a black void.</b> The phone turns very dark areas into grain. Lift them a little, or add practical lights: lamps and LED strips you can see in the shot.</li>
            <li><b>Interesting, not distracting.</b> The background should say something about you without pulling eyes off your face.</li>
            <li><b>Then set the camera.</b> Lock white balance to the key in Color, and keep the shutter at 1/60 in Exposure.</li>
          </ul>
        </div>
      </article>

      <p class="credits">
        Advice adapted from <a href="https://www.youtube.com/watch?v=mCxWTtDH-f4" target="_blank" rel="noopener">Senpai, &ldquo;The Secret to Lighting Your Videos and Live Streams&rdquo; (Streaming MasterClass Ep. 9)</a>.
        Photos, resized:
        <a href="https://commons.wikimedia.org/wiki/File:Portraits_with_and_without_softbox_lighting,_by_MorrowLess.jpg" target="_blank" rel="noopener">MorrowLess</a> (<a href="https://creativecommons.org/licenses/by-sa/2.0/" target="_blank" rel="noopener">CC BY-SA 2.0</a>),
        <a href="https://commons.wikimedia.org/wiki/File:Rembrandt_Lighting.JPG" target="_blank" rel="noopener">Jacopo188</a> (<a href="https://creativecommons.org/licenses/by-sa/3.0/" target="_blank" rel="noopener">CC BY-SA 3.0</a>),
        <a href="https://www.flickr.com/photos/58455346@N00/4724038449" target="_blank" rel="noopener">japrea</a> (<a href="https://creativecommons.org/licenses/by-sa/2.0/" target="_blank" rel="noopener">CC BY-SA 2.0</a>),
        <a href="https://www.flickr.com/photos/50841206@N02/14962847439" target="_blank" rel="noopener">Jay DeFehr</a> (<a href="https://creativecommons.org/licenses/by-sa/2.0/" target="_blank" rel="noopener">CC BY-SA 2.0</a>),
        <a href="https://www.flickr.com/photos/71835142@N08/8109583619" target="_blank" rel="noopener">Kieran Clarke</a> (<a href="https://creativecommons.org/licenses/by/2.0/" target="_blank" rel="noopener">CC BY 2.0</a>),
        <a href="https://www.flickr.com/photos/41346951@N05/15629546536" target="_blank" rel="noopener">Thomas Leuthard</a> (<a href="https://creativecommons.org/licenses/by/2.0/" target="_blank" rel="noopener">CC BY 2.0</a>).
      </p>
    </section>
    <section class="guide-panel" data-panel="exposure">
      <p class="guide-copy">Shutter and ISO brighten the same picture in different ways. Lock shutter for motion, then use ISO for whatever light is still missing.</p>
      <div class="notes">
        <article class="note"><strong>Shutter first</strong><p>At 30 fps use 1/60, double the frame rate. Faster looks stuttery. Slower than 1/60 smears a head turn or a hand.</p></article>
        <article class="note"><strong>ISO second</strong><p>Start near 100. Raise it if the face is still dark. Grain usually shows past about 800. Do not slow the shutter to avoid raising ISO.</p></article>
        <article class="note"><strong>Auto</strong><p>Auto plus a little EV when you move between rooms. The phone keeps natural motion and changes ISO for you.</p></article>
        <article class="note"><strong>Manual</strong><p>Use manual when the lights stay on for the whole session. Lock 1/60, then ride ISO.</p></article>
      </div>
    </section>
    <section class="guide-panel" data-panel="color">
      <p class="guide-copy">Auto white balance hunts when a monitor or a second lamp color enters the frame. Lock it to the key light.</p>
      <div class="notes">
        <article class="note"><strong>Daylight, 5200K</strong><p>Window light and neutral LEDs. The usual start for a talking head.</p></article>
        <article class="note"><strong>Incandescent, 3200K</strong><p>Warm desk lamps. If skin goes orange, the lock is warmer than the lamp.</p></article>
        <article class="note"><strong>Fluorescent, 4000K</strong><p>Office tubes and many cheap panels.</p></article>
        <article class="note"><strong>Mixed light</strong><p>A window plus a warm lamp paints the face two colors. Turn one off, then lock white balance.</p></article>
      </div>
    </section>
    <section class="guide-panel" data-panel="situations">
      <p class="guide-copy">Dial the look on the desk, then Save it as a preset for that time of day or those lamps. Leave Portrait off when NVIDIA Broadcast or OBS already cuts the background.</p>
      <div class="notes">
        <article class="note"><strong>Day, window beside you</strong><p>Back camera, 1x or 2x, Auto exposure, white balance Daylight.</p></article>
        <article class="note"><strong>Evening lamps</strong><p>Manual, shutter 1/60, ISO 200 to 800 until the face is right, white balance Incandescent.</p></article>
        <article class="note"><strong>Overhead only</strong><p>Eyes fall into shadow. Add a key in front before raising ISO. Torch is harsh and it drains the battery.</p></article>
        <article class="note"><strong>Stream or recording</strong><p>Same exposure either way. The bitrate slider is only the saved rate for an optional OBS source.</p></article>
      </div>
    </section>
    {{heat-panel}}
  </div>
</section>
)SHELL";

inline constexpr char kHeatPanel[] = R"SHELL(
<section class="guide-panel" data-panel="heat">
  <p class="guide-copy">The picture stays 1080p30. These are the things that keep a long session cool.</p>
  <div class="notes">
    <article class="note"><strong>Curtain</strong><p>Cover the phone. The preview stops drawing. Tap the black screen to bring the controls back. The iPhone brightness stays where you set it.</p></article>
    <article class="note"><strong>Portrait</strong><p>Leave Portrait off. Cut the background in NVIDIA Broadcast or OBS. Phone Portrait warms the iPhone, and it eases off on its own if the phone is already warm.</p></article>
    <article class="note"><strong>Torch and charging</strong><p>The torch and a charger add heat. A long show runs cooler on battery once the phone is already warm.</p></article>
    <article class="note"><strong>Lighter rate</strong><p>If the phone warms up, the app lightens the bitrate. The size stays 1080p30. Exposure, color, focus, and zoom stay as you set them.</p></article>
    <article class="note"><strong>Side button</strong><p>Pressing the side button pauses the stream. Unlock the phone and open VCamdroid to continue.</p></article>
  </div>
</section>
)SHELL";

}  // namespace panel_shell
