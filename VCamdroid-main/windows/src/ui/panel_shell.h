#pragma once

#include <string>

inline const char* PanelShellHtml() {
    return R"VCAM(<!DOCTYPE html>
<html lang="en" data-theme="dark">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>VCamdroid</title>
<link rel="preconnect" href="https://fonts.googleapis.com">
<link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
<link href="https://fonts.googleapis.com/css2?family=JetBrains+Mono:wght@400;500&family=Onest:wght@400;500;600&family=Unbounded:wght@500;600&display=swap" rel="stylesheet">
<style>
:root {
  color-scheme: dark;
  --ink: #f5f5f7;
  --ink-2: #c7c7cc;
  --ink-3: #a8a8af;
  --on-ink: #161618;
  --line: rgba(255,255,255,0.08);
  --paper: #161618;
  --surface: #202024;
  --surface-2: #1b1b1e;
  --raised: #2a2a2e;
  --raised-2: #36363c;
  --card: linear-gradient(180deg, #202024 0%, #1b1b1e 100%);
  --hair: rgba(255,255,255,0.08);
  --shadow: 0 1px 2px rgba(0,0,0,0.4), 0 24px 60px rgba(0,0,0,0.35);
  --prism-a: #6d7bff;
  --prism-b: #a77bff;
  --prism-c: #ff8fc8;
  --mint: #30d158;
  --display: "Unbounded", system-ui, sans-serif;
  --ui: "Onest", system-ui, sans-serif;
  --mono: "JetBrains Mono", ui-monospace, monospace;
  --ease: cubic-bezier(0.2, 0.8, 0.2, 1);
  --radius: 28px;
  --titlebar-h: 44px;
  --nav-w: 236px;
  --rhythm: 18px;
}
* { box-sizing: border-box; }
html, body { height: 100%; margin: 0; }
body {
  font-family: var(--ui);
  color: var(--ink);
  background: var(--paper);
  -webkit-font-smoothing: antialiased;
}
button, input { font-family: inherit; }
:focus-visible { outline: 2px solid #c5cbff; outline-offset: 3px; }
.app { position: relative; height: 100%; display: flex; flex-direction: column; overflow: hidden; }
.glow {
  position: absolute; inset: auto -18% -28% 28%; height: 46%;
  background: radial-gradient(closest-side, rgba(109,123,255,0.16), transparent 72%);
  filter: blur(48px); pointer-events: none;
}
.blob {
  position: absolute; border-radius: 50%; filter: blur(70px); pointer-events: none;
  animation: wander 22s ease-in-out infinite;
}
.blob.a { right: -120px; top: -140px; width: 420px; height: 360px; background: rgba(167,123,255,0.16); }
.blob.b { left: -160px; bottom: -180px; width: 460px; height: 380px; background: rgba(109,123,255,0.14); animation-delay: -7s; }
@keyframes wander {
  0%, 100% { transform: translate(0,0) scale(1); }
  33% { transform: translate(40px, 24px) scale(1.06); }
  66% { transform: translate(-28px, -20px) scale(0.96); }
}
.titlebar, .body { position: relative; z-index: 1; }
.titlebar {
  height: var(--titlebar-h); flex: none;
  display: flex; align-items: center; gap: 12px;
  padding: 0 20px;
}
.mark {
  width: 18px; height: 18px; flex: none;
  background: linear-gradient(135deg, #6d7bff 0%, #a77bff 55%, #ff8fc8 100%);
  clip-path: polygon(50% 0, 62% 38%, 100% 50%, 62% 62%, 50% 100%, 38% 62%, 0 50%, 38% 38%);
}
.brand {
  font-family: var(--display); font-size: 16px; font-weight: 500; letter-spacing: -0.02em;
}
.status {
  flex: 1; min-width: 0;
  font-family: var(--mono); font-size: 12px; color: var(--ink-3);
  white-space: nowrap; overflow: hidden; text-overflow: ellipsis;
  display: flex; align-items: center; gap: 8px;
}
.status .dot {
  width: 6px; height: 6px; border-radius: 50%; background: var(--ink-3); flex: none;
}
.app.live .status .dot { background: var(--mint); box-shadow: 0 0 0 4px rgba(48,209,88,0.18); }
.body { flex: 1; display: flex; min-height: 0; }
.nav {
  width: var(--nav-w); flex: none;
  padding: 0 14px 18px;
  display: flex; flex-direction: column; gap: 2px;
  overflow: auto;
}
.nav-label {
  font-family: var(--mono); font-size: 10.5px; letter-spacing: 0.14em;
  color: var(--ink-3); padding: 14px 12px 6px; text-transform: uppercase;
}
.phones { display: flex; flex-direction: column; gap: 2px; min-height: 72px; }
.nav-item {
  display: flex; flex-direction: column; align-items: flex-start; justify-content: center; gap: 2px;
  min-height: 40px; padding: 8px 12px; border-radius: 12px; border: 0;
  background: transparent; width: 100%;
  text-align: left; font-size: 13px; font-weight: 500; color: var(--ink-2);
  cursor: pointer;
  transition: background 0.2s var(--ease), color 0.2s var(--ease);
}
.nav-item:hover { background: rgba(255,255,255,0.06); color: var(--ink); }
.nav-item.on {
  background: var(--surface); color: var(--ink); font-weight: 600;
  box-shadow: inset 0 1px 0 var(--hair), 0 1px 2px rgba(0,0,0,0.4);
}
.nav-item small {
  display: block; font-family: var(--mono); font-size: 10.5px;
  color: var(--ink-3); font-weight: 400; letter-spacing: 0;
}
.empty {
  margin: 8px 12px; font-size: 13px; color: var(--ink-3); line-height: 1.45;
}
.field { display: flex; flex-direction: column; gap: 6px; padding: 12px 4px 8px; }
.field span { font-size: 12px; font-weight: 550; color: var(--ink-2); }
.input {
  width: 100%; border: 1px solid var(--line); background: var(--surface-2);
  border-radius: 14px; padding: 11px 14px; font-size: 13px; color: var(--ink);
  outline: none; box-shadow: inset 0 1px 2px rgba(0,0,0,0.25);
  transition: border-color 0.2s var(--ease), box-shadow 0.2s var(--ease);
}
.input:focus { border-color: #8d97ff; box-shadow: 0 0 0 4px rgba(109,123,255,0.28); }
.actions { display: flex; flex-direction: column; gap: 8px; padding: 8px 4px 0; }
.btn {
  height: 40px; padding: 0 16px; border-radius: 999px; border: 1px solid var(--line);
  background: var(--raised); color: var(--ink);
  font-size: 14px; font-weight: 550; cursor: pointer;
  display: inline-flex; align-items: center; justify-content: center;
  transition: transform 0.2s var(--ease), box-shadow 0.25s var(--ease), background 0.2s var(--ease), filter 0.2s var(--ease);
  box-shadow: inset 0 1px 0 var(--hair), 0 1px 2px rgba(0,0,0,0.4);
}
.btn:hover { background: var(--raised-2); }
.btn:active { transform: scale(0.98); }
.btn:disabled { background: var(--raised); color: var(--ink-3); cursor: default; }
.btn.primary {
  background: linear-gradient(135deg, #ffffff, #dcdce2); color: var(--on-ink); border-color: transparent;
  box-shadow: inset 0 1px 0 rgba(255,255,255,0.6), 0 1px 2px rgba(0,0,0,0.4);
}
.btn.primary:hover { filter: brightness(1.06); }
.btn.primary:disabled { filter: none; background: var(--raised); color: var(--ink-3); border-color: var(--line); }
.btn.ghost { background: transparent; border-color: transparent; box-shadow: none; color: var(--ink-2); }
.btn.ghost:hover { background: rgba(255,255,255,0.06); color: var(--ink); }
.obs {
  margin-top: auto; padding: 16px;
  border-radius: 18px; background: rgba(32,32,36,0.86);
  border: 1px solid var(--line);
  box-shadow: inset 0 1px 0 var(--hair);
}
.obs b { display: block; font-size: 13px; font-weight: 600; }
.obs span { font-family: var(--mono); font-size: 11.5px; color: var(--ink-3); }
.main {
  flex: 1; min-width: 0;
  padding: 0 28px 24px 8px;
  display: flex; flex-direction: column; gap: var(--rhythm);
}
.stage {
  flex: 1; min-height: 0; position: relative;
  border-radius: var(--radius);
  background: var(--card);
  border: 1px solid rgba(255,255,255,0.07);
  box-shadow: inset 0 1px 0 var(--hair), var(--shadow);
  overflow: hidden;
}
.stage iframe {
  position: absolute; inset: 0; width: 100%; height: 100%; border: 0; background: #0c0c0e;
}
.idle {
  position: absolute; inset: 0; padding: 36px;
  display: flex; flex-direction: column; justify-content: flex-end; gap: 10px;
}
.idle h1 {
  margin: 0; font-family: var(--display); font-size: 28px; font-weight: 500; letter-spacing: -0.04em;
}
.idle p { margin: 0; max-width: 420px; font-size: 14px; line-height: 1.5; color: var(--ink-2); }
.idle ol { margin: 8px 0 0; padding: 0; list-style: none; display: grid; gap: 8px; }
.idle li { display: flex; gap: 10px; align-items: flex-start; font-size: 13px; color: var(--ink); }
.n {
  flex: none; width: 22px; height: 22px; border-radius: 999px;
  display: grid; place-items: center;
  background: rgba(109,123,255,0.16); color: #c5cbff;
  font-family: var(--mono); font-size: 11px;
}
.app.live .idle { display: none; }
.app:not(.live) iframe { visibility: hidden; }
.rise { animation: rise 0.7s var(--ease) both; }
@keyframes rise { from { opacity: 0; transform: translateY(12px); filter: blur(6px); } to { opacity: 1; transform: none; filter: blur(0); } }
@media (max-width: 900px) {
  .body { flex-direction: column; }
  .nav { width: auto; max-height: 42%; border-bottom: 1px solid var(--line); }
  .main { padding: 12px 16px 16px; }
}
@media (prefers-reduced-motion: reduce) {
  .rise, .blob { animation: none; }
  .btn:active { transform: none; }
}
</style>
</head>
<body>
<div class="app" id="app">
  <div class="glow"></div>
  <i class="blob a"></i>
  <i class="blob b"></i>
  <header class="titlebar">
    <span class="mark" aria-hidden="true"></span>
    <div class="brand">VCamdroid</div>
    <div class="status"><span class="dot"></span><span id="status">Plug in the iPhone or pick a Wi-Fi phone, then Connect.</span></div>
    <button class="btn ghost" type="button" id="copy">Copy</button>
  </header>
  <div class="body">
    <nav class="nav">
      <div class="nav-label">Phones</div>
      <div class="phones" id="phones"></div>
      <p class="empty" id="empty">Plug in a data cable, tap Trust This Computer, or type the Wi-Fi IP.</p>
      <label class="field">
        <span>Wi-Fi address</span>
        <input class="input" id="address" spellcheck="false" placeholder="192.168.1.20:4747">
      </label>
      <div class="actions">
        <button class="btn primary" type="button" id="connect">Connect</button>
        <button class="btn" type="button" id="disconnect" disabled>Disconnect</button>
        <button class="btn ghost" type="button" id="refresh">Refresh</button>
      </div>
      <div class="obs">
        <b>Optional OBS</b>
        <span id="obs">127.0.0.1:4747</span>
      </div>
    </nav>
    <main class="main">
      <section class="stage rise" id="stage">
        <iframe id="desk" title="Camera desk"></iframe>
        <div class="idle" id="idle">
          <h1>Connect a phone</h1>
          <p>This window receives the iPhone and publishes the VCamdroid camera. Shot controls sit beside the preview.</p>
          <ol>
            <li><span class="n">1</span><span>Refresh to find USB or Wi-Fi phones.</span></li>
            <li><span class="n">2</span><span>Select a phone, or type the Wi-Fi IP.</span></li>
            <li><span class="n">3</span><span>Connect. The picture opens here.</span></li>
          </ol>
        </div>
      </section>
    </main>
  </div>
</div>
<script>
const phones = document.getElementById("phones");
const empty = document.getElementById("empty");
const address = document.getElementById("address");
const status = document.getElementById("status");
const obs = document.getElementById("obs");
const connect = document.getElementById("connect");
const disconnect = document.getElementById("disconnect");
const desk = document.getElementById("desk");
const app = document.getElementById("app");
let selected = -1;
let lastDesk = "";

function send(cmd, extra) {
  if (!window.chrome || !window.chrome.webview) return;
  window.chrome.webview.postMessage(Object.assign({ cmd: cmd }, extra || {}));
}

function apply(state) {
  selected = typeof state.selected === "number" ? state.selected : -1;
  const list = state.devices || [];
  phones.replaceChildren();
  list.forEach(function(device, index) {
    const button = document.createElement("button");
    button.type = "button";
    button.className = "nav-item" + (index === selected ? " on" : "");
    const title = document.createElement("span");
    title.textContent = device.label || "";
    button.appendChild(title);
    if (device.kind) {
      const kind = document.createElement("small");
      kind.textContent = device.kind === "usb" ? "USB" : "Wi-Fi";
      button.appendChild(kind);
    }
    button.addEventListener("click", function() {
      selected = index;
      send("select", { id: index });
    });
    button.addEventListener("dblclick", function() {
      selected = index;
      send("connect", { id: index, address: address.value });
    });
    phones.appendChild(button);
  });
  empty.hidden = list.length > 0;
  if (typeof state.address === "string" && document.activeElement !== address) {
    address.value = state.address;
  }
  if (state.status) status.textContent = state.status;
  if (state.obs) obs.textContent = state.obs;
  const live = !!state.connected;
  app.classList.toggle("live", live);
  connect.disabled = live;
  disconnect.disabled = !live;
  const url = state.deskUrl || "";
  if (url !== lastDesk) {
    lastDesk = url;
    desk.src = url || "about:blank";
  }
}

address.addEventListener("change", function() { send("address", { value: address.value }); });
address.addEventListener("keydown", function(event) {
  if (event.key === "Enter") {
    event.preventDefault();
    send("connect", { id: selected, address: address.value });
  }
});
document.getElementById("connect").addEventListener("click", function() {
  send("connect", { id: selected, address: address.value });
});
document.getElementById("disconnect").addEventListener("click", function() { send("disconnect"); });
document.getElementById("refresh").addEventListener("click", function() { send("refresh"); });
document.getElementById("copy").addEventListener("click", function() { send("copy"); });

if (window.chrome && window.chrome.webview) {
  window.chrome.webview.addEventListener("message", function(event) { apply(event.data); });
  send("ready");
}
</script>
</body>
</html>)VCAM";
}
