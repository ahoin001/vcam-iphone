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
.actions { display: flex; align-items: center; gap: 8px; flex: none; }
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
body.connected .idle, body.sheet-open .idle { display: none; }
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
)SHELL";

inline constexpr char kBody[] = R"SHELL(</style></head>
<body>
<header class="bar">
  <div class="status" id="status" data-tone="idle">
    <span class="dot" aria-hidden="true"></span>
    <div class="status-text" role="status" aria-live="polite">
      <div class="status-line"><strong id="status-title">Starting</strong><span class="chips" id="chips"></span></div>
      <p class="status-detail" id="status-detail"></p>
    </div>
  </div>
  <div class="actions">
    <div class="obs" title="Address for the optional DroidCam OBS source">
      <span class="obs-label">OBS</span>
      <span class="obs-address" id="obs-address">127.0.0.1:4747</span>
      <button class="btn small" id="copy" type="button">Copy</button>
    </div>
    <button class="btn setup" id="setup" type="button" hidden><span class="badge" aria-hidden="true"></span>Setup</button>
    <button class="icon-btn" id="pin" type="button" aria-pressed="true" aria-label="Keep window on top" title="Keep window on top">
      <svg viewBox="0 0 24 24" aria-hidden="true"><path d="M9 3.5h6"/><path d="M10 3.5V9l-3 4h10l-3-4V3.5"/><path d="M12 13v7.5"/></svg>
    </button>
    <button class="btn primary" id="reconnect" type="button" hidden>Connect</button>
    <button class="btn" id="disconnect" type="button" hidden>Disconnect</button>
  </div>
</header>

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
      <p class="lede">Open VCamdroid on the phone and tap <b>OBS Studio</b>. It shows up here over USB or Wi-Fi.</p>
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

  function renderDevices(devices) {
    const next = JSON.stringify(devices);
    if (next !== signature) {
      signature = next;
      deviceList.replaceChildren(...devices.map(deviceRow));
    }
    if (selected && !devices.some((device) => device.key === selected)) selected = "";
    if (!selected && !(typed && addressInput.value.trim())) {
      const preferred = devices.find((device) => device.kind === "usb") || devices[0];
      selected = preferred ? preferred.key : "";
    }
    deviceList.hidden = devices.length === 0;
    $("searching").hidden = devices.length > 0;
    paintSelection();
  }

  function select(key) {
    selected = key;
    typed = false;
    paintSelection();
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

    $("status").dataset.tone = status.tone || "idle";
    $("status-title").textContent = status.title || "";
    const detail = $("status-detail");
    detail.textContent = status.detail || "";
    detail.title = status.detail || "";
    $("chips").replaceChildren(...(status.chips || []).map((chip) => node("span", "chip", chip)));
    $("obs-address").textContent = state.obs || "";
    $("pin").setAttribute("aria-pressed", String(!!state.onTop));
    $("disconnect").hidden = !connected;
    $("reconnect").hidden = !(connected && status.tone === "error");

    const setup = $("setup");
    setup.hidden = !connected;
    setup.dataset.alert = String(checks.some((check) => check.state === "blocked"));
    $("sheet").hidden = !state.sheet;
    renderChecks($("sheet-checks"), checks);
    renderPc(checks);

    if (state.address !== lastAddress) {
      lastAddress = state.address;
      if (document.activeElement !== addressInput) addressInput.value = state.address || "";
    }
    if (!state.scanning) manualScan = false;
    $("refresh").classList.toggle("spin", manualScan);
    renderDevices(state.devices || []);
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
    if (addressInput.value.trim()) selected = "";
    paintSelection();
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
  $("setup").addEventListener("click", () => post({ type: "sheet", open: true }));
  $("sheet-done").addEventListener("click", () => post({ type: "sheet", open: false }));
  $("pc-toggle").addEventListener("click", () => {
    pcOpen = $("pc-toggle").getAttribute("aria-expanded") !== "true";
    $("pc-toggle").setAttribute("aria-expanded", String(pcOpen));
    $("pc-checks").hidden = !pcOpen;
  });
  document.addEventListener("keydown", (event) => {
    if (event.key === "Escape" && !$("sheet").hidden) post({ type: "sheet", open: false });
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

}  // namespace panel_shell
