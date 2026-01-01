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
      <div style="flex: 1;"></div>
      <div class="kpi">
        Date:
        <select id="dateSelect" style="padding: 4px 8px; border-radius: 4px; border: 1px solid rgba(255,255,255,0.2); background: rgba(255,255,255,0.05); color: white; font-size: 14px;">
          <option value="">Today (Live)</option>
        </select>
      </div>
    </div>
    <div style="margin-top:12px">
      <canvas id="cv" class="chart chart-lg" width="1000" height="360"></canvas>
    </div>
    <div style="margin-top: 12px; padding-top: 12px; border-top: 1px solid rgba(255,255,255,0.1); display: flex; gap: 24px;">
      <div><span style="font-size: 12px; color: rgba(255,255,255,0.6);">Average</span><div style="font-size: 16px; font-weight: 500;"><b id="avgv">--</b> °C</div></div>
      <div><span style="font-size: 12px; color: rgba(255,255,255,0.6);">Min</span><div style="font-size: 16px; font-weight: 500;"><b id="minv">--</b> °C</div></div>
      <div><span style="font-size: 12px; color: rgba(255,255,255,0.6);">Max</span><div style="font-size: 16px; font-weight: 500;"><b id="maxv">--</b> °C</div></div>
    </div>
  </div>
</div>

<script>
let pageState = {
  baseEndpoint: '/api/temp',
  selectedDate: '',
  page: null
};

function getChartEndpoint(baseEndpoint, selectedDate) {
  if (selectedDate) {
    // Use per-day endpoint with the new date-specific API
    // e.g., /api/temp-date?date=2024-01-15
    return baseEndpoint + '-date?date=' + encodeURIComponent(selectedDate);
  }

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

function loadAvailableDates() {
  fetch('/api/available-dates?metric=temp', {cache:'no-store'})
    .then(r => r.json())
    .then(data => {
      if (data.ok && Array.isArray(data.dates)) {
        const select = $('dateSelect');
        if (!select) return;

        data.dates.forEach(date => {
          const option = document.createElement('option');
          option.value = date;
          option.textContent = date;
          select.appendChild(option);
        });
      }
    })
    .catch(e => console.log('Could not load dates:', e));
}

const dateSelect = $('dateSelect');
if (dateSelect) {
  dateSelect.addEventListener('change', (e) => {
    pageState.selectedDate = e.target.value;
    const newEndpoint = getChartEndpoint(pageState.baseEndpoint, pageState.selectedDate);
    if (pageState.page && pageState.page.tick) {
      pageState.page.tick();
    }
  });
}

pageState.page = startSimpleSeriesPage({
  endpoint: getChartEndpoint(pageState.baseEndpoint, pageState.selectedDate),
  canvasId:'cv',
  unit:' °C',
  decimals:1,
  pollMs:2000,
  height:360,
  padFraction:0.12,
  minPad:0.2,
  nowId:'now',
  avgId:'avgv',
  minId:'minv',
  maxId:'maxv'
});

loadAvailableDates();
</script>
</body></html>
)HTML");
}
