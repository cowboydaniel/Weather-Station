#pragma once
#include <WiFiS3.h>

static void sendPageStats(WiFiClient &client) {
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
<title>System Stats</title>

<link rel="icon" href="/static/favicon.svg">
<link rel="stylesheet" href="/static/app.css">
<script src="/static/app.js"></script>
</head>

<body class="page-stats" style="--bg-layer: radial-gradient(1200px 700px at 20% 10%, rgba(255,160,100,0.15), transparent 60%), radial-gradient(900px 600px at 80% 25%, rgba(110,160,255,0.12), transparent 55%), radial-gradient(800px 800px at 60% 115%, rgba(255,200,100,0.10), transparent 60%), var(--bg); --wrap-max:1020px; --wrap-pad:24px 16px 44px;">
<div class="wrap">
  <div class="top">
    <div>
      <h1>System Stats</h1>
      <div class="sub">Network, Arduino, and sensor diagnostics.</div>
    </div>
    <div style="display:flex; gap:10px;">
      <button class="refresh-btn" id="refresh-btn">
        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M23 4v6h-6"/><path d="M1 20v-6h6"/><path d="M3.51 9a9 9 0 0114.85-3.36L23 10M1 14l4.64 4.36A9 9 0 0020.49 15"/></svg>
        Refresh
      </button>
      <a class="back" href="/">Back</a>
    </div>
  </div>

  <div class="grid">
    <div class="card">
      <div class="section-title">
        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M5 12.55a11 11 0 0114 0"/><path d="M1.42 9a16 16 0 0121.16 0"/><path d="M8.53 16.11a6 6 0 016.95 0"/><circle cx="12" cy="20" r="1"/></svg>
        Network
      </div>

      <div class="stat-row">
        <span class="stat-label">Status</span>
        <span class="stat-value"><span class="status-dot green" id="net-status-dot"></span><span id="net-status">Connected</span></span>
      </div>
      <div class="stat-row">
        <span class="stat-label">IP Address</span>
        <span class="stat-value" id="ip-addr">--</span>
      </div>
      <div class="stat-row">
        <span class="stat-label">SSID</span>
        <span class="stat-value" id="ssid">--</span>
      </div>
      <div class="stat-row">
        <span class="stat-label">Signal Strength (RSSI)</span>
        <span class="stat-value" id="rssi">--</span>
      </div>
      <div class="stat-row" style="flex-direction:column; align-items:stretch;">
        <div style="display:flex; justify-content:space-between;">
          <span class="stat-label">Signal Quality</span>
          <span class="stat-value" id="signal-quality">--%</span>
        </div>
        <div class="bar-container">
          <div class="bar-fill green" id="signal-bar" style="width:0%"></div>
        </div>
      </div>
      <div class="stat-row">
        <span class="stat-label">MAC Address</span>
        <span class="stat-value" id="mac">--</span>
      </div>
    </div>

    <div class="card">
      <div class="section-title">
        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><rect x="4" y="4" width="16" height="16" rx="2"/><rect x="9" y="9" width="6" height="6"/><line x1="9" y1="1" x2="9" y2="4"/><line x1="15" y1="1" x2="15" y2="4"/><line x1="9" y1="20" x2="9" y2="23"/><line x1="15" y1="20" x2="15" y2="23"/><line x1="20" y1="9" x2="23" y2="9"/><line x1="20" y1="14" x2="23" y2="14"/><line x1="1" y1="9" x2="4" y2="9"/><line x1="1" y1="14" x2="4" y2="14"/></svg>
        Arduino
      </div>

      <div class="stat-row">
        <span class="stat-label">Board</span>
        <span class="stat-value" id="board">Arduino UNO R4 WiFi</span>
      </div>
      <div class="stat-row">
        <span class="stat-label">Uptime</span>
        <span class="stat-value" id="uptime">--</span>
      </div>
      <div class="stat-row">
        <span class="stat-label">Free RAM</span>
        <span class="stat-value" id="free-ram">--</span>
      </div>
      <div class="stat-row" style="flex-direction:column; align-items:stretch;">
        <div style="display:flex; justify-content:space-between;">
          <span class="stat-label">Memory Usage</span>
          <span class="stat-value" id="mem-pct">--%</span>
        </div>
        <div class="bar-container">
          <div class="bar-fill green" id="mem-bar" style="width:0%"></div>
        </div>
      </div>
      <div class="stat-row">
        <span class="stat-label">CPU Frequency</span>
        <span class="stat-value" id="cpu-freq">48 MHz</span>
      </div>
      <div class="stat-row">
        <span class="stat-label">Loop Rate</span>
        <span class="stat-value" id="loop-rate">--</span>
      </div>
    </div>

    <div class="card wide">
      <div class="section-title">
        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M14 2H6a2 2 0 00-2 2v16a2 2 0 002 2h12a2 2 0 002-2V8z"/><path d="M14 2v6h6"/><path d="M16 13H8"/><path d="M16 17H8"/><path d="M10 9H8"/></svg>
        Data Buffers
      </div>

      <div class="kpi-grid">
        <div class="kpi-item">
          <div class="kpi-label">Temp Series</div>
          <div class="kpi-value" id="temp-count">--</div>
          <div class="kpi-unit">/ 600 pts</div>
        </div>
        <div class="kpi-item">
          <div class="kpi-label">Humidity Series</div>
          <div class="kpi-value" id="hum-count">--</div>
          <div class="kpi-unit">/ 600 pts</div>
        </div>
        <div class="kpi-item">
          <div class="kpi-label">Pressure Series</div>
          <div class="kpi-value" id="press-count">--</div>
          <div class="kpi-unit">/ 600 pts</div>
        </div>
        <div class="kpi-item">
          <div class="kpi-label">Gas Series</div>
          <div class="kpi-value" id="gas-count">--</div>
          <div class="kpi-unit">/ 200 pts</div>
        </div>
        <div class="kpi-item">
          <div class="kpi-label">SLP Trend</div>
          <div class="kpi-value" id="slp-count">--</div>
          <div class="kpi-unit">/ 60 pts</div>
        </div>
        <div class="kpi-item">
          <div class="kpi-label">Sample Rate</div>
          <div class="kpi-value" id="sample-rate">1</div>
          <div class="kpi-unit">Hz</div>
        </div>
        <div class="kpi-item">
          <div class="kpi-label">Gas Interval</div>
          <div class="kpi-value" id="gas-interval">3</div>
          <div class="kpi-unit">sec</div>
        </div>
        <div class="kpi-item">
          <div class="kpi-label">Total Samples</div>
          <div class="kpi-value" id="total-samples">--</div>
          <div class="kpi-unit">since boot</div>
        </div>
      </div>
    </div>

    <div class="card wide">
      <div class="section-title">
        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M12 2v4"/><path d="M12 18v4"/><path d="M4.93 4.93l2.83 2.83"/><path d="M16.24 16.24l2.83 2.83"/><path d="M2 12h4"/><path d="M18 12h4"/><path d="M4.93 19.07l2.83-2.83"/><path d="M16.24 7.76l2.83-2.83"/></svg>
        Sensor Status
      </div>

      <div class="kpi-grid">
        <div class="kpi-item">
          <div class="kpi-label">Sensor</div>
          <div class="kpi-value" id="sensor-name">BME680</div>
          <div class="kpi-unit" id="sensor-addr">I2C 0x76</div>
        </div>
        <div class="kpi-item">
          <div class="kpi-label">Temperature</div>
          <div class="kpi-value"><span class="status-dot green" id="temp-status"></span><span id="temp-val">--</span></div>
          <div class="kpi-unit">raw C</div>
        </div>
        <div class="kpi-item">
          <div class="kpi-label">Humidity</div>
          <div class="kpi-value"><span class="status-dot green" id="hum-status"></span><span id="hum-val">--</span></div>
          <div class="kpi-unit">raw %</div>
        </div>
        <div class="kpi-item">
          <div class="kpi-label">Pressure</div>
          <div class="kpi-value"><span class="status-dot green" id="press-status"></span><span id="press-val">--</span></div>
          <div class="kpi-unit">raw hPa</div>
        </div>
        <div class="kpi-item">
          <div class="kpi-label">Gas</div>
          <div class="kpi-value"><span class="status-dot green" id="gas-status"></span><span id="gas-val">--</span></div>
          <div class="kpi-unit">raw kOhm</div>
        </div>
        <div class="kpi-item">
          <div class="kpi-label">Heater</div>
          <div class="kpi-value" id="heater-temp">320</div>
          <div class="kpi-unit">C @ 150ms</div>
        </div>
        <div class="kpi-item">
          <div class="kpi-label">Oversampling</div>
          <div class="kpi-value" id="oversampling">4x/2x/4x</div>
          <div class="kpi-unit">T/H/P</div>
        </div>
        <div class="kpi-item">
          <div class="kpi-label">IIR Filter</div>
          <div class="kpi-value" id="iir-filter">7</div>
          <div class="kpi-unit">coefficient</div>
        </div>
      </div>

      <div class="note">
        BME680 is a 4-in-1 environmental sensor: temperature, humidity, barometric pressure, and gas resistance (VOC indicator).
      </div>
    </div>

    <div class="card wide">
      <div class="section-title">
        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="10"/><polyline points="12 6 12 12 16 14"/></svg>
        Request Log
      </div>

      <div class="stat-row">
        <span class="stat-label">Total Requests</span>
        <span class="stat-value" id="total-requests">--</span>
      </div>
      <div class="stat-row">
        <span class="stat-label">API Requests</span>
        <span class="stat-value" id="api-requests">--</span>
      </div>
      <div class="stat-row">
        <span class="stat-label">Page Requests</span>
        <span class="stat-value" id="page-requests">--</span>
      </div>
      <div class="stat-row">
        <span class="stat-label">404 Errors</span>
        <span class="stat-value" id="error-requests">--</span>
      </div>
      <div class="stat-row">
        <span class="stat-label">Avg Response Time</span>
        <span class="stat-value" id="avg-response">--</span>
      </div>
    </div>

    <div class="card wide">
      <div class="section-title">
        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><rect x="3" y="3" width="18" height="18" rx="2"/><path d="M12 7v10"/><path d="M8 11h8"/></svg>
        SD Card Storage
      </div>

      <div class="stat-row">
        <span class="stat-label">Status</span>
        <span class="stat-value"><span class="status-dot green" id="sd-status-dot"></span><span id="sd-status">--</span></span>
      </div>
      <div class="stat-row">
        <span class="stat-label">Total Capacity</span>
        <span class="stat-value" id="sd-total">--</span>
      </div>
      <div class="stat-row">
        <span class="stat-label">Free Space</span>
        <span class="stat-value" id="sd-free">--</span>
      </div>
      <div class="stat-row" style="flex-direction:column; align-items:stretch;">
        <div style="display:flex; justify-content:space-between;">
          <span class="stat-label">Storage Usage</span>
          <span class="stat-value" id="sd-usage-pct">--%</span>
        </div>
        <div class="bar-container">
          <div class="bar-fill green" id="sd-usage-bar" style="width:0%"></div>
        </div>
      </div>
      <div class="stat-row">
        <span class="stat-label">Logged Samples</span>
        <span class="stat-value" id="sd-samples">--</span>
      </div>
    </div>
  </div>
</div>

<script>
const el = (id) => document.getElementById(id);

function formatUptime(ms) {
  const sec = Math.floor(ms / 1000);
  const min = Math.floor(sec / 60);
  const hrs = Math.floor(min / 60);
  const days = Math.floor(hrs / 24);

  if (days > 0) return days + 'd ' + (hrs % 24) + 'h ' + (min % 60) + 'm';
  if (hrs > 0) return hrs + 'h ' + (min % 60) + 'm ' + (sec % 60) + 's';
  if (min > 0) return min + 'm ' + (sec % 60) + 's';
  return sec + 's';
}

function rssiToQuality(rssi) {
  // RSSI typically ranges from -30 (excellent) to -90 (poor)
  if (rssi >= -50) return 100;
  if (rssi <= -100) return 0;
  return Math.round(2 * (rssi + 100));
}

function getBarColor(pct) {
  if (pct >= 70) return 'green';
  if (pct >= 40) return 'yellow';
  return 'red';
}

function setStatus(dotId, isOk) {
  const dot = el(dotId);
  if (dot) {
    dot.className = 'status-dot ' + (isOk ? 'green' : 'red');
  }
}

async function fetchStats() {
  const btn = el('refresh-btn');
  btn.classList.add('loading');

  try {
    const res = await fetch('/api/stats', { cache: 'no-store' });
    const data = await res.json();

    if (!data.ok) throw new Error('API error');

    // Network stats
    el('net-status').textContent = data.network.connected ? 'Connected' : 'Disconnected';
    el('net-status-dot').className = 'status-dot ' + (data.network.connected ? 'green' : 'red');
    el('ip-addr').textContent = data.network.ip || '--';
    el('ssid').textContent = data.network.ssid || '--';
    el('rssi').textContent = data.network.rssi + ' dBm';

    const quality = rssiToQuality(data.network.rssi);
    el('signal-quality').textContent = quality + '%';
    el('signal-bar').style.width = quality + '%';
    el('signal-bar').className = 'bar-fill ' + getBarColor(quality);

    el('mac').textContent = data.network.mac || '--';

    // Arduino stats
    el('uptime').textContent = formatUptime(data.arduino.uptime_ms);
    el('free-ram').textContent = data.arduino.free_ram + ' bytes';

    const memPct = Math.round((1 - data.arduino.free_ram / data.arduino.total_ram) * 100);
    el('mem-pct').textContent = memPct + '%';
    el('mem-bar').style.width = memPct + '%';
    el('mem-bar').className = 'bar-fill ' + getBarColor(100 - memPct);

    el('loop-rate').textContent = data.arduino.loop_rate + ' Hz';

    // Buffer stats
    el('temp-count').textContent = data.buffers.temp_count;
    el('hum-count').textContent = data.buffers.hum_count;
    el('press-count').textContent = data.buffers.press_count;
    el('gas-count').textContent = data.buffers.gas_count;
    el('slp-count').textContent = data.buffers.slp_count;
    el('total-samples').textContent = data.buffers.total_samples;

    // Sensor values
    el('temp-val').textContent = data.sensor.temp_c.toFixed(2);
    el('hum-val').textContent = data.sensor.hum_pct.toFixed(2);
    el('press-val').textContent = data.sensor.press_hpa.toFixed(2);
    el('gas-val').textContent = data.sensor.gas_kohm.toFixed(2);

    setStatus('temp-status', isFinite(data.sensor.temp_c));
    setStatus('hum-status', isFinite(data.sensor.hum_pct));
    setStatus('press-status', isFinite(data.sensor.press_hpa));
    setStatus('gas-status', isFinite(data.sensor.gas_kohm));

    // Request stats
    el('total-requests').textContent = data.requests.total;
    el('api-requests').textContent = data.requests.api;
    el('page-requests').textContent = data.requests.pages;
    el('error-requests').textContent = data.requests.errors;
    el('avg-response').textContent = data.requests.avg_ms.toFixed(1) + ' ms';

    // SD Card stats
    if (data.sd_card) {
      const sdInit = data.sd_card.initialized;
      el('sd-status').textContent = sdInit ? 'Ready' : 'Not initialized';
      el('sd-status-dot').className = 'status-dot ' + (sdInit ? 'green' : 'red');

      if (sdInit) {
        const totalMB = data.sd_card.total_mb;
        const freeMB = data.sd_card.free_mb;
        const usedMB = totalMB - freeMB;
        const usagePct = Math.round((usedMB / totalMB) * 100);

        el('sd-total').textContent = totalMB + ' MB';
        el('sd-free').textContent = freeMB + ' MB';
        el('sd-usage-pct').textContent = usagePct + '%';
        el('sd-usage-bar').style.width = usagePct + '%';
        el('sd-usage-bar').className = 'bar-fill ' + getBarColor(100 - usagePct);
        el('sd-samples').textContent = data.sd_card.logged_samples.toLocaleString();
      } else {
        el('sd-total').textContent = '--';
        el('sd-free').textContent = '--';
        el('sd-usage-pct').textContent = '--%';
        el('sd-usage-bar').style.width = '0%';
        el('sd-samples').textContent = '--';
      }
    }

  } catch (e) {
    el('net-status').textContent = 'Error';
    el('net-status-dot').className = 'status-dot red';
  }

  btn.classList.remove('loading');
}

el('refresh-btn').addEventListener('click', fetchStats);

// Initial fetch
fetchStats();

// Auto-refresh every 5 seconds
setInterval(fetchStats, 5000);
</script>
</body>
</html>
)HTML");
}
