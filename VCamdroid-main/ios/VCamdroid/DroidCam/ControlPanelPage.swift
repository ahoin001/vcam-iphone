import Foundation

enum ControlPanelPage {
    static let html = #"""
    <!DOCTYPE html>
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
        --line: rgba(255, 255, 255, 0.08);
        --line-soft: rgba(255, 255, 255, 0.06);
        --paper: #161618;
        --surface: #202024;
        --surface-2: #1b1b1e;
        --raised: #2a2a2e;
        --raised-2: #36363c;
        --card: linear-gradient(180deg, #202024 0%, #1b1b1e 100%);
        --hair: rgba(255, 255, 255, 0.08);
        --shadow: 0 1px 2px rgba(0, 0, 0, 0.4), 0 24px 60px rgba(0, 0, 0, 0.35);
        --prism-a: #6d7bff;
        --prism-b: #a77bff;
        --mint: #30d158;
        --amber: #ffb547;
        --display: "Unbounded", system-ui, sans-serif;
        --ui: "Onest", system-ui, sans-serif;
        --mono: "JetBrains Mono", ui-monospace, monospace;
        --ease: cubic-bezier(0.2, 0.8, 0.2, 1);
        --radius: 28px;
        --rhythm: 18px;
      }
      * { box-sizing: border-box; }
      [hidden] { display: none !important; }
      html, body { height: 100%; }
      body {
        margin: 0;
        min-height: 100%;
        background:
          radial-gradient(90% 60% at 80% -10%, rgba(109, 123, 255, 0.14), transparent 55%),
          var(--paper);
        color: var(--ink);
        font: 14px/1.45 var(--ui);
        font-variant-numeric: tabular-nums;
        -webkit-font-smoothing: antialiased;
      }
      main {
        min-height: 100%;
        margin: 0 auto;
        padding: 16px 16px 28px;
        display: grid;
        gap: var(--rhythm);
        align-content: start;
      }
      .controls {
        display: grid;
        gap: 16px;
        align-content: start;
      }
      header {
        display: grid;
        gap: 4px;
        padding: 4px 4px 0;
      }
      #status-line {
        margin: 0;
        font-family: var(--display);
        font-size: 22px;
        font-weight: 500;
        letter-spacing: -0.03em;
        color: var(--ink);
      }
      #hint-line {
        margin: 0;
        color: var(--ink-2);
        font-size: 14px;
        line-height: 1.5;
        max-width: 560px;
      }
      section, .shot, .preview {
        background: var(--card);
        border: 1px solid rgba(255, 255, 255, 0.07);
        border-radius: var(--radius);
        padding: 22px 24px;
        display: grid;
        gap: 12px;
        box-shadow: inset 0 1px 0 var(--hair), var(--shadow);
      }
      .preview { gap: 12px; }
      .stage {
        position: relative;
        width: 100%;
        aspect-ratio: 16 / 9;
        border-radius: 18px;
        background: #0c0c0e;
        border: 1px solid var(--line);
        overflow: hidden;
        display: grid;
        place-items: center;
      }
      .stage img {
        width: 100%;
        height: 100%;
        object-fit: contain;
        display: none;
        background: #000;
      }
      .stage.live img { display: block; }
      .stage-empty {
        position: absolute;
        inset: 0;
        display: grid;
        place-items: center;
        padding: 16px;
        text-align: center;
        color: var(--ink-3);
        font-size: 13px;
        line-height: 1.45;
        pointer-events: none;
      }
      .stage.live .stage-empty { display: none; }
      .row, .choices, .tools { display: flex; gap: 8px; flex-wrap: wrap; }
      .tools { gap: 6px; }
      button, label.toggle {
        appearance: none;
        border: 1px solid var(--line);
        background: var(--raised);
        color: var(--ink);
        border-radius: 999px;
        padding: 0 14px;
        min-height: 36px;
        font: inherit;
        font-weight: 550;
        cursor: pointer;
        box-shadow: inset 0 1px 0 var(--hair);
        transition: transform 0.2s var(--ease), background 0.2s var(--ease), box-shadow 0.25s var(--ease);
      }
      button:hover, label.toggle:hover { background: var(--raised-2); }
      button:active, label.toggle:active { transform: scale(0.98); }
      button.on, label.toggle.on {
        background: var(--raised-2);
        border-color: transparent;
        color: var(--ink);
        box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.1), 0 0 0 1.5px rgba(169, 179, 255, 0.75), 0 6px 18px rgba(109, 123, 255, 0.24);
      }
      .choices button:not(.on) {
        background: var(--raised);
        color: var(--ink);
        opacity: 1;
      }
      .tools button { flex: 1; text-align: center; min-width: 64px; }
      .stops button { flex: 1; min-width: 0; padding: 0; }
      label.toggle {
        display: inline-flex;
        align-items: center;
        gap: 7px;
        user-select: none;
      }
      label.toggle input { position: absolute; opacity: 0; pointer-events: none; }
      .slider { display: grid; gap: 6px; }
      .slider span, .heading {
        display: flex;
        justify-content: space-between;
        align-items: center;
        color: var(--ink-3);
        font-family: var(--mono);
        font-size: 11.5px;
        letter-spacing: 0.08em;
        text-transform: uppercase;
      }
      .slider strong, .heading strong {
        color: var(--ink);
        font-family: var(--ui);
        font-size: 13px;
        font-weight: 600;
        letter-spacing: -0.01em;
        text-transform: none;
      }
      input[type="range"] {
        width: 100%;
        accent-color: var(--prism-a);
        height: 1.4rem;
      }
      .preview-bar {
        display: flex;
        justify-content: space-between;
        align-items: center;
        gap: 10px;
      }
      .preview-bar .heading { flex: 1; }
      .note {
        margin: 0;
        padding: 12px 14px;
        border-radius: 16px;
        background: var(--surface-2);
        border: 1px solid var(--line);
        color: var(--ink-2);
        font-size: 13px;
        line-height: 1.5;
      }
      .note strong {
        display: block;
        margin-bottom: 4px;
        font-family: var(--mono);
        font-size: 11px;
        letter-spacing: 0.12em;
        text-transform: uppercase;
        color: var(--ink-3);
        font-weight: 500;
      }
      p.quiet {
        margin: 0;
        color: var(--ink-3);
        font-size: 13px;
        line-height: 1.45;
      }
      [hidden] { display: none !important; }
      html[data-embed="1"] header { display: none; }
      @media (min-width: 900px) {
        body { overflow: hidden; }
        main {
          height: 100%;
          min-height: 0;
          max-width: none;
          margin: 0;
          padding: 18px 28px 24px 18px;
          grid-template-columns: minmax(0, 1fr) minmax(320px, 380px);
          grid-template-rows: auto minmax(0, 1fr);
          overflow: hidden;
        }
        header { grid-column: 1 / -1; }
        html[data-embed="1"] main { grid-template-rows: minmax(0, 1fr); padding-top: 8px; }
        .preview {
          grid-column: 1;
          grid-row: 2;
          min-height: 0;
          display: flex;
          flex-direction: column;
        }
        html[data-embed="1"] .preview { grid-row: 1; }
        .stage {
          flex: 1 1 auto;
          aspect-ratio: auto;
          min-height: 0;
          height: auto;
        }
        .controls {
          grid-column: 2;
          grid-row: 2;
          min-height: 0;
          overflow: auto;
          padding-right: 4px;
        }
        html[data-embed="1"] .controls { grid-row: 1; }
      }
      @media (prefers-reduced-motion: reduce) {
        button:active, label.toggle:active { transform: none; }
      }
    </style>
    </head>
    <body>
    <main>
      <header>
        <h1 id="status-line">Waiting for the computer</h1>
        <p id="hint-line">1080p30 · Connect opens the picture</p>
      </header>
      <section class="preview">
        <div class="preview-bar">
          <div class="heading"><span>Preview</span><strong id="preview-label">Off</strong></div>
          <label class="toggle"><input id="show-preview" type="checkbox">Show preview</label>
        </div>
        <div class="stage" id="stage">
          <img id="preview-image" alt="Desk preview" decoding="async">
          <div class="stage-empty" id="stage-empty">Waiting for a frame from the iPhone.</div>
        </div>
      </section>
      <div class="controls">
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
        <p class="note" id="exposure-auto-note">
          <strong>Suggested</strong>
          Leave Auto on for a smooth picture. Nudge EV if the room is too dark or bright. ISO and shutter do not need a hand unless you are locking a look.
        </p>
        <p class="note" id="exposure-manual-note" hidden>
          <strong>Manual tip</strong>
          At 30 fps keep shutter at <strong>1/60 or faster</strong>. Slower shutter (1/30, 1/15) smears motion and feels choppy — that is not a network issue. Brighten with ISO instead. Noise goes up with ISO; chop goes up with a slow shutter.
        </p>
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
          <p class="quiet" id="shutter-warn" hidden>Shutter is slower than 1/60. Expect motion smear at 30 fps.</p>
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
        <p class="note">
          <strong>Suggested</strong>
          Leave Portrait off for a cleaner background. On the PC, set the camera to VCamdroid in NVIDIA Broadcast, or add Background Removal on a VCamdroid source in OBS. Phone portrait is a light blur if you need it here, and it warms the iPhone.
        </p>
        <p class="quiet" id="portrait-on-hint" hidden>Portrait is on this iPhone. Turn it off if NVIDIA Broadcast or OBS is already cutting the background, or the edges stack and the phone runs hotter.</p>
        <label class="slider" id="portrait-row" hidden>
          <span>Strength <strong id="portrait-value">65%</strong></span>
          <input id="portrait-strength" type="range" min="0" max="1" step="0.01" value="0.65">
        </label>
      </section>
      <section>
        <div class="heading"><span>Stream</span><strong id="bitrate-value">4000 kbps</strong></div>
        <p class="note" id="stream-note">
          <strong>Suggested</strong>
          1080p30 and about 4 Mbps for live calls. Raise bitrate toward 6–8 Mbps if you record in OBS. Desk preview above is a glance only — judge smoothness in OBS or the meeting app.
        </p>
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
          <span>Bitrate <strong id="bitrate-saved">4000 kbps</strong></span>
          <input id="bitrate" type="range" min="2500" max="16000" step="100" value="4000">
        </label>
      </section>
      </div>
    </main>
    <script>
      const active = new Set();
      const pending = new Map();
      let timer = 0;
      let inflight = false;
      let tool = "exposure";
      let session = { width: 1920, height: 1080, fps: 30 };
      let previewOn = false;
      let previewTimer = 0;
      let previewBusy = false;

      const zoom = document.getElementById("zoom");
      const ev = document.getElementById("ev");
      const iso = document.getElementById("iso");
      const shutter = document.getElementById("shutter");
      const focus = document.getElementById("focus");
      const portraitStrength = document.getElementById("portrait-strength");
      const bitrate = document.getElementById("bitrate");
      const stage = document.getElementById("stage");
      const previewImage = document.getElementById("preview-image");
      const showPreview = document.getElementById("show-preview");
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
      bindRange(shutter, "shutter", "shutter-value", (seconds) => {
        updateShutterWarn();
        return shutterLabel(seconds);
      },
        (t) => logFrom(t, shutterMin, shutterMax),
        (value) => logTo(value, shutterMin, shutterMax));
      bindRange(focus, "focus", "focus-value", (value) => Number(value).toFixed(2));
      bindRange(portraitStrength, "portraitStrength", "portrait-value", (value) => Math.round(Number(value) * 100) + "%");
      bindRange(bitrate, "bitrateKbps", "bitrate-saved", (value) => Math.round(Number(value)) + " kbps");

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
          if (button.classList.contains("on") === selected) continue;
          button.classList.toggle("on", selected);
        }
      }

      function markMode() {
        for (const button of document.getElementById("resolution").querySelectorAll("button")) {
          const selected = Number(button.dataset.width) === session.width && Number(button.dataset.height) === session.height;
          if (button.classList.contains("on") === selected) continue;
          button.classList.toggle("on", selected);
        }
        for (const button of document.getElementById("fps").querySelectorAll("button")) {
          const selected = Number(button.dataset.fps) === session.fps;
          if (button.classList.contains("on") === selected) continue;
          button.classList.toggle("on", selected);
        }
      }

      function setFlag(id, checked, key) {
        if (pending.has(key)) return;
        const input = document.getElementById(id);
        const next = !!checked;
        if (input.checked === next && input.parentElement.classList.contains("on") === next) return;
        input.checked = next;
        input.parentElement.classList.toggle("on", next);
      }

      function showManual() {
        const exposure = document.getElementById("manual-exposure").checked;
        const focusManual = document.getElementById("manual-focus").checked;
        document.getElementById("ev-row").hidden = exposure;
        document.getElementById("manual-exposure-fields").hidden = !exposure;
        document.getElementById("exposure-auto-note").hidden = exposure;
        document.getElementById("exposure-manual-note").hidden = !exposure;
        document.getElementById("focus-row").hidden = !focusManual;
        const portraitOn = document.getElementById("portrait").checked;
        document.getElementById("portrait-row").hidden = !portraitOn;
        document.getElementById("portrait-on-hint").hidden = !portraitOn;
        updateShutterWarn();
      }

      function updateShutterWarn() {
        const warn = document.getElementById("shutter-warn");
        if (!warn) return;
        const manual = document.getElementById("manual-exposure").checked;
        const seconds = logFrom(shutter.value, shutterMin, shutterMax);
        warn.hidden = !(manual && seconds > (1 / 60) + 0.0005);
      }

      function showTool() {
        for (const button of document.getElementById("tools").querySelectorAll("button")) {
          const selected = button.dataset.tool === tool;
          if (button.classList.contains("on") === selected) continue;
          button.classList.toggle("on", selected);
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
          const selected = Math.abs(Number(button.dataset.stop) - Number(data.zoom)) < 0.05;
          if (button.classList.contains("on") === selected) continue;
          button.classList.toggle("on", selected);
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
          const activeRate = data.activeBitrateKbps != null ? data.activeBitrateKbps : data.bitrateKbps;
          text("bitrate-value", Math.round(Number(activeRate)) + " kbps");
          setRange(bitrate, "bitrate-saved", data.bitrateKbps, (value) => Math.round(Number(value)) + " kbps", "bitrateKbps");
          for (const button of document.getElementById("bitrate-presets").querySelectorAll("button")) {
            const selected = Number(button.dataset.rate) === Math.round(Number(data.bitrateKbps));
            if (button.classList.contains("on") === selected) continue;
            button.classList.toggle("on", selected);
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

      async function tickPreview() {
        if (!previewOn || previewBusy) return;
        previewBusy = true;
        try {
          const response = await fetch("/v1/preview.jpg?t=" + Date.now(), { cache: "no-store" });
          if (!response.ok) return;
          const blob = await response.blob();
          if (!previewOn) return;
          const url = URL.createObjectURL(blob);
          const previous = previewImage.src;
          previewImage.onload = () => {
            if (previous && previous.indexOf("blob:") === 0) URL.revokeObjectURL(previous);
          };
          previewImage.src = url;
          stage.classList.add("live");
        } catch (_) {
        } finally {
          previewBusy = false;
        }
      }

      function setPreview(on) {
        previewOn = on;
        showPreview.checked = on;
        showPreview.parentElement.classList.toggle("on", on);
        text("preview-label", on ? "On" : "Off");
        clearInterval(previewTimer);
        if (!on) {
          stage.classList.remove("live");
          const previous = previewImage.src;
          previewImage.removeAttribute("src");
          if (previous && previous.indexOf("blob:") === 0) URL.revokeObjectURL(previous);
          return;
        }
        tickPreview();
        previewTimer = setInterval(tickPreview, 90);
      }

      showPreview.addEventListener("change", () => setPreview(showPreview.checked));

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
            const panel = !!data.panel;
            const embed = document.documentElement.dataset.embed === "1";
            const battery = data.battery != null ? data.battery + "%" : "";
            const size = session.width + "×" + session.height + " @" + session.fps;
            const status = live ? "Streaming" : (embed ? "Connecting" : "Waiting for the computer");
            document.getElementById("status-line").textContent = [status, battery, size].filter(Boolean).join(" · ");
            document.getElementById("hint-line").textContent = live
              ? (previewOn
                ? "Desk preview is low-rate · apps that selected VCamdroid show the live picture"
                : "Changes apply live · apps that selected VCamdroid show the picture")
              : (embed
                ? "The companion is connected. The picture is opening."
                : "Connect in VCamPanel to open the picture.");
            document.getElementById("resolution").hidden = panel;
            document.getElementById("fps").hidden = panel;
            document.getElementById("stream-note").innerHTML = panel
              ? "<strong>VCamdroid camera</strong> Stays 1080p30 while VCamPanel is connected. Bitrate below is saved for a DroidCam OBS source after you Disconnect."
              : "<strong>Suggested</strong> 1080p30 and about 4 Mbps for live calls. Raise bitrate toward 6–8 Mbps if you record in OBS. Desk preview above is a glance only — judge smoothness in OBS or the meeting app.";
          }
          if (catalog.ok) {
            const text = await catalog.text();
            document.getElementById("fps-60").hidden = text.indexOf("@60fps") === -1;
          }
        } catch (_) {
        }
      }

      if (new URLSearchParams(location.search).has("embed")) {
        document.documentElement.dataset.embed = "1";
        document.getElementById("status-line").textContent = "Connecting";
        document.getElementById("hint-line").textContent = "The companion is connected. The picture is opening.";
      }
      showTool();
      setPreview(true);
      poll();
      setInterval(poll, 500);
    </script>
    </body>
    </html>
    """#
}
