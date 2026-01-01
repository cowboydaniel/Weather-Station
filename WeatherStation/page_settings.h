#pragma once
#include <WiFiS3.h>

static void sendPageSettings(WiFiClient &client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html; charset=utf-8");
  client.println("Cache-Control: public, max-age=300");
  client.println("Connection: close");
  client.println();

  client.println(R"HTML(
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Settings</title>

<link rel="icon" href="/static/favicon.svg">
<link rel="stylesheet" href="/static/app.css">
<script src="/static/app.js"></script>
</head>

<body class="page-settings" style="--bg-layer: radial-gradient(1200px 700px at 20% 10%, rgba(160,160,160,0.15), transparent 60%), radial-gradient(900px 600px at 80% 25%, rgba(110,160,255,0.12), transparent 55%), radial-gradient(800px 800px at 60% 115%, rgba(255,110,180,0.08), transparent 60%), var(--bg); --wrap-max:720px; --wrap-pad:24px 16px 44px;">
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
const NULL_EL = new Proxy({
  style: {},
  classList: { add(){}, remove(){}, toggle(){}, contains(){ return false; } },
  addEventListener(){}, setAttribute(){}
}, {
  get(target, prop) { return prop in target ? target[prop] : undefined; },
  set(target, prop, value) { target[prop] = value; return true; }
});
const el = (id) => {
  const node = document.getElementById(id);
  if (!node) {
    console.warn(`Missing element #${id}`);
    return NULL_EL;
  }
  return node;
};

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
