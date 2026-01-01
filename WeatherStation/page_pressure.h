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
      <div class="sub">Station pressure (24hr or less, 1 Hz) and sea-level pressure trend (60 min, 1/min)</div>
    </div>
    <a class="kpi" href="/">Back</a>
  </div>

  <div class="card">
    <div class="kpis">
      <div class="kpi">SLP now: <b id="slp">--</b></div>
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
    <div class="sep"></div>
    <div class="sub">Station pressure (hPa)</div>
    <canvas id="cv1" class="chart chart-md" width="1000" height="260"></canvas>
    <div style="margin-top: 12px; padding-top: 12px; border-top: 1px solid rgba(255,255,255,0.1); display: flex; gap: 24px;">
      <div><span style="font-size: 12px; color: rgba(255,255,255,0.6);">Average</span><div style="font-size: 16px; font-weight: 500;"><b id="avgv">--</b> hPa</div></div>
      <div><span style="font-size: 12px; color: rgba(255,255,255,0.6);">Min</span><div style="font-size: 16px; font-weight: 500;"><b id="minv">--</b> hPa</div></div>
      <div><span style="font-size: 12px; color: rgba(255,255,255,0.6);">Max</span><div style="font-size: 16px; font-weight: 500;"><b id="maxv">--</b> hPa</div></div>
    </div>

    <div class="sep"></div>
    <div class="sub">Sea-level pressure trend (hPa)</div>
    <canvas id="cv2" class="chart chart-md" width="1000" height="260"></canvas>
  </div>
</div>

<script>
const METRIC = 'pressure';
const station = setupCanvas('cv1', 260);
const slpTrendChart = setupCanvas('cv2', 260);
let pageState = {
  selectedDate: '',
  selectedTimeframe: 600,
  availableDates: [],
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
        tick();
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
  const saved = localStorage.getItem('pressureGraphSpan');
  if (saved) {
    timeframeSelect.value = saved;
    pageState.selectedTimeframe = parseInt(saved, 10);
  }

  timeframeSelect.addEventListener('change', (e) => {
    pageState.selectedTimeframe = parseInt(e.target.value, 10);
    localStorage.setItem('pressureGraphSpan', pageState.selectedTimeframe);

    const datePickerBtn = $('datePickerBtn');
    if (pageState.selectedTimeframe === 86400) {
      datePickerBtn.style.display = 'inline-block';
      openCalendarModal();
    } else {
      datePickerBtn.style.display = 'none';
      pageState.selectedDate = '';
      closeCalendarModal();
    }
    tick();
  });
}

// Date picker button listener
const datePickerBtn = $('datePickerBtn');
if (datePickerBtn) {
  datePickerBtn.addEventListener('click', openCalendarModal);
}

async function tick(){
  try{
    const endpoint = getChartEndpoint('/api/pressure', pageState.selectedDate, pageState.selectedTimeframe);
    const r = await fetch(endpoint, {cache:'no-store'});
    const j = await r.json();
    if(!j.ok) return;

    slp.textContent = j.slp_now.toFixed(1) + " hPa";
    tend.textContent = j.tendency;
    slope.textContent = j.tendency_hpa_hr.toFixed(2) + " hPa/hr";

    // Slice data for timeframes < 10 minutes (real-time only)
    let stationSeries = j.station_series || [];
    let slpTrendSeries = j.slp_trend_series || [];
    if (pageState.selectedTimeframe < 600 && !pageState.selectedDate) {
      const pointsToShow = Math.ceil(pageState.selectedTimeframe);
      stationSeries = stationSeries.slice(-pointsToShow);
      slpTrendSeries = slpTrendSeries.slice(-pointsToShow);
    }

    drawLineSeries(station.ctx, station.cv, stationSeries, {padFraction:0.15, minPad:0.2, interval_ms: j.interval_ms});
    drawLineSeries(slpTrendChart.ctx, slpTrendChart.cv, slpTrendSeries, {padFraction:0.15, minPad:0.2, interval_ms: j.slp_trend_interval_ms});
  }catch(e){}
}
let pressureIntervalId;
tick();
pressureIntervalId = setInterval(tick, 2000);
loadAvailableDates();
</script>
</body></html>
)HTML");
}
