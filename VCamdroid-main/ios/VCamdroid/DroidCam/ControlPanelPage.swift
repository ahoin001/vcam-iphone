import Foundation

enum ControlPanelPage {
    static let html = #"""
    <!DOCTYPE html>
    <html lang="en">
    <head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>VCamdroid Controls</title>
    <style>
      :root {
        color-scheme: dark;
        --bg: #141618;
        --card: #1c1f22;
        --line: #2c3136;
        --text: #f3f1ec;
        --muted: #a7a39a;
        --accent: #e2a23a;
        --accent-ink: #1a1408;
      }
      * { box-sizing: border-box; }
      body {
        margin: 0;
        background: var(--bg);
        color: var(--text);
        font: 14px/1.4 "Segoe UI", system-ui, sans-serif;
      }
      main {
        max-width: 420px;
        margin: 0 auto;
        padding: 16px;
        display: grid;
        gap: 12px;
      }
      h1 {
        margin: 0;
        font-size: 15px;
        font-weight: 600;
        letter-spacing: 0.01em;
      }
      p { margin: 0; color: var(--muted); font-size: 12px; }
      section {
        background: var(--card);
        border: 1px solid var(--line);
        border-radius: 12px;
        padding: 12px;
        display: grid;
        gap: 8px;
      }
      .row, .choices { display: flex; gap: 8px; flex-wrap: wrap; }
      button, label.toggle {
        appearance: none;
        border: 1px solid var(--line);
        background: transparent;
        color: var(--text);
        border-radius: 999px;
        padding: 6px 12px;
        font: inherit;
        cursor: pointer;
      }
      button.on, label.toggle.on {
        background: var(--accent);
        border-color: var(--accent);
        color: var(--accent-ink);
      }
      label.toggle { display: inline-flex; align-items: center; gap: 6px; }
      label.toggle input { position: absolute; opacity: 0; pointer-events: none; }
      .slider { display: grid; gap: 4px; }
      .slider span, .heading { display: flex; justify-content: space-between; color: var(--muted); font-size: 12px; }
      .slider strong, .heading strong { color: var(--text); font-weight: 600; }
      input[type="range"] { width: 100%; accent-color: var(--accent); }
      [hidden] { display: none !important; }
    </style>
    </head>
    <body>
    <main>
      <header>
        <h1>Camera</h1>
        <p>Changes apply on the phone without reconnecting OBS.</p>
      </header>
      <section>
        <div class="heading"><span>Lens</span></div>
        <div class="choices" id="lens">
          <button type="button" data-value="back">Back</button>
          <button type="button" data-value="front">Front</button>
        </div>
        <label class="slider">
          <span>Zoom <strong id="zoom-value">1.0</strong></span>
          <input id="zoom" type="range" min="1" max="10" step="0.1" value="1">
        </label>
        <div class="row">
          <label class="toggle"><input id="torch" type="checkbox">Torch</label>
          <label class="toggle"><input id="flip" type="checkbox">Flip 180°</label>
        </div>
      </section>
      <section>
        <div class="heading"><span>Exposure</span><label class="toggle"><input id="manual-exposure" type="checkbox">Manual</label></div>
        <label class="slider" id="ev-row">
          <span>EV <strong id="ev-value">0.0</strong></span>
          <input id="ev" type="range" min="-3" max="3" step="0.1" value="0">
        </label>
        <div id="manual-exposure-fields" hidden>
          <label class="slider">
            <span>ISO <strong id="iso-value">100</strong></span>
            <input id="iso" type="range" min="50" max="6400" step="1" value="100">
          </label>
          <label class="slider">
            <span>Shutter <strong id="shutter-value">0.0167</strong></span>
            <input id="shutter" type="range" min="0.0001" max="0.5" step="0.0001" value="0.0167">
          </label>
        </div>
      </section>
      <section>
        <div class="heading"><span>White balance</span></div>
        <div class="choices" id="wb">
          <button type="button" data-value="auto">Auto</button>
          <button type="button" data-value="daylight">Daylight</button>
          <button type="button" data-value="cloudy">Cloudy</button>
          <button type="button" data-value="fluorescent">Fluorescent</button>
          <button type="button" data-value="incandescent">Incandescent</button>
        </div>
      </section>
      <section>
        <div class="heading"><span>Focus</span><label class="toggle"><input id="manual-focus" type="checkbox">Manual</label></div>
        <label class="slider" id="focus-row" hidden>
          <span>Position <strong id="focus-value">1.00</strong></span>
          <input id="focus" type="range" min="0" max="1" step="0.01" value="1">
        </label>
      </section>
    </main>
    <script>
      const active = new Set();
      const pending = new Map();
      let timer = 0;
      let inflight = false;

      const zoom = document.getElementById("zoom");
      const ev = document.getElementById("ev");
      const iso = document.getElementById("iso");
      const shutter = document.getElementById("shutter");
      const focus = document.getElementById("focus");

      function text(id, value) {
        document.getElementById(id).textContent = value;
      }

      function queue(key, value) {
        pending.set(key, String(value));
        clearTimeout(timer);
        timer = setTimeout(flush, 80);
      }

      async function flush() {
        if (inflight || pending.size === 0) return;
        inflight = true;
        const params = new URLSearchParams();
        for (const [key, value] of pending) params.set(key, value);
        pending.clear();
        try {
          const response = await fetch("/v1/controls?" + params.toString(), { method: "PUT", cache: "no-store" });
          if (response.ok) applyState(await response.json());
        } catch (_) {
        } finally {
          inflight = false;
          if (pending.size) flush();
        }
      }

      function hold(key, on) {
        if (on) active.add(key);
        else active.delete(key);
      }

      function bindRange(input, key, label, format) {
        input.addEventListener("pointerdown", () => hold(key, true));
        input.addEventListener("pointerup", () => hold(key, false));
        input.addEventListener("pointercancel", () => hold(key, false));
        input.addEventListener("input", () => {
          hold(key, true);
          text(label, format(input.value));
          queue(key, input.value);
        });
        input.addEventListener("change", () => {
          hold(key, false);
          queue(key, input.value);
          flush();
        });
      }

      bindRange(zoom, "zoom", "zoom-value", (value) => Number(value).toFixed(1));
      bindRange(ev, "exposureBias", "ev-value", (value) => Number(value).toFixed(1));
      bindRange(iso, "iso", "iso-value", (value) => String(Math.round(Number(value))));
      bindRange(shutter, "shutter", "shutter-value", (value) => Number(value).toFixed(4));
      bindRange(focus, "focus", "focus-value", (value) => Number(value).toFixed(2));

      function bindChoice(group, key) {
        group.addEventListener("click", (event) => {
          const button = event.target.closest("button");
          if (!button) return;
          queue(key, button.dataset.value);
          flush();
        });
      }
      bindChoice(document.getElementById("lens"), "lens");
      bindChoice(document.getElementById("wb"), "whiteBalance");

      function bindFlag(id, key) {
        const input = document.getElementById(id);
        input.addEventListener("change", () => {
          input.parentElement.classList.toggle("on", input.checked);
          queue(key, input.checked ? "1" : "0");
          showManual();
          flush();
        });
      }
      bindFlag("torch", "torch");
      bindFlag("flip", "flip");
      bindFlag("manual-exposure", "manualExposure");
      bindFlag("manual-focus", "manualFocus");

      function setRange(input, label, value, format, key) {
        if (active.has(key) || pending.has(key)) return;
        input.value = value;
        text(label, format(value));
      }

      function mark(group, value) {
        for (const button of group.querySelectorAll("button")) {
          button.classList.toggle("on", button.dataset.value === value);
        }
      }

      function setFlag(id, checked, key) {
        if (pending.has(key)) return;
        const input = document.getElementById(id);
        input.checked = !!checked;
        input.parentElement.classList.toggle("on", input.checked);
      }

      function showManual() {
        const exposure = document.getElementById("manual-exposure").checked;
        const focusManual = document.getElementById("manual-focus").checked;
        document.getElementById("ev-row").hidden = exposure;
        document.getElementById("manual-exposure-fields").hidden = !exposure;
        document.getElementById("focus-row").hidden = !focusManual;
      }

      function applyState(data) {
        mark(document.getElementById("lens"), data.lens);
        mark(document.getElementById("wb"), data.whiteBalance);
        setRange(zoom, "zoom-value", data.zoom, (value) => Number(value).toFixed(1), "zoom");
        setRange(ev, "ev-value", data.exposureBias, (value) => Number(value).toFixed(1), "exposureBias");
        setRange(iso, "iso-value", data.iso, (value) => String(Math.round(Number(value))), "iso");
        setRange(shutter, "shutter-value", data.shutter, (value) => Number(value).toFixed(4), "shutter");
        setRange(focus, "focus-value", data.focus, (value) => Number(value).toFixed(2), "focus");
        setFlag("torch", data.torch, "torch");
        setFlag("flip", data.flip, "flip");
        setFlag("manual-exposure", data.manualExposure, "manualExposure");
        setFlag("manual-focus", data.manualFocus, "manualFocus");
        showManual();
      }

      async function poll() {
        if (inflight || pending.size) return;
        try {
          const response = await fetch("/v1/controls", { cache: "no-store" });
          if (response.ok) applyState(await response.json());
        } catch (_) {
        }
      }

      poll();
      setInterval(poll, 500);
    </script>
    </body>
    </html>
    """#
}
