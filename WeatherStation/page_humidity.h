#pragma once
#include <WiFiS3.h>

static void sendPageHumidity(WiFiClient &client) {
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
<title>Humidity</title>
<link rel="icon" href="/static/favicon.svg">
<link rel="stylesheet" href="/static/app.css">
<script src="/static/app.js"></script>
</head>
<body class="page-chart page-humidity" style="--bg-layer: radial-gradient(1200px 700px at 20% 10%, rgba(120,255,200,0.18), transparent 60%), var(--bg); --wrap-max:1000px; --wrap-pad:22px 14px 30px;">
<div class="wrap">
  <div class="top">
    <div>
      <h1>Humidity</h1>
      <div class="sub">10-minute history, 1 sample per second</div>
    </div>
    <a class="kpi" href="/">Back</a>
  </div>

  <div class="card">
    <div class="kpis">
      <div class="kpi">Now: <b id="now">--</b></div>
      <div class="kpi">Min: <b id="minv">--</b></div>
      <div class="kpi">Max: <b id="maxv">--</b></div>
    </div>
    <div style="margin-top:12px">
      <canvas id="cv" class="chart chart-lg" width="1000" height="360"></canvas>
    </div>
  </div>
</div>

<script>
function getChartEndpoint(baseEndpoint) {
  const saved = localStorage.getItem('weatherSettings');
  let graphSpan = 600; // default
  if (saved) {
    try {
      const settings = JSON.parse(saved);
      graphSpan = settings.graphSpan || 600;
    } catch (e) {}
  }

  // For longer periods (6hr+), use hourly data
  if (graphSpan > 600) {
    return baseEndpoint + '-hourly';
  }
  return baseEndpoint;
}

startSimpleSeriesPage({
  endpoint: getChartEndpoint('/api/humidity'),
  canvasId:'cv',
  unit:' %',
  decimals:1,
  pollMs:2000,
  height:360,
  fixedMin:0,
  fixedMax:100,
  padFraction:0,
  minPad:0,
  nowId:'now',
  minId:'minv',
  maxId:'maxv'
});
</script>
</body></html>
)HTML");
}
