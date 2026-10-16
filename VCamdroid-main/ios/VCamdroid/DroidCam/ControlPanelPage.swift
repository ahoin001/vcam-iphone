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
        --bg: #121416;
        --bg-soft: #171a1d;
        --card: #1a1e22;
        --card-lift: #20252a;
        --line: #2a3036;
        --line-soft: #24292e;
        --text: #f2efe8;
        --muted: #9a958c;
        --accent: #d9a04a;
        --accent-soft: rgba(217, 160, 74, 0.18);
        --accent-ink: #1a1408;
        --stage: #0c0e10;
        --radius: 16px;
        --radius-sm: 12px;
      }
      * { box-sizing: border-box; }
      body {
        margin: 0;
        min-height: 100%;
        background:
          radial-gradient(120% 80% at 50% -10%, rgba(217, 160, 74, 0.07), transparent 55%),
          linear-gradient(180deg, var(--bg-soft), var(--bg));
        color: var(--text);
        font: 14px/1.45 "Segoe UI", system-ui, sans-serif;
        font-variant-numeric: tabular-nums;
        -webkit-font-smoothing: antialiased;
      }
      main {
        max-width: 460px;
        margin: 0 auto;
        padding: 16px 16px 28px;
        display: grid;
        gap: 12px;
      }
      header {
        display: grid;
        gap: 4px;
        padding: 2px 2px 4px;
      }
      h1 {
        margin: 0;
        font-size: 13px;
        font-weight: 600;
        letter-spacing: 0.08em;
        text-transform: uppercase;
        color: var(--muted);
      }
      #status-line {
        margin: 0;
        font-size: 15px;
        font-weight: 600;
        color: var(--text);
      }
      #hint-line {
        margin: 0;
        color: var(--muted);
        font-size: 12px;
      }
      section, .shot, .preview {
        background: color-mix(in srgb, var(--card) 92%, transparent);
        border: 1px solid var(--line-soft);
        border-radius: var(--radius);
        padding: 14px;
        display: grid;
        gap: 10px;
        box-shadow: 0 1px 0 rgba(255, 255, 255, 0.03) inset;
      }
      .preview { gap: 10px; }
      .stage {
        position: relative;
        width: 100%;
        aspect-ratio: 16 / 9;
        border-radius: var(--radius-sm);
        background:
          linear-gradient(180deg, #14181c, var(--stage));
        border: 1px solid var(--line-soft);
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
        color: var(--muted);
        font-size: 12px;
        line-height: 1.4;
        pointer-events: none;
      }
      .stage.live .stage-empty { display: none; }
      .row, .choices, .tools { display: flex; gap: 8px; flex-wrap: wrap; }
      .tools { gap: 6px; }
      button, label.toggle {
        appearance: none;
        border: 1px solid var(--line);
        background: var(--card-lift);
        color: var(--text);
        border-radius: 999px;
        padding: 8px 13px;
        font: inherit;
        cursor: pointer;
        transition: background 120ms ease, border-color 120ms ease, color 120ms ease;
      }
      button:hover, label.toggle:hover {
        border-color: color-mix(in srgb, var(--accent) 45%, var(--line));
      }
      button:active, label.toggle:active { transform: scale(0.98); }
      button.on, label.toggle.on {
        background: var(--accent);
        border-color: var(--accent);
        color: var(--accent-ink);
      }
      /* Keep unselected choice buttons visually stable — no pulse/opacity flicker */
      .choices button:not(.on) {
        background: var(--card-lift);
        color: var(--text);
        opacity: 1;
      }
      .tools button { flex: 1; text-align: center; min-width: 64px; }
      .stops button { flex: 1; min-width: 0; padding: 8px 0; }
      label.toggle {
        display: inline-flex;
        align-items: center;
        gap: 7px;
        user-select: none;
      }
      label.toggle input { position: absolute; opacity: 0; pointer-events: none; }
      .slider { display: grid; gap: 5px; }
      .slider span, .heading {
        display: flex;
        justify-content: space-between;
        align-items: center;
        color: var(--muted);
        font-size: 12px;
      }
      .slider strong, .heading strong { color: var(--text); font-weight: 600; }
      input[type="range"] {
        width: 100%;
        accent-color: var(--accent);
        height: 1.4rem;
      }
      .preview-bar {
        display: flex;
        justify-content: space-between;
        align-items: center;
        gap: 10px;
      }
      .preview-bar .heading { flex: 1; }
      .guide {
        display: grid;
        gap: 8px;
        padding: 11px 12px;
        border-radius: var(--radius-sm);
        background: rgba(255, 255, 255, 0.03);
        border: 1px solid var(--line-soft);
      }
      .guide-lead {
        margin: 0;
        font-size: 12.5px;
        line-height: 1.45;
        color: var(--text);
      }
      .guide-lead b {
        color: var(--accent);
        font-weight: 650;
      }
      .guide-uses {
        display: grid;
        grid-template-columns: 1fr 1fr;
        gap: 6px;
      }
      .guide-uses.three { grid-template-columns: 1fr; }
      @media (min-width: 380px) {
        .guide-uses.three { grid-template-columns: 1fr 1fr 1fr; }
      }
      .guide-use {
        padding: 8px 9px;
        border-radius: 10px;
        background: var(--card-lift);
        border: 1px solid var(--line-soft);
        min-width: 0;
      }
      .guide-use span {
        display: block;
        font-size: 10px;
        letter-spacing: 0.06em;
        text-transform: uppercase;
        color: var(--accent);
        font-weight: 650;
        margin-bottom: 3px;
      }
      .guide-use p {
        margin: 0;
        font-size: 11.5px;
        line-height: 1.4;
        color: var(--muted);
      }
      .guide-use.warn span { color: #e0a070; }
      .guide-use.warn {
        border-color: color-mix(in srgb, #e0a070 35%, var(--line-soft));
        background: color-mix(in srgb, #e0a070 8%, var(--card-lift));
      }
      .field-tip {
        margin: -2px 0 2px;
        font-size: 11.5px;
        line-height: 1.4;
        color: var(--muted);
      }
      .field-tip b { color: var(--text); font-weight: 600; }
      p.quiet {
        margin: 0;
        color: var(--muted);
        font-size: 12px;
        line-height: 1.45;
      }
      p.quiet.warn {
        color: #e0a070;
        padding: 8px 10px;
        border-radius: 10px;
        background: color-mix(in srgb, #e0a070 10%, transparent);
        border: 1px solid color-mix(in srgb, #e0a070 30%, var(--line-soft));
      }
      [hidden] { display: none !important; }
    </style>
    </head>
    <body>
    <main>
      <header>
        <h1>Camera desk</h1>
        <p id="status-line">Waiting for the computer</p>
        <p id="hint-line">1080p30 · the computer sets the rate while it is connected</p>
      </header>
      <section class="preview">
        <div class="preview-bar">
          <div class="heading"><span>Preview</span><strong id="preview-label">Off</strong></div>
          <label class="toggle"><input id="show-preview" type="checkbox">Show preview</label>
        </div>
        <div class="guide">
          <p class="guide-lead">This is a <b>glance feed</b> only (~12 fps). Judge smoothness in OBS or your call app.</p>
          <div class="guide-uses">
            <div class="guide-use"><span>Use for</span><p>Framing checks while you tweak controls at the desk.</p></div>
            <div class="guide-use warn"><span>Not for</span><p>Judging chop, blur quality, or final look — that lives on the computer.</p></div>
          </div>
        </div>
        <div class="stage" id="stage">
          <img id="preview-image" alt="Desk preview" decoding="async">
          <div class="stage-empty" id="stage-empty">Turn on Show preview when you need a desk glance. The live picture stays on the computer.</div>
        </div>
      </section>
      <section class="shot">
        <div class="heading"><span>Lens &amp; framing</span></div>
        <div class="guide">
          <p class="guide-lead">Pick the camera that matches the shot, then frame with zoom. Keep torch off unless the room is very dark.</p>
          <div class="guide-uses three">
            <div class="guide-use"><span>Streams</span><p>Back camera + 1–2× for talking-head. Front for face-to-camera.</p></div>
            <div class="guide-use"><span>Recording</span><p>Stay near optical stops (1× / 2×). Heavy digital zoom softens detail.</p></div>
            <div class="guide-use"><span>Flip / torch</span><p>Flip if the phone is upside-down on a mount. Torch eats battery and looks harsh on face cams.</p></div>
          </div>
        </div>
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
        <div class="guide" id="exposure-auto-note">
          <p class="guide-lead">Leave <b>Auto</b> on for most streams and videos. Use EV to brighten or darken without locking shutter.</p>
          <div class="guide-uses">
            <div class="guide-use"><span>Streams / calls</span><p>Auto + a little EV. The phone tracks lights as you move.</p></div>
            <div class="guide-use"><span>Recording</span><p>Still start on Auto. Switch to Manual only if the room light is fixed and Auto hunts.</p></div>
          </div>
        </div>
        <div class="guide" id="exposure-manual-note" hidden>
          <p class="guide-lead"><b>ISO</b> brightens (and adds grain). <b>Shutter</b> freezes motion — keep it fast at 30 fps.</p>
          <div class="guide-uses three">
            <div class="guide-use"><span>ISO</span><p>Raise ISO to get brighter when shutter is already at 1/60+. Prefer the lowest ISO that still looks bright enough.</p></div>
            <div class="guide-use"><span>Shutter</span><p>At 30 fps use <b>1/60 or faster</b>. 1/30 or slower smears motion and feels choppy — that is not lag.</p></div>
            <div class="guide-use"><span>Recording tip</span><p>Lock ISO + shutter once the look is right so brightness does not drift mid-take.</p></div>
          </div>
        </div>
        <label class="slider" id="ev-row">
          <span>EV <strong id="ev-value">0.0</strong></span>
          <input id="ev" type="range" min="-3" max="3" step="0.1" value="0">
        </label>
        <p class="field-tip" id="ev-tip">EV is overall brightness while Auto still picks shutter and ISO.</p>
        <div id="manual-exposure-fields" hidden>
          <label class="slider">
            <span>ISO <strong id="iso-value">100</strong></span>
            <input id="iso" type="range" min="0" max="1" step="0.001" value="0.2">
          </label>
          <p class="field-tip">Sensor sensitivity. Higher = brighter + more noise. Use this before slowing shutter.</p>
          <label class="slider">
            <span>Shutter <strong id="shutter-value">1/60</strong></span>
            <input id="shutter" type="range" min="0" max="1" step="0.001" value="0.5">
          </label>
          <p class="field-tip">How long each frame is open. Fast = crisp motion. Slow = smear that looks like stutter.</p>
          <p class="quiet warn" id="shutter-warn" hidden>Shutter is slower than 1/60. Expect motion smear at 30 fps — raise ISO instead.</p>
        </div>
      </section>
      <section id="tool-whiteBalance" hidden>
        <div class="heading"><span>White balance</span></div>
        <div class="guide">
          <p class="guide-lead">Sets how warm or cool skin and walls look. Match the light you are under.</p>
          <div class="guide-uses three">
            <div class="guide-use"><span>Streams</span><p>Auto is fine under mixed desk lights. Pick a preset if skin looks green or orange.</p></div>
            <div class="guide-use"><span>Recording</span><p>Lock a preset so color does not shift when someone walks by with a lamp.</p></div>
            <div class="guide-use"><span>Quick map</span><p>Daylight = window. Incandescent = warm bulbs. Fluorescent = office tubes. Cloudy = soft overcast.</p></div>
          </div>
        </div>
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
        <div class="guide">
          <p class="guide-lead">Keep <b>Auto</b> for talking-head streams. Use Manual when you want the focus plane locked.</p>
          <div class="guide-uses">
            <div class="guide-use"><span>Streams / calls</span><p>Auto. Tap the phone preview if you need a quick re-focus on your face.</p></div>
            <div class="guide-use"><span>Recording</span><p>Manual after you set distance — stops hunting when you gesture toward the lens.</p></div>
          </div>
        </div>
        <label class="slider" id="focus-row" hidden>
          <span>Position <strong id="focus-value">1.00</strong></span>
          <input id="focus" type="range" min="0" max="1" step="0.01" value="1">
        </label>
        <p class="field-tip" id="focus-tip" hidden>Near = closer subjects. Far = distance / background. Set it, then leave it alone.</p>
      </section>
      <section id="tool-portrait" hidden>
        <div class="heading"><span>Portrait</span><label class="toggle"><input id="portrait" type="checkbox">On</label></div>
        <div class="guide">
          <p class="guide-lead">Phone blur is a light backup. Prefer <b>NVIDIA Broadcast</b> or <b>OBS Background Removal</b> on the PC for streams and recordings.</p>
          <div class="guide-uses three">
            <div class="guide-use"><span>Streams</span><p>Do blur on the PC so the phone stays cool and the feed stays smooth.</p></div>
            <div class="guide-use"><span>Recording</span><p>Same — PC removal usually looks cleaner and keeps 1080p30 stable.</p></div>
            <div class="guide-use warn"><span>If you use this</span><p>Do not stack with PC background tools. It warms the iPhone and can drop frames.</p></div>
          </div>
        </div>
        <p class="quiet" id="portrait-on-hint" hidden>Portrait is on this iPhone. Turn it off if NVIDIA Broadcast or OBS is already cutting the background.</p>
        <label class="slider" id="portrait-row" hidden>
          <span>Strength <strong id="portrait-value">65%</strong></span>
          <input id="portrait-strength" type="range" min="0" max="1" step="0.01" value="0.65">
        </label>
        <p class="field-tip" id="portrait-strength-tip" hidden>Start around 50–70%. Higher softens the background more and costs more heat.</p>
      </section>
      <section>
        <div class="heading"><span>Stream</span><strong id="bitrate-value">4000 kbps</strong></div>
        <div class="guide">
          <p class="guide-lead">Resolution, frame rate, and bitrate set how heavy the phone works and how sharp OBS gets.</p>
          <div class="guide-uses three">
            <div class="guide-use"><span>Live / calls</span><p><b>1080p30</b> · about <b>4 Mbps</b>. Smooth and kind to heat and upload.</p></div>
            <div class="guide-use"><span>Recording in OBS</span><p>Still 1080p30; raise bitrate to <b>6–8 Mbps</b> for cleaner files.</p></div>
            <div class="guide-use warn"><span>Avoid first</span><p>60 fps and very high bitrate until you know the phone stays cool on a long take.</p></div>
          </div>
        </div>
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
        <p class="field-tip">Bitrate is how many bits per second leave the phone. Higher = sharper, more heat and bandwidth.</p>
      </section>
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
        document.getElementById("ev-tip").hidden = exposure;
        document.getElementById("manual-exposure-fields").hidden = !exposure;
        document.getElementById("exposure-auto-note").hidden = exposure;
        document.getElementById("exposure-manual-note").hidden = !exposure;
        document.getElementById("focus-row").hidden = !focusManual;
        document.getElementById("focus-tip").hidden = !focusManual;
        const portraitOn = document.getElementById("portrait").checked;
        document.getElementById("portrait-row").hidden = !portraitOn;
        document.getElementById("portrait-strength-tip").hidden = !portraitOn;
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
        updateShutterWarn();
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
            const battery = data.battery != null ? data.battery + "%" : "";
            const size = session.width + "×" + session.height + " @" + session.fps;
            document.getElementById("status-line").textContent = [
              live ? "Streaming" : "Waiting for the computer",
              battery,
              size
            ].filter(Boolean).join(" · ");
            document.getElementById("hint-line").textContent = live
              ? (previewOn
                ? "Desk preview is a glance (~12 fps) · judge smoothness in OBS"
                : "Changes apply live · judge smoothness in OBS, not this page")
              : "Connect the computer first, then use these controls from the desk";
          }
          if (catalog.ok) {
            const text = await catalog.text();
            document.getElementById("fps-60").hidden = text.indexOf("@60fps") === -1;
          }
        } catch (_) {
        }
      }

      showTool();
      setPreview(false);
      poll();
      setInterval(poll, 500);
    </script>
    </body>
    </html>
    """#
}
