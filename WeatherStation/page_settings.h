#pragma once
#include <WiFiS3.h>

static void sendPageSettings(WiFiClient &client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html; charset=utf-8");
  client.println("Cache-Control: no-store");
  client.println("Connection: close");
  client.println();

  client.println(R"HTML(
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Settings</title>

<link rel="icon" href="data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 64 64'%3E%3Cdefs%3E%3CradialGradient id='g' cx='30%25' cy='30%25' r='80%25'%3E%3Cstop offset='0%25' stop-color='%23e0e0e0'/%3E%3Cstop offset='45%25' stop-color='%23a0a0a0'/%3E%3Cstop offset='100%25' stop-color='%23606060'/%3E%3C/radialGradient%3E%3C/defs%3E%3Crect x='6' y='6' width='52' height='52' rx='14' fill='url(%23g)'/%3E%3Ccircle cx='32' cy='32' r='10' fill='none' stroke='white' stroke-width='3'/%3E%3Cg fill='white'%3E%3Crect x='30' y='10' width='4' height='8' rx='2'/%3E%3Crect x='30' y='46' width='4' height='8' rx='2'/%3E%3Crect x='10' y='30' width='8' height='4' rx='2'/%3E%3Crect x='46' y='30' width='8' height='4' rx='2'/%3E%3Crect x='16' y='16' width='4' height='8' rx='2' transform='rotate(45 18 20)'/%3E%3Crect x='44' y='40' width='4' height='8' rx='2' transform='rotate(45 46 44)'/%3E%3Crect x='16' y='40' width='4' height='8' rx='2' transform='rotate(-45 18 44)'/%3E%3Crect x='44' y='16' width='4' height='8' rx='2' transform='rotate(-45 46 20)'/%3E%3C/g%3E%3C/svg%3E">

<style>
:root{
  --bg:#070a18;
  --panel: rgba(255,255,255,0.06);
  --panel2: rgba(255,255,255,0.04);
  --border: rgba(255,255,255,0.12);
  --text: rgba(255,255,255,0.92);
  --muted: rgba(255,255,255,0.62);
  --shadow: 0 16px 40px rgba(0,0,0,0.45);
  --radius: 20px;
  --accent: #7aa7ff;
}
*{ box-sizing:border-box; }
body{
  margin:0;
  font-family: ui-sans-serif, system-ui, -apple-system, Segoe UI, Roboto, Arial;
  color:var(--text);
  background:
    radial-gradient(1200px 700px at 20% 10%, rgba(160,160,160,0.15), transparent 60%),
    radial-gradient(900px 600px at 80% 25%, rgba(110,160,255,0.12), transparent 55%),
    radial-gradient(800px 800px at 60% 115%, rgba(255,110,180,0.08), transparent 60%),
    var(--bg);
  min-height:100vh;
}
body.light{
  --bg:#f5f5f7;
  --panel: rgba(0,0,0,0.04);
  --panel2: rgba(0,0,0,0.02);
  --border: rgba(0,0,0,0.10);
  --text: rgba(0,0,0,0.88);
  --muted: rgba(0,0,0,0.55);
  --shadow: 0 16px 40px rgba(0,0,0,0.12);
  background:
    radial-gradient(1200px 700px at 20% 10%, rgba(110,160,255,0.12), transparent 60%),
    radial-gradient(900px 600px at 80% 25%, rgba(255,110,180,0.08), transparent 55%),
    var(--bg);
}
.wrap{ max-width: 720px; margin:0 auto; padding:24px 16px 44px; }
.top{
  display:flex; justify-content:space-between; align-items:flex-end; gap:12px;
}
h1{ margin:0; font-size:20px; }
.sub{ margin-top:6px; font-size:13px; color:var(--muted); line-height:1.4; }
a.back{
  text-decoration:none;
  color:var(--text);
  border:1px solid var(--border);
  background:var(--panel);
  padding:10px 12px;
  border-radius:999px;
  font-weight:650;
  font-size:13px;
}
.card{
  margin-top:14px;
  background:linear-gradient(180deg,var(--panel), var(--panel2));
  border:1px solid var(--border);
  border-radius:var(--radius);
  box-shadow:var(--shadow);
  padding:20px;
  backdrop-filter: blur(12px);
}
.section-title{
  font-size:14px;
  font-weight:650;
  text-transform:uppercase;
  letter-spacing:0.5px;
  color:var(--muted);
  margin-bottom:16px;
  display:flex;
  align-items:center;
  gap:8px;
}
.section-title svg{ width:18px; height:18px; opacity:0.7; }
.field{
  display:flex;
  justify-content:space-between;
  align-items:center;
  padding:14px 0;
  border-bottom:1px solid var(--border);
}
.field:last-child{ border-bottom:none; }
.field-label{
  font-size:14px;
  font-weight:500;
}
.field-desc{
  font-size:12px;
  color:var(--muted);
  margin-top:4px;
}
.field-control{
  display:flex;
  align-items:center;
  gap:8px;
}
input[type="number"], input[type="text"]{
  background:var(--panel);
  border:1px solid var(--border);
  border-radius:10px;
  padding:10px 12px;
  color:var(--text);
  font-size:14px;
  width:100px;
  text-align:right;
}
input[type="number"]:focus, input[type="text"]:focus{
  outline:none;
  border-color:var(--accent);
}
select{
  background:var(--panel);
  border:1px solid var(--border);
  border-radius:10px;
  padding:10px 12px;
  color:var(--text);
  font-size:14px;
  cursor:pointer;
}
select:focus{ outline:none; border-color:var(--accent); }

/* Toggle switch */
.toggle{
  position:relative;
  width:52px;
  height:28px;
}
.toggle input{
  opacity:0;
  width:0;
  height:0;
}
.toggle-slider{
  position:absolute;
  cursor:pointer;
  top:0; left:0; right:0; bottom:0;
  background:var(--panel);
  border:1px solid var(--border);
  border-radius:14px;
  transition: background 0.2s, border-color 0.2s;
}
.toggle-slider:before{
  position:absolute;
  content:"";
  height:20px;
  width:20px;
  left:3px;
  bottom:3px;
  background:white;
  border-radius:50%;
  transition: transform 0.2s;
}
.toggle input:checked + .toggle-slider{
  background:var(--accent);
  border-color:var(--accent);
}
.toggle input:checked + .toggle-slider:before{
  transform:translateX(24px);
}

.btn{
  background:var(--accent);
  color:#fff;
  border:none;
  border-radius:10px;
  padding:12px 20px;
  font-size:14px;
  font-weight:600;
  cursor:pointer;
  transition: opacity 0.2s;
}
.btn:hover{ opacity:0.85; }
.btn-secondary{
  background:var(--panel);
  border:1px solid var(--border);
  color:var(--text);
}
.actions{
  margin-top:20px;
  display:flex;
  gap:10px;
  justify-content:flex-end;
}
.note{
  margin-top:14px;
  padding:14px;
  background:rgba(255,200,100,0.08);
  border:1px solid rgba(255,200,100,0.20);
  border-radius:12px;
  font-size:12px;
  color:var(--muted);
  line-height:1.5;
}
.note b{ color:var(--text); }
.unit-label{
  font-size:12px;
  color:var(--muted);
  min-width:40px;
}
.saved-msg{
  position:fixed;
  bottom:30px;
  left:50%;
  transform:translateX(-50%);
  background:rgba(120,255,180,0.95);
  color:#000;
  padding:12px 24px;
  border-radius:999px;
  font-weight:600;
  font-size:14px;
  opacity:0;
  transition: opacity 0.3s;
  pointer-events:none;
}
.saved-msg.show{ opacity:1; }

@media (max-width:600px){
  .field{ flex-direction:column; align-items:flex-start; gap:10px; }
  .field-control{ width:100%; justify-content:flex-end; }
}
</style>
</head>

<body>
<div class="wrap">
  <div class="top">
    <div>
      <h1>Settings</h1>
      <div class="sub">Configure display preferences and station parameters.</div>
    </div>
    <a class="back" href="/">Back</a>
  </div>

  <div class="card">
    <div class="section-title">
      <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="5"/><path d="M12 1v2M12 21v2M4.22 4.22l1.42 1.42M18.36 18.36l1.42 1.42M1 12h2M21 12h2M4.22 19.78l1.42-1.42M18.36 5.64l1.42-1.42"/></svg>
      Appearance
    </div>

    <div class="field">
      <div>
        <div class="field-label">Dark theme</div>
        <div class="field-desc">Use dark color scheme</div>
      </div>
      <label class="toggle">
        <input type="checkbox" id="theme-toggle" checked>
        <span class="toggle-slider"></span>
      </label>
    </div>
  </div>

  <div class="card">
    <div class="section-title">
      <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M12 2L2 7l10 5 10-5-10-5z"/><path d="M2 17l10 5 10-5"/><path d="M2 12l10 5 10-5"/></svg>
      Location
    </div>

    <div class="field">
      <div>
        <div class="field-label">Altitude</div>
        <div class="field-desc">Used for sea-level pressure calculation</div>
      </div>
      <div class="field-control">
        <input type="number" id="altitude" step="0.1" value="242">
        <span class="unit-label">m</span>
      </div>
    </div>

    <div class="field">
      <div>
        <div class="field-label">Latitude</div>
        <div class="field-desc">For sunrise/sunset calculations</div>
      </div>
      <div class="field-control">
        <input type="number" id="latitude" step="0.01" value="-33.85">
        <span class="unit-label">deg</span>
      </div>
    </div>

    <div class="field">
      <div>
        <div class="field-label">Longitude</div>
        <div class="field-desc">For sunrise/sunset calculations</div>
      </div>
      <div class="field-control">
        <input type="number" id="longitude" step="0.01" value="151.21">
        <span class="unit-label">deg</span>
      </div>
    </div>

    <div class="note">
      <b>Note:</b> Altitude affects the Arduino's sea-level pressure calculation. To change it on the device, update <code>ALTITUDE_M</code> in the Arduino sketch and re-upload. Lat/Long are used client-side for astronomy calculations.
    </div>
  </div>

  <div class="card">
    <div class="section-title">
      <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><polyline points="22 12 18 12 15 21 9 3 6 12 2 12"/></svg>
      Graphs
    </div>

    <div class="field">
      <div>
        <div class="field-label">Refresh rate</div>
        <div class="field-desc">How often to fetch new data</div>
      </div>
      <div class="field-control">
        <select id="refresh-rate">
          <option value="1000">1 second</option>
          <option value="2000" selected>2 seconds</option>
          <option value="5000">5 seconds</option>
          <option value="10000">10 seconds</option>
        </select>
      </div>
    </div>

    <div class="field">
      <div>
        <div class="field-label">Graph time span</div>
        <div class="field-desc">Visible history window (limited by buffer)</div>
      </div>
      <div class="field-control">
        <select id="graph-span">
          <option value="60">1 minute</option>
          <option value="300">5 minutes</option>
          <option value="600" selected>10 minutes (full)</option>
        </select>
      </div>
    </div>

    <div class="field">
      <div>
        <div class="field-label">Show grid lines</div>
        <div class="field-desc">Display horizontal grid on graphs</div>
      </div>
      <label class="toggle">
        <input type="checkbox" id="show-grid" checked>
        <span class="toggle-slider"></span>
      </label>
    </div>
  </div>

  <div class="card">
    <div class="section-title">
      <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M4 15s1-1 4-1 5 2 8 2 4-1 4-1V3s-1 1-4 1-5-2-8-2-4 1-4 1z"/><line x1="4" y1="22" x2="4" y2="15"/></svg>
      Units
    </div>

    <div class="field">
      <div>
        <div class="field-label">Temperature</div>
        <div class="field-desc">Display temperature unit</div>
      </div>
      <div class="field-control">
        <select id="temp-unit">
          <option value="C" selected>Celsius (C)</option>
          <option value="F">Fahrenheit (F)</option>
        </select>
      </div>
    </div>

    <div class="field">
      <div>
        <div class="field-label">Pressure</div>
        <div class="field-desc">Display pressure unit</div>
      </div>
      <div class="field-control">
        <select id="pressure-unit">
          <option value="hPa" selected>hPa</option>
          <option value="inHg">inHg</option>
          <option value="mmHg">mmHg</option>
        </select>
      </div>
    </div>
  </div>

  <div class="actions">
    <button class="btn btn-secondary" id="reset-btn">Reset to defaults</button>
    <button class="btn" id="save-btn">Save settings</button>
  </div>
</div>

<div class="saved-msg" id="saved-msg">Settings saved!</div>

<script>
const el = (id) => document.getElementById(id);

// Default settings
const defaults = {
  theme: 'dark',
  altitude: 242,
  latitude: -33.85,
  longitude: 151.21,
  refreshRate: 2000,
  graphSpan: 600,
  showGrid: true,
  tempUnit: 'C',
  pressureUnit: 'hPa'
};

// Load settings from localStorage
function loadSettings() {
  const saved = localStorage.getItem('weatherSettings');
  if (saved) {
    try {
      return { ...defaults, ...JSON.parse(saved) };
    } catch (e) {
      return defaults;
    }
  }
  return defaults;
}

// Save settings to localStorage
function saveSettings(settings) {
  localStorage.setItem('weatherSettings', JSON.stringify(settings));
}

// Apply settings to form
function applyToForm(settings) {
  el('theme-toggle').checked = settings.theme === 'dark';
  el('altitude').value = settings.altitude;
  el('latitude').value = settings.latitude;
  el('longitude').value = settings.longitude;
  el('refresh-rate').value = settings.refreshRate;
  el('graph-span').value = settings.graphSpan;
  el('show-grid').checked = settings.showGrid;
  el('temp-unit').value = settings.tempUnit;
  el('pressure-unit').value = settings.pressureUnit;

  applyTheme(settings.theme);
}

// Get settings from form
function getFromForm() {
  return {
    theme: el('theme-toggle').checked ? 'dark' : 'light',
    altitude: parseFloat(el('altitude').value) || defaults.altitude,
    latitude: parseFloat(el('latitude').value) || defaults.latitude,
    longitude: parseFloat(el('longitude').value) || defaults.longitude,
    refreshRate: parseInt(el('refresh-rate').value) || defaults.refreshRate,
    graphSpan: parseInt(el('graph-span').value) || defaults.graphSpan,
    showGrid: el('show-grid').checked,
    tempUnit: el('temp-unit').value,
    pressureUnit: el('pressure-unit').value
  };
}

// Apply theme
function applyTheme(theme) {
  if (theme === 'light') {
    document.body.classList.add('light');
  } else {
    document.body.classList.remove('light');
  }
}

// Show saved message
function showSavedMsg() {
  const msg = el('saved-msg');
  msg.classList.add('show');
  setTimeout(() => msg.classList.remove('show'), 2000);
}

// Initialize
const settings = loadSettings();
applyToForm(settings);

// Theme toggle live preview
el('theme-toggle').addEventListener('change', () => {
  applyTheme(el('theme-toggle').checked ? 'dark' : 'light');
});

// Save button
el('save-btn').addEventListener('click', () => {
  const newSettings = getFromForm();
  saveSettings(newSettings);
  showSavedMsg();
});

// Reset button
el('reset-btn').addEventListener('click', () => {
  if (confirm('Reset all settings to defaults?')) {
    applyToForm(defaults);
    saveSettings(defaults);
    showSavedMsg();
  }
});
</script>
</body>
</html>
)HTML");
}
