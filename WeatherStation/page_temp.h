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
    </div>
    <div class="timeframe-selector">
      <span class="timeframe-label">Time span:</span>
      <select id="timeframeSelect">
        <option value="60">1 minute</option>
        <option value="300">5 minutes</option>
        <option value="600" selected>10 minutes</option>
        <option value="21600">6 hours</option>
        <option value="43200">12 hours</option>
        <option value="86400">24 hours</option>
      </select>
      <span class="timeframe-label" style="margin-left: 16px;">Date:</span>
      <button id="datePickerBtn" style="padding: 6px 10px; background: rgba(255,255,255,0.1); border: 1px solid rgba(255,255,255,0.15); border-radius: 6px; color: white; cursor: pointer; font-size: 13px;">Today (Live)</button>
      <div id="calendarContainer" class="calendar-container"></div>
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
const METRIC = 'temp';
const METRIC_KEY = 'temp';
let pageState = {
  baseEndpoint: '/api/temp',
  selectedDate: '',
  selectedTimeframe: 600,
  availableDates: [],
  page: null
};

function getChartEndpoint(baseEndpoint, selectedDate, timeframe) {
  if (selectedDate) {
    return baseEndpoint + '-date?date=' + encodeURIComponent(selectedDate);
  }
  if (timeframe > 600) {
    return baseEndpoint + '-hourly';
  }
  return baseEndpoint;
}

function renderCalendar() {
  const container = $('calendarContainer');
  if (!container) return;

  const today = new Date();
  const year = today.getFullYear();
  const month = today.getMonth();

  const firstDay = new Date(year, month, 1);
  const lastDay = new Date(year, month + 1, 0);
  const daysInMonth = lastDay.getDate();
  const startingDayOfWeek = firstDay.getDay();

  let html = '<div class="calendar-header">';
  html += '<div>' + firstDay.toLocaleDateString('en-US', {month: 'short', year: 'numeric'}) + '</div>';
  html += '</div>';
  html += '<div class="calendar-dates">';

  for (let i = 0; i < startingDayOfWeek; i++) {
    html += '<div class="calendar-date disabled"></div>';
  }

  const today_str = today.getFullYear() + '-' + String(today.getMonth() + 1).padStart(2, '0') + '-' + String(today.getDate()).padStart(2, '0');

  for (let day = 1; day <= daysInMonth; day++) {
    const dateStr = year + '-' + String(month + 1).padStart(2, '0') + '-' + String(day).padStart(2, '0');
    const hasData = pageState.availableDates.includes(dateStr) || dateStr === today_str;
    const isActive = dateStr === pageState.selectedDate || (pageState.selectedDate === '' && dateStr === today_str);

    html += '<div class="calendar-date' + (hasData ? ' has-data' : '') + (isActive ? ' active' : '') + '" data-date="' + dateStr + '">' + day + '</div>';
  }

  html += '</div>';
  container.innerHTML = html;

  container.querySelectorAll('[data-date]').forEach(el => {
    el.addEventListener('click', (e) => {
      pageState.selectedDate = e.target.dataset.date;
      renderCalendar();
      $('datePickerBtn').textContent = pageState.selectedDate;
      if (pageState.page && pageState.page.tick) {
        pageState.page.tick();
      }
    });
  });
}

function toggleCalendar() {
  const container = $('calendarContainer');
  if (container) {
    if (pageState.selectedTimeframe === 86400) {
      container.classList.toggle('visible');
    } else {
      container.classList.remove('visible');
    }
  }
}

function loadAvailableDates() {
  fetch('/api/available-dates?metric=' + METRIC, {cache:'no-store'})
    .then(r => r.json())
    .then(data => {
      if (data.ok && Array.isArray(data.dates)) {
        pageState.availableDates = data.dates;
        if (pageState.selectedTimeframe === 86400) {
          renderCalendar();
        }
      }
    })
    .catch(e => console.log('Could not load dates:', e));
}

const timeframeSelect = $('timeframeSelect');
if (timeframeSelect) {
  const saved = localStorage.getItem('tempGraphSpan');
  if (saved) {
    timeframeSelect.value = saved;
    pageState.selectedTimeframe = parseInt(saved, 10);
  }

  timeframeSelect.addEventListener('change', (e) => {
    pageState.selectedTimeframe = parseInt(e.target.value, 10);
    localStorage.setItem('tempGraphSpan', pageState.selectedTimeframe);
    pageState.selectedDate = '';
    $('datePickerBtn').textContent = 'Today (Live)';
    toggleCalendar();
    if (pageState.page && pageState.page.tick) {
      pageState.page.tick();
    }
  });
}

const datePickerBtn = $('datePickerBtn');
if (datePickerBtn) {
  datePickerBtn.addEventListener('click', toggleCalendar);
}

pageState.page = startSimpleSeriesPage({
  endpoint: getChartEndpoint(pageState.baseEndpoint, pageState.selectedDate, pageState.selectedTimeframe),
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

toggleCalendar();
loadAvailableDates();
</script>
</body></html>
)HTML");
}
