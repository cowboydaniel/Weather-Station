#pragma once
#include <WiFiS3.h>

// Helper to send the graphs page with specified timeframe
static void sendPageGraphs(WiFiClient &client, const char* timeframe, int timeframeSecs, const char* timeLabel) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html; charset=utf-8");
  client.println("Cache-Control: public, max-age=300");
  client.println("Connection: close");
  client.println();

  // Start HTML
  client.println(R"HTML(
<!doctype html>
<html><head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Graphs</title>
<link rel="icon" href="/static/favicon.svg">
<link rel="stylesheet" href="/static/app.css">
<script src="/static/app.js"></script>
<style>
.timeframe-nav {
  display: flex;
  gap: 8px;
  margin-bottom: 20px;
  flex-wrap: wrap;
}
.timeframe-nav a {
  padding: 10px 20px;
  background: rgba(255,255,255,0.08);
  border: 1px solid rgba(255,255,255,0.15);
  border-radius: 8px;
  color: rgba(255,255,255,0.7);
  text-decoration: none;
  font-size: 14px;
  font-weight: 500;
  transition: all 0.15s;
}
.timeframe-nav a:hover {
  background: rgba(255,255,255,0.12);
  color: rgba(255,255,255,0.9);
}
.timeframe-nav a.active {
  background: rgba(255,255,255,0.18);
  border-color: rgba(255,255,255,0.3);
  color: #fff;
}
.graph-section {
  margin-bottom: 24px;
}
.graph-section h2 {
  font-size: 16px;
  font-weight: 500;
  margin: 0 0 8px 0;
  color: rgba(255,255,255,0.85);
}
.graph-stats {
  display: flex;
  gap: 20px;
  margin-top: 8px;
  font-size: 13px;
}
.graph-stats span {
  color: rgba(255,255,255,0.5);
}
.graph-stats b {
  color: rgba(255,255,255,0.85);
  font-weight: 500;
}
</style>
</head>
<body class="page-chart" style="--bg-layer: radial-gradient(1200px 700px at 15% 10%, rgba(110,160,255,0.22), transparent 60%), radial-gradient(900px 600px at 85% 25%, rgba(255,110,180,0.18), transparent 55%), var(--bg); --wrap-max:1000px; --wrap-pad:22px 14px 30px;">
<div class="wrap">
  <div class="top">
    <div>
      <h1>Graphs</h1>
      <div class="sub" id="subtitle">--</div>
    </div>
    <a class="kpi" href="/">Back</a>
  </div>

  <nav class="timeframe-nav">
    <a href="/graphs/1m" id="nav-1m">1 min</a>
    <a href="/graphs/5m" id="nav-5m">5 min</a>
    <a href="/graphs/10m" id="nav-10m">10 min</a>
    <a href="/graphs/24h" id="nav-24h">24 hours</a>
  </nav>

  <div class="card">
    <div class="graph-section">
      <h2>Temperature</h2>
      <canvas id="cvTemp" class="chart" width="1000" height="200"></canvas>
      <div class="graph-stats">
        <div><span>Now:</span> <b id="tempNow">--</b></div>
        <div><span>Min:</span> <b id="tempMin">--</b></div>
        <div><span>Max:</span> <b id="tempMax">--</b></div>
      </div>
    </div>

    <div class="graph-section">
      <h2>Humidity</h2>
      <canvas id="cvHum" class="chart" width="1000" height="200"></canvas>
      <div class="graph-stats">
        <div><span>Now:</span> <b id="humNow">--</b></div>
        <div><span>Min:</span> <b id="humMin">--</b></div>
        <div><span>Max:</span> <b id="humMax">--</b></div>
      </div>
    </div>

    <div class="graph-section">
      <h2>Pressure</h2>
      <canvas id="cvPress" class="chart" width="1000" height="200"></canvas>
      <div class="graph-stats">
        <div><span>Now:</span> <b id="pressNow">--</b></div>
        <div><span>Min:</span> <b id="pressMin">--</b></div>
        <div><span>Max:</span> <b id="pressMax">--</b></div>
      </div>
    </div>

    <div class="graph-section">
      <h2>Gas Resistance</h2>
      <canvas id="cvGas" class="chart" width="1000" height="200"></canvas>
      <div class="graph-stats">
        <div><span>Now:</span> <b id="gasNow">--</b></div>
        <div><span>Min:</span> <b id="gasMin">--</b></div>
        <div><span>Max:</span> <b id="gasMax">--</b></div>
      </div>
    </div>
  </div>
</div>

<script>
)HTML");

  // Inject timeframe configuration
  client.print("const TIMEFRAME = ");
  client.print(timeframeSecs);
  client.println(";");

  client.print("const TIMEFRAME_LABEL = '");
  client.print(timeLabel);
  client.println("';");

  client.print("const TIMEFRAME_ID = '");
  client.print(timeframe);
  client.println("';");

  // Continue with JavaScript
  client.println(R"HTML(
const USE_DATE_API = TIMEFRAME >= 86400;

// Set active nav and subtitle
document.getElementById('nav-' + TIMEFRAME_ID).classList.add('active');
document.getElementById('subtitle').textContent = TIMEFRAME_LABEL + ' view' + (USE_DATE_API ? ' (from SD card)' : ' (live buffer)');

// Setup canvases
const charts = {
  temp: setupCanvas('cvTemp', 200),
  hum: setupCanvas('cvHum', 200),
  press: setupCanvas('cvPress', 200),
  gas: setupCanvas('cvGas', 200)
};

// Get today's date for SD card queries
function getTodayStr() {
  const d = new Date();
  return d.getFullYear() + '-' + String(d.getMonth() + 1).padStart(2, '0') + '-' + String(d.getDate()).padStart(2, '0');
}

// Slice series to timeframe for live data
function sliceSeries(series, intervalMs) {
  if (USE_DATE_API || !Array.isArray(series) || series.length === 0) return series;

  const hasInterval = typeof intervalMs === 'number' && intervalMs > 0;
  if (!hasInterval) return series;

  const expectedCount = Math.max(1, Math.round((TIMEFRAME * 1000) / intervalMs));
  const sliced = series.slice(-expectedCount);

  if (sliced.length < expectedCount) {
    const padLength = expectedCount - sliced.length;
    const padding = new Array(padLength).fill(null);
    return padding.concat(sliced);
  }

  return sliced;
}

// Draw a graph and update stats
function renderGraph(chart, series, opts, nowEl, minEl, maxEl, unit) {
  const stats = drawLineSeries(chart.ctx, chart.cv, series, opts);
  if (stats) {
    if (nowEl) nowEl.textContent = stats.last.toFixed(1) + unit;
    if (minEl) minEl.textContent = stats.min.toFixed(1) + unit;
    if (maxEl) maxEl.textContent = stats.max.toFixed(1) + unit;
  }
  return stats;
}

// Fetch and render all graphs
async function tick() {
  try {
    // Temperature
    const tempEndpoint = USE_DATE_API ? '/api/temp-date?date=' + getTodayStr() : '/api/temp';
    const tempRes = await fetch(tempEndpoint, {cache:'no-store'});
    const tempData = await tempRes.json();
    if (tempData.ok) {
      let series, timestamps;
      if (tempData.data) {
        // Date API returns [[ts, val], ...]
        timestamps = tempData.data.map(d => d[0]);
        series = tempData.data.map(d => d[1]);
      } else {
        series = sliceSeries(tempData.series || [], tempData.interval_ms);
      }
      renderGraph(charts.temp, series, {
        padFraction: 0.12,
        minPad: 0.2,
        interval_ms: tempData.interval_ms,
        timeMode: USE_DATE_API ? 'absolute' : null,
        timestamps: timestamps,
        expectedDurationMs: USE_DATE_API ? 86400000 : (TIMEFRAME * 1000)
      }, $('tempNow'), $('tempMin'), $('tempMax'), ' °C');
    }

    // Humidity
    const humEndpoint = USE_DATE_API ? '/api/humidity-date?date=' + getTodayStr() : '/api/humidity';
    const humRes = await fetch(humEndpoint, {cache:'no-store'});
    const humData = await humRes.json();
    if (humData.ok) {
      let series, timestamps;
      if (humData.data) {
        timestamps = humData.data.map(d => d[0]);
        series = humData.data.map(d => d[1]);
      } else {
        series = sliceSeries(humData.series || [], humData.interval_ms);
      }
      renderGraph(charts.hum, series, {
        fixedMin: 0,
        fixedMax: 100,
        padFraction: 0,
        minPad: 0,
        interval_ms: humData.interval_ms,
        timeMode: USE_DATE_API ? 'absolute' : null,
        timestamps: timestamps,
        expectedDurationMs: USE_DATE_API ? 86400000 : (TIMEFRAME * 1000)
      }, $('humNow'), $('humMin'), $('humMax'), ' %');
    }

    // Pressure
    const pressEndpoint = USE_DATE_API ? '/api/pressure-date?date=' + getTodayStr() : '/api/pressure';
    const pressRes = await fetch(pressEndpoint, {cache:'no-store'});
    const pressData = await pressRes.json();
    if (pressData.ok) {
      let series, timestamps;
      if (pressData.data) {
        timestamps = pressData.data.map(d => d[0]);
        series = pressData.data.map(d => d[1]);
      } else {
        series = sliceSeries(pressData.station_series || [], pressData.interval_ms);
      }
      renderGraph(charts.press, series, {
        padFraction: 0.15,
        minPad: 0.2,
        interval_ms: pressData.interval_ms,
        timeMode: USE_DATE_API ? 'absolute' : null,
        timestamps: timestamps,
        expectedDurationMs: USE_DATE_API ? 86400000 : (TIMEFRAME * 1000)
      }, $('pressNow'), $('pressMin'), $('pressMax'), ' hPa');
    }

    // Gas
    const gasEndpoint = USE_DATE_API ? '/api/gas-date?date=' + getTodayStr() : '/api/gas';
    const gasRes = await fetch(gasEndpoint, {cache:'no-store'});
    const gasData = await gasRes.json();
    if (gasData.ok) {
      let series, timestamps;
      if (gasData.data) {
        timestamps = gasData.data.map(d => d[0]);
        series = gasData.data.map(d => d[1]);
      } else {
        series = sliceSeries(gasData.series || [], gasData.interval_ms);
      }
      renderGraph(charts.gas, series, {
        padFraction: 0.15,
        minPad: 0.2,
        interval_ms: gasData.interval_ms,
        timeMode: USE_DATE_API ? 'absolute' : null,
        timestamps: timestamps,
        expectedDurationMs: USE_DATE_API ? 86400000 : (TIMEFRAME * 1000)
      }, $('gasNow'), $('gasMin'), $('gasMax'), ' kΩ');
    }
  } catch(e) {
    console.error('Fetch error:', e);
  }
}

tick();
setInterval(tick, 2000);
</script>
</body></html>
)HTML");
}

// Individual timeframe page handlers
static void sendPageGraphs1m(WiFiClient &client) {
  sendPageGraphs(client, "1m", 60, "1 minute");
}

static void sendPageGraphs5m(WiFiClient &client) {
  sendPageGraphs(client, "5m", 300, "5 minute");
}

static void sendPageGraphs10m(WiFiClient &client) {
  sendPageGraphs(client, "10m", 600, "10 minute");
}

static void sendPageGraphs24h(WiFiClient &client) {
  sendPageGraphs(client, "24h", 86400, "24 hour");
}
