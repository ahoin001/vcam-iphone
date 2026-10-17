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
        --panel: #16191c;
        --line: rgba(255, 255, 255, 0.08);
        --line-strong: rgba(255, 255, 255, 0.14);
        --hover: rgba(255, 255, 255, 0.05);
        --text: #f2efe8;
        --muted: #9a958c;
        --accent: #d9a04a;
        --accent-ink: #1a1408;
        --stage: #0c0e10;
        --warn: #e0a070;
        --desk: 320px;
        --grab: 6px;
        --ease: cubic-bezier(0.23, 1, 0.32, 1);
      }
      * { box-sizing: border-box; }
      html, body { height: 100%; }
      body {
        margin: 0;
        height: 100%;
        overflow: hidden;
        background: var(--bg);
        color: var(--text);
        font: 12.5px/1.4 "Segoe UI Variable Text", "Segoe UI", system-ui, sans-serif;
        font-variant-numeric: tabular-nums;
        -webkit-font-smoothing: antialiased;
      }
      main {
        height: 100%;
        min-height: 0;
        display: grid;
        grid-template-columns: 1fr;
        grid-template-rows: auto auto minmax(0, 1fr);
      }
      header {
        grid-column: 1;
        display: flex;
        align-items: center;
        justify-content: space-between;
        gap: 12px;
        min-height: 36px;
        padding: 8px 12px 7px;
        border-bottom: 1px solid var(--line);
      }
      .header-actions { display: flex; align-items: center; gap: 8px; min-width: 0; }
      #status-line {
        margin: 0;
        font-size: 12.5px;
        font-weight: 600;
        letter-spacing: -0.01em;
        text-wrap: pretty;
      }
      #hint-line {
        margin: 0;
        color: var(--muted);
        font-size: 11.5px;
        text-align: right;
        text-wrap: pretty;
      }
      .preview {
        grid-column: 1;
        display: grid;
        gap: 8px;
        padding: 10px 12px 12px;
        border-bottom: 1px solid var(--line);
        background: var(--panel);
      }
      .preview-bar {
        display: flex;
        justify-content: space-between;
        align-items: center;
        gap: 10px;
        color: var(--muted);
        font-size: 11px;
      }
      .preview-bar strong { color: var(--text); font-weight: 600; }
      .stage {
        position: relative;
        width: 100%;
        aspect-ratio: 16 / 9;
        max-height: 160px;
        border-radius: 8px;
        background: var(--stage);
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
        outline: 1px solid oklch(1 0 0 / 0.1);
        outline-offset: -1px;
      }
      .stage.live img { display: block; }
      .stage-empty {
        position: absolute;
        inset: 0;
        display: grid;
        place-items: center;
        padding: 12px;
        text-align: center;
        color: var(--muted);
        font-size: 11.5px;
        pointer-events: none;
      }
      .stage.live .stage-empty { display: none; }
      .grabber {
        display: none;
        touch-action: none;
        cursor: col-resize;
        position: relative;
      }
      .grabber::before {
        content: "";
        position: absolute;
        inset: 20% 2px;
        border-radius: 99px;
        background: var(--line-strong);
      }
      .grabber:hover::before, .grabber.drag::before { background: var(--accent); }
      .controls {
        min-height: 0;
        overflow: auto;
        background: var(--panel);
      }
      details.folder {
        border-bottom: 1px solid var(--line);
      }
      details.folder > summary {
        list-style: none;
        display: grid;
        grid-template-columns: 12px auto minmax(0, 1fr) 22px;
        align-items: center;
        gap: 8px;
        min-height: 32px;
        padding: 0 8px 0 12px;
        cursor: pointer;
        user-select: none;
      }
      details.folder > summary::-webkit-details-marker { display: none; }
      details.folder > summary:hover { background: var(--hover); }
      .chev {
        width: 8px;
        height: 8px;
        border-right: 1.5px solid var(--muted);
        border-bottom: 1.5px solid var(--muted);
        transform: rotate(-45deg);
        justify-self: center;
        transition: transform 160ms var(--ease);
      }
      details[open] > summary .chev { transform: rotate(45deg); }
      summary .name { font-weight: 600; font-size: 12px; white-space: nowrap; }
      summary .sum {
        color: var(--muted);
        font-size: 11.5px;
        font-weight: 500;
        text-align: right;
        min-width: 0;
        overflow: hidden;
        text-overflow: ellipsis;
        white-space: nowrap;
      }
      .body {
        display: grid;
        gap: 8px;
        padding: 0 12px 12px 32px;
      }
      .prop {
        display: grid;
        grid-template-columns: 72px minmax(0, 1fr);
        align-items: center;
        gap: 8px;
        min-height: 28px;
      }
      .prop > span {
        color: var(--muted);
        font-size: 11.5px;
      }
      .prop.stack {
        grid-template-columns: 1fr;
        gap: 4px;
      }
      .row, .choices { display: flex; gap: 4px; flex-wrap: wrap; }
      button, label.toggle {
        appearance: none;
        border: 1px solid var(--line-strong);
        background: rgba(255, 255, 255, 0.04);
        color: var(--text);
        border-radius: 6px;
        padding: 5px 9px;
        font: inherit;
        font-size: 11.5px;
        cursor: pointer;
        transition: background-color 120ms var(--ease), border-color 120ms var(--ease), color 120ms var(--ease), transform 140ms var(--ease);
      }
      button:hover, label.toggle:hover { background: var(--hover); }
      button:active, label.toggle:active { transform: scale(0.96); }
      button.on, label.toggle.on {
        background: var(--accent);
        border-color: var(--accent);
        color: var(--accent-ink);
      }
      button:disabled { cursor: default; opacity: 0.45; transform: none; }
      button:disabled:not(.on):hover { background: rgba(255, 255, 255, 0.04); }
      button.on:disabled { opacity: 0.7; }
      .choices button, .stops button { flex: 1; min-width: 0; text-align: center; }
      .prop > label.toggle, .prop > .choices, .prop > .row { justify-self: start; }
      label.toggle {
        display: inline-flex;
        align-items: center;
        gap: 6px;
        user-select: none;
        width: max-content;
      }
      label.toggle input { position: absolute; opacity: 0; pointer-events: none; }
      .slider { display: grid; gap: 2px; }
      .slider span {
        display: flex;
        justify-content: space-between;
        color: var(--muted);
        font-size: 11px;
      }
      .slider strong { color: var(--text); font-weight: 600; }
      input[type="range"] {
        width: 100%;
        accent-color: var(--accent);
        height: 1.25rem;
      }
      .hint {
        margin: 0;
        color: var(--muted);
        font-size: 11.5px;
        line-height: 1.4;
        text-wrap: pretty;
      }
      .hint.warn {
        color: var(--warn);
        padding: 6px 8px;
        border-radius: 6px;
        background: color-mix(in srgb, var(--warn) 10%, transparent);
        border: 1px solid color-mix(in srgb, var(--warn) 28%, transparent);
      }
      .line { margin: 0; color: var(--muted); font-size: 11.5px; line-height: 1.35; }
      .info {
        width: 22px; height: 22px; padding: 0;
        border-radius: 999px; color: var(--muted); font-weight: 650;
      }
      .presets {
        display: flex; align-items: center; gap: 6px;
        min-height: 40px; padding: 6px 12px;
        border-bottom: 1px solid var(--line);
      }
      .presets .name { font-weight: 600; font-size: 12px; }
      .presets select, .preset-form input {
        min-width: 0; height: 28px; flex: 1;
        border-radius: 6px; border: 1px solid var(--line-strong);
        background: rgba(255, 255, 255, 0.04); color: var(--text);
        padding: 0 8px; font: inherit; font-size: 11.5px;
      }
      .preset-form { display: flex; gap: 6px; padding: 0 12px 8px; }
      .popover {
        position: fixed; z-index: 30;
        width: min(300px, calc(100vw - 24px));
        padding: 12px 14px; display: grid; gap: 8px;
        border-radius: 12px; border: 1px solid var(--line-strong);
        background: #1c2126; color: var(--text);
        box-shadow: 0 16px 40px rgba(0, 0, 0, 0.35);
      }
      .popover[hidden] { display: none !important; }
      .popover strong { font-size: 12.5px; }
      .popover p { margin: 0; color: var(--muted); font-size: 12px; line-height: 1.45; }
      .popover .pair { color: var(--warn); }
      .guide-layer {
        position: fixed; inset: 0; z-index: 40;
        display: grid; place-items: center;
        padding: 18px;
        background: rgba(8, 9, 10, 0.72);
      }
      .guide-card {
        width: min(860px, 100%);
        max-height: min(720px, 100%);
        overflow: auto;
        display: grid; gap: 14px;
        padding: 18px 18px 22px;
        border-radius: 16px;
        border: 1px solid var(--line-strong);
        background:
          radial-gradient(80% 50% at 100% 0%, rgba(217, 160, 74, 0.12), transparent 50%),
          #16191c;
      }
      .guide-head, .guide-tabs { display: flex; align-items: center; gap: 8px; }
      .guide-head { justify-content: space-between; }
      .guide-head h2 { margin: 0; font-size: 18px; font-weight: 650; }
      .guide-tabs { flex-wrap: wrap; }
      .guide-tabs button.on { background: var(--accent); border-color: var(--accent); color: var(--accent-ink); }
      .guide-panel { display: none; gap: 12px; }
      .guide-panel.on { display: grid; }
      .guide-panel p { margin: 0; color: var(--muted); font-size: 13px; line-height: 1.5; }
      .guide-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; }
      .guide-card .note {
        padding: 12px 14px; border-radius: 12px;
        border: 1px solid var(--line); background: rgba(255, 255, 255, 0.03);
      }
      .guide-card .note strong { display: block; margin-bottom: 4px; color: var(--text); font-size: 13px; }
      .lights { display: grid; grid-template-columns: 220px minmax(0, 1fr); gap: 14px; align-items: center; }
      .lights svg { width: 100%; height: auto; }
      @media (max-width: 700px) {
        .guide-grid, .lights { grid-template-columns: 1fr; }
      }
      [hidden] { display: none !important; }
      :focus-visible { outline: 2px solid var(--accent); outline-offset: 2px; }

      @media (min-width: 720px) {
        body:not(.companion) main {
          grid-template-columns: minmax(0, 1fr) var(--grab) var(--desk);
          grid-template-rows: auto minmax(0, 1fr);
        }
        body:not(.companion) header { grid-column: 1 / -1; }
        body:not(.companion) .preview {
          grid-column: 1;
          grid-row: 2;
          min-height: 0;
          border-bottom: 0;
          border-right: 1px solid var(--line);
          align-content: start;
        }
        body:not(.companion) .stage { max-height: none; aspect-ratio: auto; height: min(100%, 100%); min-height: 140px; }
        body:not(.companion) .grabber { display: block; grid-column: 2; grid-row: 2; }
        body:not(.companion) .controls { grid-column: 3; grid-row: 2; }
      }
      body.companion .preview,
      body.companion .grabber { display: none !important; }
      body.companion main {
        grid-template-columns: 1fr;
        grid-template-rows: auto minmax(0, 1fr);
      }
      body.companion .controls { grid-row: 2; }
    </style>
    </head>
    <body>
    <main>
      <header>
        <p id="status-line">Waiting for the computer</p>
        <div class="header-actions">
          <p id="hint-line">1080p30</p>
          <button id="open-guide" type="button">Guide</button>
        </div>
      </header>
      <section class="preview" id="preview-pane">
        <div class="preview-bar">
          <span>Preview <strong id="preview-label">Off</strong></span>
          <label class="toggle"><input id="show-preview" type="checkbox">Show</label>
        </div>
        <div class="stage" id="stage">
          <img id="preview-image" alt="Desk preview" decoding="async">
          <div class="stage-empty" id="stage-empty">Waiting for a frame from the iPhone.</div>
        </div>
      </section>
      <div class="grabber" id="grabber" role="separator" aria-orientation="vertical" aria-label="Resize desk"></div>
      <div class="controls" id="desk">
        <div class="presets">
          <span class="name">Presets</span>
          <select id="preset-list" aria-label="Saved looks"><option value="">No saved looks</option></select>
          <button id="preset-save" type="button">Save</button>
          <button id="preset-delete" type="button" hidden>Delete</button>
          <button class="info" type="button" data-pop="presets" aria-label="About presets">?</button>
        </div>
        <form class="preset-form" id="preset-form" hidden>
          <input id="preset-name" maxlength="40" placeholder="Name this look" autocomplete="off">
          <button type="submit">Save look</button>
        </form>
        <details class="folder" open data-key="lens">
          <summary><span class="chev"></span><span class="name">Lens</span><span class="sum" id="lens-sum">Back · 1.0×</span><button class="info" type="button" data-pop="lens" aria-label="Lens advice">?</button></summary>
          <div class="body">
            <p class="line">Back camera for a talking head. Stay near 1× or 2×.</p>
            <div class="prop">
              <span>Camera</span>
              <div class="choices" id="lens">
                <button type="button" data-value="back">Back</button>
                <button type="button" data-value="front">Front</button>
              </div>
            </div>
            <div class="prop stack">
              <span>Zoom <strong id="zoom-value">1.0×</strong></span>
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
            </div>
            <div class="prop">
              <span>Shot</span>
              <div class="row">
                <label class="toggle"><input id="torch" type="checkbox">Torch</label>
                <label class="toggle"><input id="flip" type="checkbox">Flip 180°</label>
              </div>
            </div>
          </div>
        </details>
        <details class="folder" open data-key="exposure">
          <summary><span class="chev"></span><span class="name">Exposure</span><span class="sum" id="exposure-sum">Auto · EV 0.0</span><button class="info" type="button" data-pop="exposure" aria-label="Exposure advice">?</button></summary>
          <div class="body">
            <p class="line">Set shutter for motion, then ISO for brightness.</p>
            <div class="prop">
              <span>Mode</span>
              <label class="toggle"><input id="manual-exposure" type="checkbox">Manual</label>
            </div>
            <p class="line" id="ev-tip">EV brightens or darkens. Auto still picks shutter and ISO.</p>
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
              <p class="hint warn" id="shutter-warn" hidden>Slower than 1/60 smears motion. Raise ISO instead.</p>
            </div>
          </div>
        </details>
        <details class="folder" data-key="whiteBalance">
          <summary><span class="chev"></span><span class="name">White balance</span><span class="sum" id="wb-sum">Auto</span><button class="info" type="button" data-pop="whiteBalance" aria-label="White balance advice">?</button></summary>
          <div class="body">
            <p class="line">Lock this to the key light so skin color stays put.</p>
            <div class="choices" id="wb">
              <button type="button" data-value="auto">Auto</button>
              <button type="button" data-value="daylight">Daylight</button>
              <button type="button" data-value="cloudy">Cloudy</button>
              <button type="button" data-value="fluorescent">Fluorescent</button>
              <button type="button" data-value="incandescent">Incandescent</button>
            </div>
          </div>
        </details>
        <details class="folder" data-key="focus">
          <summary><span class="chev"></span><span class="name">Focus</span><span class="sum" id="focus-sum">Auto</span><button class="info" type="button" data-pop="focus" aria-label="Focus advice">?</button></summary>
          <div class="body">
            <p class="line">Manual once you are sitting still.</p>
            <div class="prop">
              <span>Mode</span>
              <label class="toggle"><input id="manual-focus" type="checkbox">Manual</label>
            </div>
            <label class="slider" id="focus-row" hidden>
              <span>Position <strong id="focus-value">1.00</strong></span>
              <input id="focus" type="range" min="0" max="1" step="0.01" value="1">
            </label>
            <p class="line" id="focus-tip" hidden>Near is closer. Far is the background. Set it, then leave it.</p>
          </div>
        </details>
        <details class="folder" data-key="portrait">
          <summary><span class="chev"></span><span class="name">Portrait</span><span class="sum" id="portrait-sum">Off</span><button class="info" type="button" data-pop="portrait" aria-label="Portrait advice">?</button></summary>
          <div class="body">
            <p class="line">Leave Portrait off if the PC already cuts the background.</p>
            <div class="prop">
              <span>Phone blur</span>
              <label class="toggle"><input id="portrait" type="checkbox">On</label>
            </div>
            <p class="hint warn" id="portrait-on-hint" hidden>Portrait is on. Turn it off if NVIDIA Broadcast or OBS is already cutting the background.</p>
            <label class="slider" id="portrait-row" hidden>
              <span>Strength <strong id="portrait-value">65%</strong></span>
              <input id="portrait-strength" type="range" min="0" max="1" step="0.01" value="0.65">
            </label>
          </div>
        </details>
        <details class="folder" data-key="stream">
          <summary><span class="chev"></span><span class="name">Stream</span><span class="sum" id="stream-sum">1080p30 · 4 Mbps</span><button class="info" type="button" data-pop="stream" aria-label="Stream advice">?</button></summary>
          <div class="body">
            <p class="line">The heading is the rate in use. The slider is the saved OBS rate.</p>
            <div class="prop">
              <span>Size</span>
              <div class="choices" id="resolution">
                <button type="button" data-width="1280" data-height="720">720</button>
                <button type="button" data-width="1920" data-height="1080">1080</button>
              </div>
            </div>
            <div class="prop">
              <span>Frame rate</span>
              <div class="choices" id="fps">
                <button type="button" data-fps="30">30</button>
                <button type="button" id="fps-60" data-fps="60" hidden>60</button>
              </div>
            </div>
            <p class="line" id="mode-note" hidden>VCamPanel holds 1080p30 while it is connected. Size and frame rate apply to the optional OBS source.</p>
            <div class="prop stack">
              <span>Bitrate <strong id="bitrate-value">4000 kbps</strong></span>
              <div class="choices" id="bitrate-presets">
                <button type="button" data-rate="4000">4 Mbps</button>
                <button type="button" data-rate="6000">6 Mbps</button>
                <button type="button" data-rate="8000">8 Mbps</button>
              </div>
              <label class="slider">
                <span><strong id="bitrate-saved">4000 kbps</strong></span>
                <input id="bitrate" type="range" min="2500" max="16000" step="100" value="4000">
              </label>
            </div>
          </div>
        </details>
      </div>
    </main>
    <div class="popover" id="popover" hidden role="dialog"></div>
    <div class="guide-layer" id="guide" hidden>
      <div class="guide-card" role="dialog" aria-labelledby="guide-title">
        <div class="guide-head">
          <h2 id="guide-title">Guide</h2>
          <button id="guide-close" type="button">Done</button>
        </div>
        <div class="guide-tabs" role="tablist">
          <button type="button" class="on" data-guide="light">Lighting</button>
          <button type="button" data-guide="exposure">Exposure</button>
          <button type="button" data-guide="color">Color</button>
          <button type="button" data-guide="situations">Situations</button>
        </div>
        <section class="guide-panel on" data-guide-panel="light">
          <div class="lights">
            <svg viewBox="0 0 220 160" aria-hidden="true">
              <rect width="220" height="160" rx="16" fill="#101316"/>
              <circle cx="110" cy="78" r="16" fill="#f2efe8"/>
              <rect x="96" y="96" width="28" height="34" rx="8" fill="#c8c2b6"/>
              <circle cx="46" cy="48" r="10" fill="#d9a04a"/>
              <path d="M54 54 L96 74" stroke="#d9a04a" stroke-width="2"/>
              <circle cx="176" cy="62" r="7" fill="#8aa0b4"/>
              <path d="M170 66 L126 80" stroke="#8aa0b4" stroke-width="2"/>
              <circle cx="110" cy="22" r="6" fill="#e7d7b2"/>
              <path d="M110 28 L110 60" stroke="#e7d7b2" stroke-width="2"/>
              <text x="28" y="148" fill="#9a958c" font-size="11" font-family="Segoe UI, sans-serif">Key</text>
              <text x="150" y="148" fill="#9a958c" font-size="11" font-family="Segoe UI, sans-serif">Fill</text>
            </svg>
            <div class="guide-grid">
              <div class="note"><strong>Key</strong><p>The main light, slightly off camera and a little above the eyes. Soften it. A bare LED speckles on camera.</p></div>
              <div class="note"><strong>Fill</strong><p>The other side, dimmer than the key, so one side of the face stays a little darker.</p></div>
              <div class="note"><strong>Back</strong><p>A light behind you on the hair or shoulders separates you from the wall.</p></div>
              <div class="note"><strong>Window</strong><p>A window behind you silhouettes the face unless the key in front is brighter than the window.</p></div>
            </div>
          </div>
        </section>
        <section class="guide-panel" data-guide-panel="exposure">
          <p>Shutter and ISO brighten the same picture in different ways. Set the shutter for how motion should look, then use ISO only for the light you still need.</p>
          <div class="guide-grid">
            <div class="note"><strong>Shutter first</strong><p>At 30 fps use 1/60. That is double the frame rate. Faster looks stuttery. Slower than 1/60 smears a head turn or a hand.</p></div>
            <div class="note"><strong>ISO second</strong><p>Start near 100. Raise it if the face is still dark. Grain usually shows past roughly 800. Do not slow the shutter to avoid raising ISO.</p></div>
            <div class="note"><strong>Auto</strong><p>Auto plus a little EV when you move between rooms. The phone keeps the 1/60-style motion and changes ISO for you.</p></div>
            <div class="note"><strong>Manual</strong><p>Use manual when the lights stay on for the whole session. Lock 1/60, then ride ISO.</p></div>
          </div>
        </section>
        <section class="guide-panel" data-guide-panel="color">
          <p>Auto white balance hunts when a monitor or a colored lamp enters the frame. Lock it to the key light once that light is on.</p>
          <div class="guide-grid">
            <div class="note"><strong>Daylight, 5200K</strong><p>Window light and neutral LEDs. The usual starting point for a talking head.</p></div>
            <div class="note"><strong>Incandescent, 3200K</strong><p>Warm desk lamps. Lower numbers make skin warmer. If the face goes orange, the lock is warmer than the lamp.</p></div>
            <div class="note"><strong>Fluorescent, 4000K</strong><p>Office tubes and a lot of cheap panels.</p></div>
            <div class="note"><strong>Mixed light</strong><p>Window plus a warm lamp paints the face two colors. Turn one of them off, then lock white balance.</p></div>
          </div>
        </section>
        <section class="guide-panel" data-guide-panel="situations">
          <p>Dial the look, then save it as a preset. Switch presets when the time of day or the lamps change. Leave Portrait off if NVIDIA Broadcast or OBS is already cutting the background.</p>
          <div class="guide-grid">
            <div class="note"><strong>Day, window beside you</strong><p>Back camera, 1× or 2×, Auto exposure, white balance Daylight.</p></div>
            <div class="note"><strong>Evening lamps</strong><p>Manual, shutter 1/60, ISO 200 to 800 until the face is right, white balance Incandescent.</p></div>
            <div class="note"><strong>Overhead light only</strong><p>Eyes fall into shadow. Add a key in front before you raise ISO. Torch is a last resort: it is harsh and it drains the battery.</p></div>
            <div class="note"><strong>Stream or recording</strong><p>Use the same exposure either way. The bitrate slider is the saved rate for an optional OBS source. It does not change the VCamdroid picture while VCamPanel is connected.</p></div>
          </div>
        </section>
      </div>
    </div>
    <script>
      const active = new Set();
      const pending = new Map();
      const confirmed = {};
      const seen = {};
      const intents = new Map();
      const intentMs = 2500;
      const controlKeys = ["lens", "zoom", "manualExposure", "exposureBias", "iso", "shutter", "whiteBalance",
        "manualFocus", "focus", "torch", "flip", "portrait", "portraitStrength", "bitrateKbps", "activeBitrateKbps"];
      const sessionKeys = ["mode", "panelClient"];
      let epoch = 0;
      let timer = 0;
      let inflight = false;
      let polling = false;
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
      const grabber = document.getElementById("grabber");
      const companion = new URLSearchParams(location.search).get("companion") === "1";
      const isoMin = 50, isoMax = 6400;
      const shutterMin = 0.0001, shutterMax = 0.5;
      const wbNames = { auto: "Auto", daylight: "Daylight", cloudy: "Cloudy", fluorescent: "Fluorescent", incandescent: "Incandescent" };

      function text(id, value) {
        const node = document.getElementById(id);
        if (node && node.textContent !== value) node.textContent = value;
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

      function same(a, b) {
        if (a === undefined || b === undefined || a === null || b === null) return a === b;
        if (typeof a === "number" || typeof b === "number") {
          const x = Number(a);
          const y = Number(b);
          return Math.abs(x - y) <= Math.max(0.0005, Math.abs(y) * 0.001);
        }
        return String(a) === String(b);
      }

      function desired(key) {
        const intent = intents.get(key);
        return intent ? intent.value : confirmed[key];
      }

      function intend(key, value) {
        epoch += 1;
        intents.set(key, { value, at: Date.now() });
      }

      // A control only changes for the user's own click, the reply to it, or a value two polls agree on.
      // A response that started before the click is dropped, so a stale poll cannot flip a button back.
      function ingest(data, keys, trusted) {
        const now = Date.now();
        for (const key of keys) {
          if (!(key in data)) continue;
          const value = data[key];
          const intent = intents.get(key);
          if (intent) {
            if (same(value, intent.value)) {
              intents.delete(key);
              confirmed[key] = value;
              seen[key] = value;
              continue;
            }
            if (now - intent.at < intentMs) continue;
            intents.delete(key);
          }
          if (trusted || !(key in confirmed) || same(seen[key], value)) confirmed[key] = value;
          seen[key] = value;
        }
        render();
      }

      function sessionValues(data) {
        const values = { panelClient: !!data.panelClient };
        if (data.width && data.height && data.fps) values.mode = data.width + "x" + data.height + "@" + data.fps;
        return values;
      }

      function currentMode() {
        const match = /^(\d+)x(\d+)@(\d+)$/.exec(String(desired("mode") || ""));
        return match
          ? { width: Number(match[1]), height: Number(match[2]), fps: Number(match[3]) }
          : { width: 1920, height: 1080, fps: 30 };
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
        const sent = [...pending.keys()];
        for (const [key, value] of pending) params.set(key, value);
        pending.clear();
        try {
          const response = await fetch("/v1/controls?" + params.toString(), { method: "PUT", cache: "no-store" });
          if (response.ok) {
            const data = await response.json();
            if (data.success === false) sent.forEach((key) => intents.delete(key));
            ingest(data, controlKeys, true);
          }
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
          const value = Number(read ? read(input.value) : input.value);
          intend(key, value);
          text(label, format(value));
          queue(key, value);
        });
        input.addEventListener("change", () => {
          hold(key, false);
          const value = Number(read ? read(input.value) : input.value);
          intend(key, value);
          render();
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
          intend(key, button.dataset.value);
          render();
          queue(key, button.dataset.value);
          flush();
        });
      }
      bindChoice(document.getElementById("lens"), "lens");
      bindChoice(document.getElementById("wb"), "whiteBalance");

      document.getElementById("zoom-stops").addEventListener("click", (event) => {
        const button = event.target.closest("button");
        if (!button) return;
        const value = Number(button.dataset.stop);
        intend("zoom", value);
        render();
        queue("zoom", value);
        flush();
      });

      function bindFlag(id, key) {
        const input = document.getElementById(id);
        input.addEventListener("change", () => {
          intend(key, input.checked);
          render();
          queue(key, input.checked ? "1" : "0");
          flush();
        });
      }
      bindFlag("torch", "torch");
      bindFlag("flip", "flip");
      bindFlag("manual-exposure", "manualExposure");
      bindFlag("manual-focus", "manualFocus");
      bindFlag("portrait", "portrait");

      document.getElementById("resolution").addEventListener("click", (event) => {
        const button = event.target.closest("button");
        if (!button || button.disabled) return;
        putSession(Number(button.dataset.width), Number(button.dataset.height), currentMode().fps);
      });
      document.getElementById("fps").addEventListener("click", (event) => {
        const button = event.target.closest("button");
        if (!button || button.disabled) return;
        const mode = currentMode();
        putSession(mode.width, mode.height, Number(button.dataset.fps));
      });
      document.getElementById("bitrate-presets").addEventListener("click", (event) => {
        const button = event.target.closest("button");
        if (!button) return;
        const value = Number(button.dataset.rate);
        intend("bitrateKbps", value);
        render();
        queue("bitrateKbps", value);
        flush();
      });

      function setRange(input, key) {
        const value = desired(key);
        if (value === undefined) return;
        if (!active.has(key)) {
          const next = input._write ? input._write(value) : value;
          if (!same(Number(input.value), Number(next))) input.value = next;
        }
        text(input._label, input._format(value));
      }

      function markButtons(group, isOn) {
        for (const button of group.querySelectorAll("button")) {
          const on = !!isOn(button);
          if (button.classList.contains("on") !== on) button.classList.toggle("on", on);
          if (button.getAttribute("aria-pressed") !== String(on)) button.setAttribute("aria-pressed", String(on));
        }
      }

      function setFlag(id, key) {
        const value = desired(key);
        if (value === undefined) return;
        const input = document.getElementById(id);
        const next = !!value;
        if (input.checked !== next) input.checked = next;
        if (input.parentElement.classList.contains("on") !== next) input.parentElement.classList.toggle("on", next);
      }

      function render() {
        markButtons(document.getElementById("lens"), (button) => button.dataset.value === desired("lens"));
        markButtons(document.getElementById("wb"), (button) => button.dataset.value === desired("whiteBalance"));
        setRange(zoom, "zoom");
        const zoomValue = desired("zoom");
        markButtons(document.getElementById("zoom-stops"),
          (button) => zoomValue !== undefined && Math.abs(Number(button.dataset.stop) - Number(zoomValue)) < 0.05);
        setRange(ev, "exposureBias");
        setRange(iso, "iso");
        setRange(shutter, "shutter");
        setRange(focus, "focus");
        setRange(portraitStrength, "portraitStrength");
        setFlag("torch", "torch");
        setFlag("flip", "flip");
        setFlag("manual-exposure", "manualExposure");
        setFlag("manual-focus", "manualFocus");
        setFlag("portrait", "portrait");
        const rate = desired("bitrateKbps");
        if (rate !== undefined) {
          setRange(bitrate, "bitrateKbps");
          const activeRate = confirmed.activeBitrateKbps !== undefined ? confirmed.activeBitrateKbps : rate;
          text("bitrate-value", Math.round(Number(activeRate)) + " kbps");
          markButtons(document.getElementById("bitrate-presets"),
            (button) => Number(button.dataset.rate) === Math.round(Number(rate)));
        }
        const mode = currentMode();
        const locked = !!confirmed.panelClient;
        markButtons(document.getElementById("resolution"),
          (button) => Number(button.dataset.width) === mode.width && Number(button.dataset.height) === mode.height);
        markButtons(document.getElementById("fps"), (button) => Number(button.dataset.fps) === mode.fps);
        for (const button of document.querySelectorAll("#resolution button, #fps button")) {
          if (button.disabled !== locked) button.disabled = locked;
        }
        const note = document.getElementById("mode-note");
        if (note && note.hidden === locked) note.hidden = !locked;
        showManual();
      }

      function showManual() {
        const exposure = document.getElementById("manual-exposure").checked;
        const focusManual = document.getElementById("manual-focus").checked;
        document.getElementById("ev-row").hidden = exposure;
        document.getElementById("ev-tip").hidden = exposure;
        document.getElementById("manual-exposure-fields").hidden = !exposure;
        document.getElementById("focus-row").hidden = !focusManual;
        document.getElementById("focus-tip").hidden = !focusManual;
        const portraitOn = document.getElementById("portrait").checked;
        document.getElementById("portrait-row").hidden = !portraitOn;
        document.getElementById("portrait-on-hint").hidden = !portraitOn;
        updateShutterWarn();
        updateSummaries();
      }

      function updateShutterWarn() {
        const warn = document.getElementById("shutter-warn");
        if (!warn) return;
        const manual = document.getElementById("manual-exposure").checked;
        const seconds = logFrom(shutter.value, shutterMin, shutterMax);
        warn.hidden = !(manual && seconds > (1 / 60) + 0.0005);
      }

      function updateSummaries() {
        const lens = document.querySelector("#lens button.on");
        const zoomLabel = document.getElementById("zoom-value").textContent;
        text("lens-sum", (lens ? lens.textContent : "Back") + " · " + zoomLabel);
        const manual = document.getElementById("manual-exposure").checked;
        text("exposure-sum", manual
          ? "ISO " + document.getElementById("iso-value").textContent + " · " + document.getElementById("shutter-value").textContent
          : "Auto · EV " + document.getElementById("ev-value").textContent);
        const wb = document.querySelector("#wb button.on");
        text("wb-sum", wb ? (wbNames[wb.dataset.value] || wb.textContent) : "Auto");
        const focusManual = document.getElementById("manual-focus").checked;
        text("focus-sum", focusManual ? document.getElementById("focus-value").textContent : "Auto");
        const portraitOn = document.getElementById("portrait").checked;
        text("portrait-sum", portraitOn ? document.getElementById("portrait-value").textContent : "Off");
        const mode = currentMode();
        const size = (mode.height >= 1080 ? "1080p" : "720p") + mode.fps;
        text("stream-sum", size + " · " + document.getElementById("bitrate-value").textContent);
      }

      async function putSession(width, height, fps) {
        if (confirmed.panelClient) return;
        intend("mode", width + "x" + height + "@" + fps);
        render();
        const issued = epoch;
        const params = new URLSearchParams({ width: String(width), height: String(height), fps: String(fps) });
        try {
          const response = await fetch("/v1/session?" + params.toString(), { method: "PUT", cache: "no-store" });
          if (!response.ok) return;
          const data = await response.json();
          if (data.success === false) intents.delete("mode");
          if (issued === epoch) ingest(sessionValues(data), sessionKeys, true);
        } catch (_) {
        }
      }

      async function refreshControls() {
        const started = epoch;
        try {
          const response = await fetch("/v1/controls", { cache: "no-store" });
          if (response.ok && started === epoch) ingest(await response.json(), controlKeys, true);
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
        const empty = document.getElementById("stage-empty");
        if (empty) {
          empty.textContent = on
            ? "Waiting for a frame from the iPhone."
            : "Turn on Show when you need a glance. The live picture stays on the computer.";
        }
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

      function loadDeskWidth() {
        try {
          const saved = Number(localStorage.getItem("vcam-desk"));
          if (saved >= 260 && saved <= 520) {
            document.documentElement.style.setProperty("--desk", saved + "px");
          }
        } catch (_) {}
      }

      function bindGrabber() {
        if (companion) return;
        let startX = 0;
        let startW = 320;
        grabber.addEventListener("pointerdown", (event) => {
          grabber.classList.add("drag");
          grabber.setPointerCapture(event.pointerId);
          startX = event.clientX;
          startW = parseFloat(getComputedStyle(document.documentElement).getPropertyValue("--desk")) || 320;
        });
        grabber.addEventListener("pointermove", (event) => {
          if (!grabber.classList.contains("drag")) return;
          const next = Math.max(260, Math.min(520, startW - (event.clientX - startX)));
          document.documentElement.style.setProperty("--desk", next + "px");
        });
        function endDrag(event) {
          if (!grabber.classList.contains("drag")) return;
          grabber.classList.remove("drag");
          try {
            localStorage.setItem("vcam-desk", String(parseFloat(getComputedStyle(document.documentElement).getPropertyValue("--desk")) || 320));
          } catch (_) {}
          if (grabber.hasPointerCapture(event.pointerId)) grabber.releasePointerCapture(event.pointerId);
        }
        grabber.addEventListener("pointerup", endDrag);
        grabber.addEventListener("pointercancel", endDrag);
      }

      // VCamPanel keeps the layout in its own settings file, because this page's origin changes with the phone's IP.
      const nativeHost = companion && window.chrome && window.chrome.webview ? window.chrome.webview : null;

      function readFolders() {
        if (nativeHost) {
          const match = /[#&]layout=([^&]*)/.exec(location.hash);
          if (match) {
            try { return JSON.parse(decodeURIComponent(match[1])) || {}; } catch (_) {}
          }
        }
        try { return JSON.parse(localStorage.getItem("vcam-folders") || "{}") || {}; } catch (_) { return {}; }
      }

      function writeFolders(saved) {
        const body = JSON.stringify(saved);
        if (nativeHost) {
          try { nativeHost.postMessage("desk-layout:" + body); } catch (_) {}
          history.replaceState(null, "", location.pathname + location.search + "#layout=" + encodeURIComponent(body));
        }
        try { localStorage.setItem("vcam-folders", body); } catch (_) {}
      }

      function restoreFolders() {
        const saved = readFolders();
        document.querySelectorAll("details.folder").forEach((el) => {
          if (Object.prototype.hasOwnProperty.call(saved, el.dataset.key)) el.open = !!saved[el.dataset.key];
        });
        document.getElementById("desk").addEventListener("toggle", (event) => {
          if (!event.target.classList.contains("folder")) return;
          const next = {};
          document.querySelectorAll("details.folder").forEach((el) => { next[el.dataset.key] = el.open; });
          writeFolders(next);
        }, true);
      }

      async function poll() {
        if (polling || inflight || pending.size) return;
        polling = true;
        const started = epoch;
        const fresh = () => started === epoch && !inflight && pending.size === 0;
        try {
          const [controls, snapshot, catalog] = await Promise.all([
            fetch("/v1/controls", { cache: "no-store" }),
            fetch("/v1/session", { cache: "no-store" }),
            fetch("/v1/resolutions", { cache: "no-store" })
          ]);
          if (controls.ok) {
            const data = await controls.json();
            if (fresh()) ingest(data, controlKeys, false);
          }
          if (snapshot.ok) {
            const data = await snapshot.json();
            if (fresh()) ingest(sessionValues(data), sessionKeys, false);
            const live = data.streaming || data.status === "streaming";
            const battery = data.battery != null ? data.battery + "%" : "";
            const size = data.width && data.height ? data.width + "×" + data.height + " @" + data.fps : "";
            text("status-line", [
              live ? "Streaming" : "Waiting for the computer",
              battery,
              size
            ].filter(Boolean).join(" · "));
            text("hint-line", live
              ? (companion ? "Next frame updates this window and VCamdroid apps." : "Changes apply live")
              : "The picture is opening.");
          }
          if (catalog.ok) {
            const body = await catalog.text();
            const sixty = /"fps"\s*:\s*60\b|@60fps/.test(body);
            const button = document.getElementById("fps-60");
            if (button.hidden === sixty) button.hidden = !sixty;
          }
        } catch (_) {
        } finally {
          polling = false;
        }
      }

      const advice = {
        lens: ["Lens", "Use the back camera for a talking head and the front camera when you need to face the screen. Digital zoom past 2× softens detail, so frame with 1× or 2× when you can. Flip if the phone is upside down on a mount. Torch is only for a dark corner: it is harsh and it drains the battery."],
        exposure: ["ISO and shutter", "These two brighten the same picture in different ways. Shutter decides motion: at 30 fps lock 1/60, which is double the frame rate. Faster looks stuttery. Slower smears movement. ISO decides grain: start near 100 and raise it only if the face is still dark. Do not slow the shutter to avoid a higher ISO. Auto plus EV is enough when the room light changes."],
        whiteBalance: ["White balance", "Lock this to the key light. Daylight (5200K) for a window or neutral LEDs. Incandescent (3200K) for warm lamps. Fluorescent (4000K) for office tubes. Auto hunts when a monitor or a second color of light enters the frame. If skin goes orange, the lock is warmer than the lamp. If it goes blue, the lock is cooler."],
        focus: ["Focus", "Auto is right when you lean toward the camera. Manual is right once you are sitting still, so the phone does not hunt the background. Near is closer to the lens. Set it, then leave it."],
        portrait: ["Portrait", "Leave Portrait off if NVIDIA Broadcast or OBS is already cutting the background. The phone blur is lighter, it warms the phone, and stacking it with a PC cut makes the edges double. Use it only when you do not have those PC tools."],
        stream: ["Stream", "While VCamPanel is the video client, the picture stays 1080p30 at the Wi-Fi or USB rate. The bitrate slider saves a rate for the optional OBS source. Streams can stay near 4 Mbps. A recording can use 6–8. Moving the slider does not change the VCamdroid picture."],
        presets: ["Presets", "Save the current look under a name for each setup: morning window, evening lamps, overhead office light. Choosing one applies lens, zoom, exposure, white balance, focus, torch, flip, and portrait. The list stays on this iPhone."]
      };

      const popover = document.getElementById("popover");
      function closePop() { popover.hidden = true; }
      function openPop(button) {
        const item = advice[button.dataset.pop];
        if (!item) return;
        const same = popover.dataset.pop === button.dataset.pop && !popover.hidden;
        if (same) { closePop(); return; }
        popover.dataset.pop = button.dataset.pop;
        popover.innerHTML = "<strong></strong><p></p>";
        popover.querySelector("strong").textContent = item[0];
        popover.querySelector("p").textContent = item[1];
        popover.hidden = false;
        const rect = button.getBoundingClientRect();
        const width = popover.offsetWidth;
        const left = Math.max(8, Math.min(rect.right - width, window.innerWidth - width - 8));
        const top = Math.min(rect.bottom + 8, window.innerHeight - popover.offsetHeight - 8);
        popover.style.left = left + "px";
        popover.style.top = Math.max(8, top) + "px";
      }
      document.querySelectorAll(".info").forEach((button) => {
        button.addEventListener("click", (event) => {
          event.preventDefault();
          event.stopPropagation();
          openPop(button);
        });
      });
      document.addEventListener("click", (event) => {
        if (!event.target.closest(".popover") && !event.target.closest(".info")) closePop();
      });
      document.addEventListener("keydown", (event) => {
        if (event.key === "Escape") { closePop(); closeGuide(); }
      });

      const guide = document.getElementById("guide");
      function openGuide() { guide.hidden = false; closePop(); }
      function closeGuide() { guide.hidden = true; }
      window.openGuide = openGuide;
      document.getElementById("open-guide").addEventListener("click", openGuide);
      document.getElementById("guide-close").addEventListener("click", closeGuide);
      guide.addEventListener("click", (event) => { if (event.target === guide) closeGuide(); });
      document.querySelectorAll(".guide-tabs button").forEach((button) => {
        button.addEventListener("click", () => {
          document.querySelectorAll(".guide-tabs button").forEach((item) => item.classList.toggle("on", item === button));
          document.querySelectorAll(".guide-panel").forEach((panel) => {
            panel.classList.toggle("on", panel.dataset.guidePanel === button.dataset.guide);
          });
        });
      });

      const presetList = document.getElementById("preset-list");
      const presetForm = document.getElementById("preset-form");
      const presetName = document.getElementById("preset-name");
      const presetDelete = document.getElementById("preset-delete");
      let presets = [];
      let presetChoice = "";

      function renderPresets() {
        const selected = presetList.value;
        presetList.replaceChildren();
        const empty = document.createElement("option");
        empty.value = "";
        empty.textContent = presets.length ? "Choose a look" : "No saved looks";
        presetList.append(empty);
        for (const preset of presets) {
          const option = document.createElement("option");
          option.value = preset.id;
          option.textContent = preset.name;
          presetList.append(option);
        }
        presetList.value = presets.some((preset) => preset.id === selected) ? selected : presetChoice;
        if (![...presetList.options].some((option) => option.value === presetList.value)) presetList.value = "";
        presetDelete.hidden = !presetList.value;
      }

      async function loadPresets() {
        try {
          const response = await fetch("/v1/presets", { cache: "no-store" });
          if (!response.ok) return;
          const data = await response.json();
          presets = data.presets || [];
          renderPresets();
        } catch (_) {}
      }

      async function sendPreset(query) {
        const applying = query.indexOf("apply=") === 0;
        if (applying) {
          intents.clear();
          epoch += 1;
        }
        const response = await fetch("/v1/presets?" + query, { method: "PUT", cache: "no-store" });
        if (!response.ok) return;
        const data = await response.json();
        presets = data.presets || [];
        if (query.indexOf("name=") === 0 && presets.length) presetChoice = presets[presets.length - 1].id;
        renderPresets();
        if (applying) refreshControls();
      }

      document.getElementById("preset-save").addEventListener("click", () => {
        presetForm.hidden = !presetForm.hidden;
        if (!presetForm.hidden) presetName.focus();
      });
      presetForm.addEventListener("submit", (event) => {
        event.preventDefault();
        const name = presetName.value.trim();
        if (!name) return;
        presetForm.hidden = true;
        presetName.value = "";
        sendPreset("name=" + encodeURIComponent(name));
      });
      presetList.addEventListener("change", () => {
        presetChoice = presetList.value;
        presetDelete.hidden = !presetChoice;
        if (presetChoice) sendPreset("apply=" + encodeURIComponent(presetChoice));
      });
      presetDelete.addEventListener("click", () => {
        if (!presetList.value) return;
        const id = presetList.value;
        presetChoice = "";
        sendPreset("delete=" + encodeURIComponent(id));
      });

      if (companion) document.body.classList.add("companion");
      loadDeskWidth();
      bindGrabber();
      restoreFolders();
      setPreview(!companion);
      loadPresets();
      poll();
      setInterval(poll, 500);
    </script>
    </body>
    </html>
    """#
}
