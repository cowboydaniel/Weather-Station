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
    </div>

    <!-- Calendar Modal Overlay -->
    <div id="calendarModal" class="calendar-modal" style="display: none;">
      <div class="calendar-modal-overlay"></div>
      <div class="calendar-modal-content">
        <div class="calendar-modal-header">
          <h3>Select Date</h3>
          <button id="calendarCloseBtn" class="calendar-modal-close">&times;</button>
        </div>
        <div class="calendar-widget-container">
          <div class="calendar-nav-row">
            <button class="calendar-nav-btn" id="prevMonth" aria-label="Previous month">‹</button>
            <div class="calendar-month-year" id="monthYear">January 2026</div>
            <button class="calendar-nav-btn" id="nextMonth" aria-label="Next month">›</button>
          </div>
          <div class="calendar-weekdays">
            <div class="calendar-weekday-label">Sun</div>
            <div class="calendar-weekday-label">Mon</div>
            <div class="calendar-weekday-label">Tue</div>
            <div class="calendar-weekday-label">Wed</div>
            <div class="calendar-weekday-label">Thu</div>
            <div class="calendar-weekday-label">Fri</div>
            <div class="calendar-weekday-label">Sat</div>
          </div>
          <div class="calendar-days" id="calendarDays"></div>
        </div>
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
const METRIC = 'temp';
const METRIC_KEY = 'temp';
let pageState = {
  baseEndpoint: '/api/temp',
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
  if (timeframe > 600) {
    return baseEndpoint + '-hourly';
  }
  return baseEndpoint;
}

function renderCalendarDays() {
  const year = pageState.calendarYear;
  const month = pageState.calendarMonth;
  const today = new Date();

  const firstDay = new Date(year, month, 1).getDay();
  const lastDay = new Date(year, month + 1, 0).getDate();
  const prevMonthLastDay = new Date(year, month, 0).getDate();

  const monthNames = ['January', 'February', 'March', 'April', 'May', 'June', 'July', 'August', 'September', 'October', 'November', 'December'];
  $('monthYear').textContent = monthNames[month] + ' ' + year;

  const calendarDaysEl = $('calendarDays');
  calendarDaysEl.innerHTML = '';

  // Previous month's days
  for (let i = firstDay - 1; i >= 0; i--) {
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

  // Next month's days
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

// Timeframe select listener
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
    if (pageState.selectedTimeframe !== 86400) {
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

loadAvailableDates();
</script>
</body></html>
)HTML");
}
