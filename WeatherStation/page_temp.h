#pragma once
#include <WiFiS3.h>

static void sendPageTemp(WiFiClient &client) {
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
<title>Temperature</title>
<link rel="icon" href="/static/favicon.svg">
<link rel="stylesheet" href="/static/app.css">
<script src="/static/app.js"></script>
</head>
<body class="page-chart page-temp" style="--bg-layer: radial-gradient(1200px 700px at 15% 10%, rgba(110,160,255,0.22), transparent 60%), var(--bg); --wrap-max:1000px; --wrap-pad:22px 14px 30px;">
<div class="wrap">
  <div class="top">
    <div>
      <h1>Temperature</h1>
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
startSimpleSeriesPage({
  endpoint:'/api/temp',
  canvasId:'cv',
  unit:' °C',
  decimals:1,
  pollMs:2000,
  height:360,
  padFraction:0.12,
  minPad:0.2,
  nowId:'now',
  minId:'minv',
  maxId:'maxv'
});
</script>
</body></html>
)HTML");
}
