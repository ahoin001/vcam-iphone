import Foundation

enum ControlPanelPage {
    static let html = #"""
    <!DOCTYPE html>
    <html lang="en">
    <head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>VCamdroid</title>
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
        font-variant-numeric: tabular-nums;
      }
      main {
        max-width: 440px;
        margin: 0 auto;
        padding: 14px 16px 24px;
        display: grid;
        gap: 10px;
      }
      h1 { margin: 0; font-size: 15px; font-weight: 600; }
      p { margin: 0; color: var(--muted); font-size: 12px; }
      section, .shot {
        background: var(--card);
        border: 1px solid var(--line);
        border-radius: 14px;
        padding: 12px;
        display: grid;
        gap: 8px;
      }
      .row, .choices, .tools { display: flex; gap: 8px; flex-wrap: wrap; }
      .tools { gap: 6px; }
      button, label.toggle {
        appearance: none;
        border: 1px solid var(--line);
        background: transparent;
        color: var(--text);
        border-radius: 999px;
        padding: 7px 12px;
        font: inherit;
        cursor: pointer;
      }
      button:active, label.toggle:active { transform: scale(0.97); }
      button.on, label.toggle.on {
        background: var(--accent);
        border-color: var(--accent);
        color: var(--accent-ink);
      }
      .tools button { flex: 1; text-align: center; min-width: 64px; }
      .stops button { flex: 1; min-width: 0; padding: 7px 0; }
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
        <p id="status-line">Waiting for OBS · watch the picture there</p>
        <p id="hint-line">1080p30 · 6–8 Mbps for recording · curtain on the phone keeps it cooler</p>
      </header>
      <section class="shot">
        <div class="heading"><span>Lens</span></div>
        <div class="choices" id="lens">
          <button type="button" data-value="back">Back</button>
          <button type="button" data-value="front">Front</button>
        </div>
        <div class="heading"><span>Zoom <strong id="zoom-value">1.0×</strong></span></div>
        <div class="row stops" id="zoom-stops">
          <button type="button" data-stop="1">1×</button>
          <button type="button" data-stop="2">2×</button>
          <button type="button" data-stop="3">3×</button>
          <button type="button" data-stop="5">5×</button>
          <button type="button" data-stop="10">10×</button>
        </div>
        <label class="slider">
          <span></span>
          <input id="zoom" type="range" min="1" max="10" step="0.1" value="1">
        </label>
        <div class="row">
          <label class="toggle"><input id="torch" type="checkbox">Torch</label>
          <label class="toggle"><input id="flip" type="checkbox">Flip 180°</label>
        </div>
      </section>
      <div class="tools" id="tools">
        <button type="button" class="on" data-tool="exposure">Exposure</button>
        <button type="button" data-tool="whiteBalance">WB</button>
        <button type="button" data-tool="focus">Focus</button>
        <button type="button" data-tool="portrait">Portrait</button>
      </div>
      <section id="tool-exposure">
        <div class="heading"><span>Exposure</span><label class="toggle"><input id="manual-exposure" type="checkbox">Manual</label></div>
        <label class="slider" id="ev-row">
          <span>EV <strong id="ev-value">0.0</strong></span>
          <input id="ev" type="range" min="-3" max="3" step="0.1" value="0">
        </label>
        <div id="manual-exposure-fields" hidden>
          <label class="slider">
            <span>ISO <strong id="iso-value">100</strong></span>
            <input id="iso" type="range" min="0" max="1" step="0.001" value="0.2">
          </label>
          <label class="slider">
            <span>Shutter <strong id="shutter-value">1/60</strong></span>
            <input id="shutter" type="range" min="0" max="1" step="0.001" value="0.5">
          </label>
        </div>
      </section>
      <section id="tool-whiteBalance" hidden>
        <div class="heading"><span>White balance</span></div>
        <div class="choices" id="wb">
          <button type="button" data-value="auto">Auto</button>
          <button type="button" data-value="daylight">Daylight</button>
          <button type="button" data-value="cloudy">Cloudy</button>
          <button type="button" data-value="fluorescent">Fluorescent</button>
          <button type="button" data-value="incandescent">Incandescent</button>
        </div>
      </section>
      <section id="tool-focus" hidden>
        <div class="heading"><span>Focus</span><label class="toggle"><input id="manual-focus" type="checkbox">Manual</label></div>
        <label class="slider" id="focus-row" hidden>
          <span>Position <strong id="focus-value">1.00</strong></span>
          <input id="focus" type="range" min="0" max="1" step="0.01" value="1">
        </label>
      </section>
      <section id="tool-portrait" hidden>
        <div class="heading"><span>Portrait</span><label class="toggle"><input id="portrait" type="checkbox">On</label></div>
        <p>Leave the iPhone preview off and watch the blurred background in OBS.</p>
        <label class="slider" id="portrait-row" hidden>
          <span>Strength <strong id="portrait-value">65%</strong></span>
          <input id="portrait-strength" type="range" min="0" max="1" step="0.01" value="0.65">
        </label>
      </section>
      <section>
        <div class="heading"><span>Stream</span><strong id="bitrate-value">4000 kbps</strong></div>
        <div class="choices" id="resolution">
          <button type="button" data-width="1280" data-height="720">1280×720</button>
          <button type="button" data-width="1920" data-height="1080">1920×1080</button>
        </div>
        <div class="choices" id="fps">
          <button type="button" data-fps="30">30 fps</button>
          <button type="button" id="fps-60" data-fps="60" hidden>60 fps</button>
        </div>
        <div class="choices" id="bitrate-presets">
          <button type="button" data-rate="4000">4 Mbps</button>
          <button type="button" data-rate="6000">6 Mbps</button>
          <button type="button" data-rate="8000">8 Mbps</button>
        </div>
        <label class="slider">
          <span>Bitrate</span>
          <input id="bitrate" type="range" min="2500" max="16000" step="100" value="4000">
        </label>
      </section>
    </main>
    <script>
      const active = new Set();
      const pending = new Map();
      let timer = 0;
      let inflight = false;
      let tool = "exposure";
      let session = { width: 1920, height: 1080, fps: 30 };

      const zoom = document.getElementById("zoom");
      const ev = document.getElementById("ev");
      const iso = document.getElementById("iso");
      const shutter = document.getElementById("shutter");
      const focus = document.getElementById("focus");
      const portraitStrength = document.getElementById("portrait-strength");
      const bitrate = document.getElementById("bitrate");
      const isoMin = 50, isoMax = 6400;
      const shutterMin = 0.0001, shutterMax = 0.5;

      function text(id, value) {
        document.getElementById(id).textContent = value;
      }

      function logFrom(t, min, max) {
        return min * Math.pow(max / min, Number(t));
      }

      function logTo(value, min, max) {
        const clamped = Math.min(max, Math.max(min, Number(value)));
        return Math.log(clamped / min) / Math.log(max / min);
      }

      function shutterLabel(seconds) {
        const value = Number(seconds);
        if (value >= 1) return value.toFixed(1) + "s";
        return "1/" + Math.max(1, Math.round(1 / value));
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

      function bindRange(input, key, label, format, read, write) {
        input.addEventListener("pointerdown", () => hold(key, true));
        input.addEventListener("pointerup", () => hold(key, false));
        input.addEventListener("pointercancel", () => hold(key, false));
        input.addEventListener("input", () => {
          hold(key, true);
          const value = read ? read(input.value) : input.value;
          text(label, format(value));
          queue(key, value);
        });
        input.addEventListener("change", () => {
          hold(key, false);
          const value = read ? read(input.value) : input.value;
          queue(key, value);
          flush();
        });
        input._write = write;
        input._format = format;
        input._label = label;
      }

      bindRange(zoom, "zoom", "zoom-value", (value) => Number(value).toFixed(1) + "×");
      bindRange(ev, "exposureBias", "ev-value", (value) => Number(value).toFixed(1));
      bindRange(iso, "iso", "iso-value", (value) => String(Math.round(Number(value))),
        (t) => logFrom(t, isoMin, isoMax),
        (value) => logTo(value, isoMin, isoMax));
      bindRange(shutter, "shutter", "shutter-value", shutterLabel,
        (t) => logFrom(t, shutterMin, shutterMax),
        (value) => logTo(value, shutterMin, shutterMax));
      bindRange(focus, "focus", "focus-value", (value) => Number(value).toFixed(2));
      bindRange(portraitStrength, "portraitStrength", "portrait-value", (value) => Math.round(Number(value) * 100) + "%");
      bindRange(bitrate, "bitrateKbps", "bitrate-value", (value) => Math.round(Number(value)) + " kbps");

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

      document.getElementById("zoom-stops").addEventListener("click", (event) => {
        const button = event.target.closest("button");
        if (!button) return;
        queue("zoom", button.dataset.stop);
        flush();
      });

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
      bindFlag("portrait", "portrait");

      document.getElementById("tools").addEventListener("click", (event) => {
        const button = event.target.closest("button");
        if (!button) return;
        tool = button.dataset.tool;
        showTool();
      });

      document.getElementById("resolution").addEventListener("click", (event) => {
        const button = event.target.closest("button");
        if (!button) return;
        putSession({ width: Number(button.dataset.width), height: Number(button.dataset.height), fps: session.fps });
      });
      document.getElementById("fps").addEventListener("click", (event) => {
        const button = event.target.closest("button");
        if (!button) return;
        putSession({ width: session.width, height: session.height, fps: Number(button.dataset.fps) });
      });
      document.getElementById("bitrate-presets").addEventListener("click", (event) => {
        const button = event.target.closest("button");
        if (!button) return;
        queue("bitrateKbps", button.dataset.rate);
        flush();
      });

      function setRange(input, label, value, format, key) {
        if (active.has(key) || pending.has(key)) return;
        if (input._write) input.value = input._write(value);
        else input.value = value;
        text(label, (input._format || format)(value));
      }

      function mark(group, value) {
        for (const button of group.querySelectorAll("button")) {
          const selected = button.dataset.value === value
            || button.dataset.stop === String(value)
            || (button.dataset.width && Number(button.dataset.width) === Number(value));
          button.classList.toggle("on", selected);
        }
      }

      function markMode() {
        for (const button of document.getElementById("resolution").querySelectorAll("button")) {
          button.classList.toggle("on", Number(button.dataset.width) === session.width && Number(button.dataset.height) === session.height);
        }
        for (const button of document.getElementById("fps").querySelectorAll("button")) {
          button.classList.toggle("on", Number(button.dataset.fps) === session.fps);
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
        document.getElementById("portrait-row").hidden = !document.getElementById("portrait").checked;
      }

      function showTool() {
        for (const button of document.getElementById("tools").querySelectorAll("button")) {
          button.classList.toggle("on", button.dataset.tool === tool);
        }
        for (const name of ["exposure", "whiteBalance", "focus", "portrait"]) {
          document.getElementById("tool-" + name).hidden = name !== tool;
        }
      }

      function applyState(data) {
        mark(document.getElementById("lens"), data.lens);
        mark(document.getElementById("wb"), data.whiteBalance);
        setRange(zoom, "zoom-value", data.zoom, (value) => Number(value).toFixed(1) + "×", "zoom");
        for (const button of document.getElementById("zoom-stops").querySelectorAll("button")) {
          button.classList.toggle("on", Math.abs(Number(button.dataset.stop) - Number(data.zoom)) < 0.05);
        }
        setRange(ev, "ev-value", data.exposureBias, (value) => Number(value).toFixed(1), "exposureBias");
        setRange(iso, "iso-value", data.iso, (value) => String(Math.round(Number(value))), "iso");
        setRange(shutter, "shutter-value", data.shutter, shutterLabel, "shutter");
        setRange(focus, "focus-value", data.focus, (value) => Number(value).toFixed(2), "focus");
        setFlag("torch", data.torch, "torch");
        setFlag("flip", data.flip, "flip");
        setFlag("manual-exposure", data.manualExposure, "manualExposure");
        setFlag("manual-focus", data.manualFocus, "manualFocus");
        setFlag("portrait", data.portrait, "portrait");
        setRange(portraitStrength, "portrait-value", data.portraitStrength, (value) => Math.round(Number(value) * 100) + "%", "portraitStrength");
        if (data.bitrateKbps != null) {
          setRange(bitrate, "bitrate-value", data.bitrateKbps, (value) => Math.round(Number(value)) + " kbps", "bitrateKbps");
          for (const button of document.getElementById("bitrate-presets").querySelectorAll("button")) {
            button.classList.toggle("on", Number(button.dataset.rate) === Math.round(Number(data.bitrateKbps)));
          }
        }
        showManual();
      }

      async function putSession(next) {
        session = next;
        markMode();
        const params = new URLSearchParams({
          width: String(next.width),
          height: String(next.height),
          fps: String(next.fps)
        });
        try {
          await fetch("/v1/session?" + params.toString(), { method: "PUT", cache: "no-store" });
        } catch (_) {
        }
      }

      async function poll() {
        if (inflight || pending.size) return;
        try {
          const [controls, snapshot, catalog] = await Promise.all([
            fetch("/v1/controls", { cache: "no-store" }),
            fetch("/v1/session", { cache: "no-store" }),
            fetch("/v1/resolutions", { cache: "no-store" })
          ]);
          if (controls.ok) applyState(await controls.json());
          if (snapshot.ok) {
            const data = await snapshot.json();
            session.width = data.width || session.width;
            session.height = data.height || session.height;
            session.fps = data.fps || session.fps;
            markMode();
            const live = data.streaming || data.status === "streaming";
            const battery = data.battery != null ? data.battery + "%" : "";
            const size = session.width + "×" + session.height + " @" + session.fps;
            document.getElementById("status-line").textContent = [
              live ? "Streaming" : "Waiting for OBS",
              battery,
              size
            ].filter(Boolean).join(" · ");
            document.getElementById("hint-line").textContent = live
              ? "Changes apply live · watch the picture in OBS"
              : "Connect OBS first, then use these controls from the desk";
          }
          if (catalog.ok) {
            const text = await catalog.text();
            document.getElementById("fps-60").hidden = text.indexOf("@60fps") === -1;
          }
        } catch (_) {
        }
      }

      showTool();
      poll();
      setInterval(poll, 500);
    </script>
    </body>
    </html>
    """#
}
