#pragma once
#include <WiFiS3.h>

static void sendPagePressure(WiFiClient &client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html; charset=utf-8");
  client.println("Cache-Control: public, max-age=300");
  client.println("Connection: close");
  client.println();

  client.println(R"HTML(
<!doctype html>
<html><head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Pressure</title>
<link rel="icon" href="/static/favicon.svg">
<link rel="stylesheet" href="/static/app.css">
<script src="/static/app.js"></script>
</head>
<body class="page-chart page-pressure" style="--bg-layer: radial-gradient(1200px 700px at 80% 10%, rgba(255,110,180,0.18), transparent 60%), var(--bg); --wrap-max:1000px; --wrap-pad:22px 14px 30px;">
<div class="wrap">
  <div class="top">
    <div>
      <h1>Pressure</h1>
      <div class="sub">Station pressure (10 min, 1 Hz) and sea-level trend (60 min, 1/min)</div>
    </div>
    <a class="kpi" href="/">Back</a>
  </div>

  <div class="card">
    <div class="kpis">
      <div class="kpi">SLP now: <b id="slp">--</b></div>
      <div class="kpi">Tendency: <b id="tend">--</b></div>
      <div class="kpi">Slope: <b id="slope">--</b></div>
    </div>

    <div class="sep"></div>
    <div class="sub">Station pressure (hPa)</div>
    <canvas id="cv1" class="chart chart-md" width="1000" height="260"></canvas>

    <div class="sep"></div>
    <div class="sub">Sea-level pressure trend (hPa)</div>
    <canvas id="cv2" class="chart chart-md" width="1000" height="260"></canvas>
  </div>
</div>

<script>
const station = setupCanvas('cv1', 260);
const slpTrendChart = setupCanvas('cv2', 260);
const settings = loadUserSettings();
const spanSec = Math.max(60, settings.graphSpan || 600);
const showGrid = settings.showGrid !== false;

async function tick(){
  try{
    const r = await fetch('/api/pressure', {cache:'no-store'});
    const j = await r.json();
    if(!j.ok) return;

    slp.textContent = j.slp_now.toFixed(1) + " hPa";
    tend.textContent = j.tendency;
    slope.textContent = j.tendency_hpa_hr.toFixed(2) + " hPa/hr";

    const pickedStation = pickSeriesBySpan({
      station_series: j.station_series,
      station_series_min: j.station_series_min,
      interval_ms: j.interval_ms,
      station_min_interval_ms: j.station_min_interval_ms
    }, spanSec);

    drawLineSeries(station.ctx, station.cv, pickedStation.series || [], {padFraction:0.15, minPad:0.2, showGrid});
    drawLineSeries(slpTrendChart.ctx, slpTrendChart.cv, j.slp_trend_series || [], {padFraction:0.15, minPad:0.2, showGrid});
  }catch(e){}
}
tick(); setInterval(tick, 2000);  // 2s polling - data updates at 1Hz anyway
</script>
</body></html>
)HTML");
}
