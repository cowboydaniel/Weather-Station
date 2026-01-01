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
      <div class="sub">10-minute live buffer, or select 24hr for historical data from SD card</div>
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
        <option value="86400">24 hours</option>
      </select>
      <button id="datePickerBtn" style="display: none; margin-left: 12px; padding: 6px 12px; background: rgba(255,255,255,0.1); border: 1px solid rgba(255,255,255,0.2); border-radius: 4px; color: rgba(255,255,255,0.8); font-size: 14px; cursor: pointer;">Today (Live)</button>
    </div>
    <div style="margin-top:12px">
      <canvas id="cv" class="chart chart-lg" width="1000" height="360"></canvas>
    </div>
    <div style="margin-top: 12px; padding-top: 12px; border-top: 1px solid rgba(255,255,255,0.1); display: flex; gap: 24px;">
      <div><span style="font-size: 12px; color: rgba(255,255,255,0.6);">Average</span><div style="font-size: 16px; font-weight: 500;"><b id="avgv">--</b> %</div></div>
      <div><span style="font-size: 12px; color: rgba(255,255,255,0.6);">Min</span><div style="font-size: 16px; font-weight: 500;"><b id="minv">--</b> %</div></div>
      <div><span style="font-size: 12px; color: rgba(255,255,255,0.6);">Max</span><div style="font-size: 16px; font-weight: 500;"><b id="maxv">--</b> %</div></div>
    </div>
  </div>
</div>

<script>
const METRIC = 'humidity';
const METRIC_KEY = 'humidity';
let pageState = {
  baseEndpoint: '/api/humidity',
  selectedDate: '',
  selectedTimeframe: 600,
  availableDates: [],
  page: null,
  calendarMonth: new Date().getMonth(),
  calendarYear: new Date().getFullYear()
};

function getChartEndpoint(baseEndpoint, selectedDate, timeframe) {
  if (selectedDate) {
    return baseEndpoint + '-date?date=' + encodeURIComponent(selectedDate);
  }
  // Always use base endpoint for real-time (never use -hourly which has insufficient data)
  return baseEndpoint;
}

function renderCalendarDays() {
  const year = pageState.calendarYear;
  const month = pageState.calendarMonth;
  const today = new Date();

  // Get first day of month (0=Sunday, 1=Monday, ... 6=Saturday)
  // Convert to Monday-first (0=Monday, ... 6=Sunday)
  const firstDayOfMonth = new Date(year, month, 1).getDay();
  const mondayFirstDay = (firstDayOfMonth === 0) ? 6 : firstDayOfMonth - 1;

  const lastDay = new Date(year, month + 1, 0).getDate();
  const prevMonthLastDay = new Date(year, month, 0).getDate();

  const monthNames = ['January', 'February', 'March', 'April', 'May', 'June', 'July', 'August', 'September', 'October', 'November', 'December'];
  $('monthYear').textContent = monthNames[month] + ' ' + year;

  const calendarDaysEl = $('calendarDays');
  calendarDaysEl.innerHTML = '';

  // Previous month's days (to fill from Monday)
  for (let i = mondayFirstDay - 1; i >= 0; i--) {
    const day = prevMonthLastDay - i;
    const el = document.createElement('div');
    el.className = 'calendar-day other-month';
    el.textContent = day;
    calendarDaysEl.appendChild(el);
  }

  // Current month's days
  const today_str = today.getFullYear() + '-' + String(today.getMonth() + 1).padStart(2, '0') + '-' + String(today.getDate()).padStart(2, '0');

  for (let day = 1; day <= lastDay; day++) {
    const dateStr = year + '-' + String(month + 1).padStart(2, '0') + '-' + String(day).padStart(2, '0');
    const hasData = pageState.availableDates.includes(dateStr) || dateStr === today_str;
    const isToday = dateStr === today_str;

    const el = document.createElement('div');
    el.className = 'calendar-day';
    if (isToday) el.classList.add('today');
    if (!hasData) el.classList.add('no-data');
    el.textContent = day;
    el.dataset.date = dateStr;

    if (hasData) {
      el.addEventListener('click', (e) => {
        pageState.selectedDate = dateStr;
        $('datePickerBtn').textContent = dateStr;
        closeCalendarModal();
        recreatePage();
      });
      el.style.cursor = 'pointer';
    }

    calendarDaysEl.appendChild(el);
  }

  // Next month's days (to fill to complete weeks)
  const totalCells = calendarDaysEl.children.length;
  const remainingCells = (7 - (totalCells % 7)) % 7;
  for (let day = 1; day <= remainingCells; day++) {
    const el = document.createElement('div');
    el.className = 'calendar-day other-month';
    el.textContent = day;
    calendarDaysEl.appendChild(el);
  }
}

function openCalendarModal() {
  if (pageState.selectedTimeframe !== 86400) {
    return;
  }
  $('calendarModal').style.display = 'flex';
  renderCalendarDays();
}

function closeCalendarModal() {
  $('calendarModal').style.display = 'none';
}

function loadAvailableDates() {
  fetch('/api/available-dates?metric=' + METRIC, {cache:'no-store'})
    .then(r => r.json())
    .then(data => {
      if (data.ok && Array.isArray(data.dates)) {
        pageState.availableDates = data.dates;
      }
    })
    .catch(e => console.log('Could not load dates:', e));
}

function recreatePage() {
  if (pageState.page && pageState.page.intervalId) {
    clearInterval(pageState.page.intervalId);
  }
  // For 24hr timeframe, pass expectedDurationMs so the graph shows full day axis
  const expectedDurationMs = pageState.selectedTimeframe >= 86400 ? 86400000 : null;
  const page = startSimpleSeriesPage({
    endpoint: getChartEndpoint(pageState.baseEndpoint, pageState.selectedDate, pageState.selectedTimeframe),
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
    avgId:'avgv',
    minId:'minv',
    maxId:'maxv',
    expectedDurationMs: expectedDurationMs
  });

  // Wrap render to slice data for timeframes < 10 minutes
  const originalRender = page.render;
  page.render = function(series, interval_ms, timeMode, timestamps, expectedDurationMs) {
    if (pageState.selectedTimeframe < 600 && !pageState.selectedDate) {
      // Slice to the requested timeframe
      const pointsToShow = Math.ceil(pageState.selectedTimeframe);
      series = series.slice(-pointsToShow);
      if (Array.isArray(timestamps)) {
        timestamps = timestamps.slice(-pointsToShow);
      }
    }
    return originalRender.call(this, series, interval_ms, timeMode, timestamps, expectedDurationMs);
  };

  pageState.page = page;
}

// Modal event listeners
const calendarModal = $('calendarModal');
const prevMonthBtn = $('prevMonth');
const nextMonthBtn = $('nextMonth');
const closeBtn = $('calendarCloseBtn');

if (prevMonthBtn) {
  prevMonthBtn.addEventListener('click', () => {
    pageState.calendarMonth--;
    if (pageState.calendarMonth < 0) {
      pageState.calendarMonth = 11;
      pageState.calendarYear--;
    }
    renderCalendarDays();
  });
}

if (nextMonthBtn) {
  nextMonthBtn.addEventListener('click', () => {
    pageState.calendarMonth++;
    if (pageState.calendarMonth > 11) {
      pageState.calendarMonth = 0;
      pageState.calendarYear++;
    }
    renderCalendarDays();
  });
}

if (closeBtn) {
  closeBtn.addEventListener('click', closeCalendarModal);
}

if (calendarModal) {
  calendarModal.querySelector('.calendar-modal-overlay').addEventListener('click', closeCalendarModal);
}

// Escape key to close modal
document.addEventListener('keydown', (e) => {
  if (e.key === 'Escape') {
    closeCalendarModal();
  }
});

// Timeframe select listener
const timeframeSelect = $('timeframeSelect');
if (timeframeSelect) {
  const saved = localStorage.getItem('humidityGraphSpan');
  if (saved) {
    timeframeSelect.value = saved;
    pageState.selectedTimeframe = parseInt(saved, 10);
  }

  timeframeSelect.addEventListener('change', (e) => {
    pageState.selectedTimeframe = parseInt(e.target.value, 10);
    localStorage.setItem('humidityGraphSpan', pageState.selectedTimeframe);

    const datePickerBtn = $('datePickerBtn');
    const today = new Date();
    const todayStr = today.getFullYear() + '-' + String(today.getMonth() + 1).padStart(2, '0') + '-' + String(today.getDate()).padStart(2, '0');

    if (pageState.selectedTimeframe >= 21600) {  // 6hr or longer: 6hr=21600, 12hr=43200, 24hr=86400
      // Auto-load today's data from SD card
      pageState.selectedDate = todayStr;
      datePickerBtn.style.display = 'inline-block';
      datePickerBtn.textContent = todayStr;
      if (pageState.selectedTimeframe === 86400) {
        // Only show calendar for 24hr mode
        // Don't auto-open, but allow manual date picking
      }
      closeCalendarModal();
    } else {
      // Live data from RAM buffer (1m, 5m, 10m)
      pageState.selectedDate = '';
      datePickerBtn.style.display = 'none';
      closeCalendarModal();
    }
    recreatePage();
  });
}

// Date picker button listener
const datePickerBtn = $('datePickerBtn');
if (datePickerBtn) {
  datePickerBtn.addEventListener('click', openCalendarModal);
}

// Initialize page using recreatePage to ensure consistent config
recreatePage();

loadAvailableDates();
</script>
</body></html>
)HTML");
}
