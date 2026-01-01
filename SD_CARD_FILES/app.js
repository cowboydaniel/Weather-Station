const $ = (id) => document.getElementById(id);

function setupCanvas(id, heightPx){
  const cv = $(id);
  const ctx = cv.getContext('2d');
  const targetHeight = heightPx || Number(cv.dataset.height) || 360;
  const resize = () => {
    const r = cv.getBoundingClientRect();
    const dpr = Math.max(1, window.devicePixelRatio || 1);
    cv.width = Math.floor(r.width * dpr);
    cv.height = Math.floor(targetHeight * dpr);
  };
  window.addEventListener('resize', resize);
  resize();
  return {cv, ctx};
}

function drawLineSeries(ctx, cv, series, opts={}){
  if (!Array.isArray(series) || series.length < 2) return null;
  const data = series.filter(v => typeof v === 'number' && isFinite(v));
  if (data.length < 2) return null;

  let dataMin = data[0], dataMax = data[0];
  for (const v of data){ if(v<dataMin) dataMin=v; if(v>dataMax) dataMax=v; }

  let min = (opts.fixedMin !== undefined) ? opts.fixedMin : dataMin;
  let max = (opts.fixedMax !== undefined) ? opts.fixedMax : dataMax;
  if (opts.fixedMin === undefined || opts.fixedMax === undefined){
    const pad = Math.max(opts.minPad ?? 0.2, (dataMax - dataMin) * (opts.padFraction ?? 0.12));
    if (opts.fixedMin === undefined) min -= pad;
    if (opts.fixedMax === undefined) max += pad;
  }
  let rng = max - min;
  if (rng < 1e-6) rng = 1;

  const w = cv.width, h = cv.height;
  ctx.clearRect(0,0,w,h);

  ctx.strokeStyle = 'rgba(255,255,255,0.06)';
  ctx.lineWidth = 1;
  for(let i=1;i<6;i++){
    const y = (h*i)/6;
    ctx.beginPath(); ctx.moveTo(0,y); ctx.lineTo(w,y); ctx.stroke();
  }

  // Draw X-axis ticks and labels based on interval
  drawXAxisTicks(ctx, w, h, series.length, opts.interval_ms, opts.timeMode, opts.timestamps);

  ctx.lineWidth = 3;
  ctx.strokeStyle = 'rgba(255,255,255,0.88)';
  ctx.beginPath();
  for(let i=0;i<series.length;i++){
    const v = (typeof series[i] === 'number' && isFinite(series[i])) ? series[i] : null;
    const x = (i/(series.length-1))*w;
    const y = v===null ? h/2 : (h - ((v - min)/rng)*h);
    if(i===0) ctx.moveTo(x,y); else ctx.lineTo(x,y);
  }
  ctx.stroke();

  ctx.globalAlpha = 0.10;
  ctx.fillStyle = 'rgba(255,255,255,1)';
  ctx.lineTo(w,h); ctx.lineTo(0,h); ctx.closePath();
  ctx.fill();
  ctx.globalAlpha = 1;

  return {min: dataMin, max: dataMax, last: data[data.length-1], data: series, min: min, max: max, range: rng};
}

function drawXAxisTicks(ctx, w, h, numPoints, interval_ms, timeMode, timestamps){
  if (!interval_ms || numPoints < 2) return;

  const HOURLY_MS = 3600000;
  let tickCount, tickLabel;

  if (timeMode === 'absolute' && Array.isArray(timestamps) && timestamps.length === numPoints) {
    // Absolute time mode for 24-hour data with real timestamps
    tickCount = Math.min(5, numPoints - 1);
    const firstTime = timestamps[0];
    const lastTime = timestamps[numPoints - 1];

    tickLabel = (idx) => {
      const timeIdx = Math.round((idx / tickCount) * (numPoints - 1));
      const timestamp = timestamps[timeIdx];
      if (!timestamp) return '';
      const date = new Date(timestamp);
      return date.getHours().toString().padStart(2, '0') + ':' + date.getMinutes().toString().padStart(2, '0');
    };
  } else if (interval_ms >= HOURLY_MS) {
    // Relative time for hourly data
    tickCount = Math.min(numPoints - 1, 6);
    tickLabel = (idx) => '-' + (tickCount - idx) + 'h';
  } else if (interval_ms >= 60000) {
    // Relative time for minute-scale data
    const minutes = Math.round(numPoints * interval_ms / 60000);
    tickCount = Math.min(Math.ceil(minutes / 5), 6);
    tickLabel = (idx) => {
      const totalMinutes = Math.round(numPoints * interval_ms / 60000);
      const minPerTick = Math.max(1, Math.round(totalMinutes / tickCount));
      return '-' + (tickCount - idx) * minPerTick + 'm';
    };
  } else {
    // Relative time for second-scale data
    const seconds = Math.round(numPoints * interval_ms / 1000);
    tickCount = Math.min(Math.ceil(seconds / 30), 6);
    tickLabel = (idx) => {
      const totalSeconds = Math.round(numPoints * interval_ms / 1000);
      const secPerTick = Math.max(1, Math.round(totalSeconds / tickCount));
      const secVal = (tickCount - idx) * secPerTick;
      return '-' + secVal + 's';
    };
  }

  ctx.save();
  ctx.strokeStyle = 'rgba(255,255,255,0.15)';
  ctx.lineWidth = 1;
  ctx.fillStyle = 'rgba(255,255,255,0.5)';
  ctx.font = '11px sans-serif';
  ctx.textAlign = 'center';
  ctx.textBaseline = 'top';

  for (let i = 0; i <= tickCount; i++) {
    const x = (i / tickCount) * w;
    // Draw tick mark
    ctx.beginPath();
    ctx.moveTo(x, h - 14);
    ctx.lineTo(x, h - 8);
    ctx.stroke();
    // Draw label
    ctx.fillText(tickLabel(i), x, h - 6);
  }

  ctx.restore();
}

function startSimpleSeriesPage(cfg){
  const nowEl = cfg.nowId ? $(cfg.nowId) : null;
  const minEl = cfg.minId ? $(cfg.minId) : null;
  const maxEl = cfg.maxId ? $(cfg.maxId) : null;
  const decimals = cfg.decimals ?? 1;
  const unit = cfg.unit || '';
  const chart = setupCanvas(cfg.canvasId, cfg.height || 360);
  let lastInterval = null;
  let lastSeries = null;
  let lastStats = null;
  let lastTimeMode = null;
  let lastTimestamps = null;

  // Create tooltip element
  const tooltip = document.createElement('div');
  tooltip.className = 'hover-tooltip';
  document.body.appendChild(tooltip);

  const render = (series, interval_ms, timeMode, timestamps) => {
    lastInterval = interval_ms;
    lastSeries = series;
    lastTimeMode = timeMode;
    lastTimestamps = timestamps;
    const stats = drawLineSeries(chart.ctx, chart.cv, series, {
      fixedMin: cfg.fixedMin,
      fixedMax: cfg.fixedMax,
      padFraction: cfg.padFraction,
      minPad: cfg.minPad,
      interval_ms: interval_ms,
      timeMode: timeMode,
      timestamps: timestamps
    });
    lastStats = stats;
    if (!stats) return;
    if (nowEl) nowEl.textContent = stats.last.toFixed(decimals) + unit;
    if (minEl) minEl.textContent = stats.min.toFixed(decimals) + unit;
    if (maxEl) maxEl.textContent = stats.max.toFixed(decimals) + unit;
  };

  // Mouse hover handler
  chart.cv.addEventListener('mousemove', (e) => {
    if (!lastStats || !lastSeries || !lastStats.data) {
      tooltip.classList.remove('visible');
      return;
    }

    const rect = chart.cv.getBoundingClientRect();
    const relX = e.clientX - rect.left;
    const w = chart.cv.width;
    const dpr = window.devicePixelRatio || 1;
    const relXScaled = relX * dpr;

    const idx = Math.round((relXScaled / w) * (lastSeries.length - 1));
    const clampedIdx = Math.max(0, Math.min(lastSeries.length - 1, idx));
    const value = lastSeries[clampedIdx];

    if (typeof value !== 'number' || !isFinite(value)) {
      tooltip.classList.remove('visible');
      return;
    }

    // Build tooltip text
    let tooltipText = value.toFixed(decimals) + unit;
    if (lastTimeMode === 'absolute' && Array.isArray(lastTimestamps) && lastTimestamps[clampedIdx]) {
      const date = new Date(lastTimestamps[clampedIdx]);
      tooltipText += ' at ' + date.getHours().toString().padStart(2, '0') + ':' + date.getMinutes().toString().padStart(2, '0');
    }

    tooltip.textContent = tooltipText;
    tooltip.classList.add('visible');
    tooltip.style.left = e.clientX + 'px';
    tooltip.style.top = (e.clientY - 30) + 'px';
  });

  chart.cv.addEventListener('mouseleave', () => {
    tooltip.classList.remove('visible');
  });

  const tick = async () => {
    try{
      const r = await fetch(cfg.endpoint, {cache:'no-store'});
      const j = await r.json();
      if(!j.ok) return;
      render(j.series || [], j.interval_ms, j.timeMode, j.timestamps);
    }catch(e){}
  };
  tick();
  const intervalId = setInterval(tick, cfg.pollMs || 2000);
  return {tick, render, chart, intervalId};
}

function sparkline(points, polyId, fillId){
  const pl = $(polyId);
  const pf = $(fillId);
  if (!pl || !pf) return;

  if (!Array.isArray(points) || points.length < 2) {
    pl.setAttribute("points", "");
    pf.setAttribute("d", "");
    return;
  }
  const data = points.filter(v => typeof v === "number" && isFinite(v));
  if (data.length < 2) {
    pl.setAttribute("points", "");
    pf.setAttribute("d", "");
    return;
  }

  let min = data[0], max = data[0];
  for (const v of data) { if (v < min) min = v; if (v > max) max = v; }
  let range = max - min;
  if (range < 1e-6) range = 1.0;

  const W = 100, H = 44;
  const topPad = 6, botPad = 6;
  const usableH = H - topPad - botPad;

  let pts = [];
  for (let i = 0; i < points.length; i++) {
    const v = points[i];
    const x = (i / (points.length - 1)) * W;
    let y;
    if (typeof v !== "number" || !isFinite(v)) {
      y = topPad + usableH / 2;
    } else {
      const n = (v - min) / range;
      y = topPad + (1 - n) * usableH;
    }
    pts.push(x.toFixed(2) + "," + y.toFixed(2));
  }
  pl.setAttribute("points", pts.join(" "));

  const first = pts[0].split(",");
  const last = pts[pts.length - 1].split(",");
  const d =
    "M " + first[0] + " " + (H - botPad) +
    " L " + pts.join(" L ") +
    " L " + last[0] + " " + (H - botPad) +
    " Z";
  pf.setAttribute("d", d);
}
