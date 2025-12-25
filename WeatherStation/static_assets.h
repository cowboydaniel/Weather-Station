#pragma once
#include <WiFiS3.h>

// Shared static assets (CSS/JS/favicon) served from /static/*
static const char APP_CSS[] =
R"CSS(
:root{
  --bg:#070a18;
  --panel: rgba(255,255,255,0.06);
  --panel2: rgba(255,255,255,0.04);
  --border: rgba(255,255,255,0.12);
  --text: rgba(255,255,255,0.92);
  --muted: rgba(255,255,255,0.62);
  --shadow: 0 16px 40px rgba(0,0,0,0.45);
  --radius: 20px;
  --sparkStroke: rgba(255,255,255,0.85);
  --sparkFill: rgba(255,255,255,0.10);
  --accent: #7aa7ff;
  --green: rgba(120,255,180,0.95);
  --yellow: rgba(255,210,120,0.95);
  --red: rgba(255,120,120,0.95);
}

*{ box-sizing:border-box; }
body{
  margin:0;
  font-family: ui-sans-serif, system-ui, -apple-system, Segoe UI, Roboto, Arial;
  color:var(--text);
  background: var(--bg-layer, var(--bg));
  min-height:100vh;
}
body.light{
  --bg:#f5f5f7;
  --panel: rgba(0,0,0,0.04);
  --panel2: rgba(0,0,0,0.02);
  --border: rgba(0,0,0,0.10);
  --text: rgba(0,0,0,0.88);
  --muted: rgba(0,0,0,0.55);
  --shadow: 0 16px 40px rgba(0,0,0,0.12);
  background: var(--bg-layer, var(--bg));
}
.wrap{ max-width: var(--wrap-max, 1020px); margin:0 auto; padding: var(--wrap-pad, 24px 16px 44px); }
.top, header{ display:flex; justify-content:space-between; align-items:flex-end; gap:12px; }
header{ margin-bottom:16px; }
h1{ margin:0; font-size:20px; }
.page-index h1{ font-size:22px; letter-spacing:0.2px; }
.sub{ margin-top:6px; font-size:13px; color:var(--muted); line-height:1.4; }
a{ color:var(--text); text-decoration:none; }
a.back, .pill{
  text-decoration:none;
  color:rgba(255,255,255,0.88);
  border:1px solid var(--border);
  background:linear-gradient(180deg,var(--panel), var(--panel2));
  padding:10px 12px;
  border-radius:999px;
  font-weight:650;
  font-size:13px;
  display:inline-flex;
  gap:8px;
  align-items:center;
}
.pill{ color:var(--muted); box-shadow:var(--shadow); backdrop-filter: blur(10px); }
.dot{
  width:10px; height:10px; border-radius:50%;
  background:#ffd278;
  box-shadow:0 0 0 6px rgba(255,210,120,0.10);
}

.grid{ display:grid; grid-template-columns:repeat(12,1fr); gap:12px; margin-top:14px; }
.card{
  grid-column:span 6;
  background:linear-gradient(180deg,var(--panel), rgba(255,255,255,0.03));
  border:1px solid var(--border);
  border-radius:var(--radius);
  box-shadow:var(--shadow);
  padding:16px;
  position:relative;
  overflow:hidden;
  backdrop-filter: blur(12px);
  text-decoration:none;
  color:var(--text);
  display:block;
  cursor:pointer;
  transition: transform 120ms ease, border-color 120ms ease, background 120ms ease;
}
.card:hover{ transform: translateY(-1px); border-color: rgba(255,255,255,0.18); }
.card:active{ transform: translateY(0px); }
.card::after{
  content:"";
  position:absolute; inset:-70px -70px auto auto;
  width:200px; height:200px;
  background:radial-gradient(circle at 30% 30%, rgba(255,255,255,0.14), transparent 55%);
  transform:rotate(20deg);
  pointer-events:none;
}
.card.wide{ grid-column:span 12; cursor:default; }
.card.wide:hover{ transform:none; border-color: var(--border); }

.label{ font-size:12px; text-transform:uppercase; letter-spacing:0.3px; color:var(--muted); }
.big{ margin-top:8px; font-size:34px; font-weight:650; letter-spacing:-0.6px; }
.unit{ font-size:14px; color:var(--muted); margin-left:8px; font-weight:500; }
.pair, .row{
  margin-top:10px;
  display:flex; justify-content:space-between; gap:12px;
  font-size:12px; color:var(--muted);
}
.pair b, .row b{ color:rgba(255,255,255,0.88); font-weight:650; }
.chip{
  display:inline-block;
  padding:4px 10px;
  border-radius:999px;
  border:1px solid rgba(255,255,255,0.12);
  background:rgba(255,255,255,0.04);
  color:rgba(255,255,255,0.70);
  font-size:12px;
}
.badge{
  display:inline-flex; align-items:center; gap:8px;
  padding:8px 12px;
  border-radius:999px;
  border:1px solid rgba(255,255,255,0.14);
  background:rgba(255,255,255,0.05);
  color:rgba(255,255,255,0.88);
  font-weight:650;
}
.icon-links{
  display:flex;
  gap:8px;
  align-items:center;
}
.icon-link{
  display:flex;
  align-items:center;
  justify-content:center;
  width:40px;
  height:40px;
  border-radius:12px;
  background:linear-gradient(180deg,var(--panel), var(--panel2));
  border:1px solid var(--border);
  color:var(--muted);
  text-decoration:none;
  transition: background 120ms, border-color 120ms, color 120ms;
}
.icon-link:hover{
  background:rgba(255,255,255,0.08);
  border-color:rgba(255,255,255,0.18);
  color:var(--text);
}
.icon-link svg{ width:18px; height:18px; }

.kpis{display:flex;gap:14px;flex-wrap:wrap}
.kpi, .kpi-item{
  padding:10px 12px;
  border:1px solid var(--border);
  border-radius:999px;
  background:rgba(255,255,255,.04);
  color:var(--muted);
  font-size:13px;
}
.kpi b{color:var(--text)}
.kpi-grid{
  display:grid;
  grid-template-columns:repeat(4,1fr);
  gap:10px;
}
.kpi-label{
  font-size:10px;
  color:var(--muted);
  text-transform:uppercase;
  letter-spacing:0.3px;
}
.kpi-value{
  margin-top:6px;
  font-size:18px;
  font-weight:700;
}
.kpi-unit{
  font-size:11px;
  color:var(--muted);
  font-weight:500;
}
.kpi .k, .kpi .v{ display:block; }
.kpi .k{ font-size:11px; color:rgba(255,255,255,0.55); text-transform:uppercase; letter-spacing:0.3px; }
.kpi .v{ margin-top:6px; font-size:16px; font-weight:650; color:rgba(255,255,255,0.9); }

.spark{
  margin-top:10px;
  width:100%;
  height:44px;
  border-radius:14px;
  border:1px solid rgba(255,255,255,0.10);
  background:rgba(0,0,0,0.18);
  overflow:hidden;
}
.spark svg{ width:100%; height:100%; display:block; }
.spark polyline{ fill:none; stroke:var(--sparkStroke); stroke-width:2; }
.spark path.fill{ fill:var(--sparkFill); stroke:none; }
.spark .gridline{ stroke: rgba(255,255,255,0.06); stroke-width:1; }

.footer{ margin-top:10px; font-size:12px; color:rgba(255,255,255,0.45); line-height:1.4; }

.note{
  margin-top:10px;
  color:rgba(255,255,255,0.45);
  font-size:12px;
  line-height:1.5;
}
.tag{
  display:inline-flex; align-items:center;
  padding:8px 12px;
  border-radius:999px;
  border:1px solid rgba(255,255,255,0.14);
  background:rgba(255,255,255,0.05);
  font-weight:650;
}
.tag.green{ border-color:rgba(120,255,180,0.3); background:rgba(120,255,180,0.08); color:rgba(120,255,180,0.95); }
.tag.yellow{ border-color:rgba(255,210,120,0.3); background:rgba(255,210,120,0.08); color:rgba(255,210,120,0.95); }
.tag.red{ border-color:rgba(255,120,120,0.3); background:rgba(255,120,120,0.08); color:rgba(255,120,120,0.95); }
.tag.blue{ border-color:rgba(120,180,255,0.3); background:rgba(120,180,255,0.08); color:rgba(120,180,255,0.95); }

canvas.chart{
  width:100%;
  display:block;
  border-radius:14px;
  background:rgba(0,0,0,0.20);
  border:1px solid rgba(255,255,255,0.10);
}
canvas.chart-lg{ height:360px; }
canvas.chart-md{ height:260px; }
canvas.chart-sm{ height:220px; }
.sep{ height:10px; }

.section-title{
  font-size:12px;
  font-weight:650;
  text-transform:uppercase;
  letter-spacing:0.5px;
  color:var(--muted);
  margin-bottom:16px;
  display:flex;
  align-items:center;
  gap:8px;
}
.section-title svg{ width:16px; height:16px; opacity:0.7; }
.field{
  display:flex;
  justify-content:space-between;
  align-items:center;
  padding:14px 0;
  border-bottom:1px solid var(--border);
}
.field:last-child{ border-bottom:none; }
.field-label{
  font-size:14px;
  font-weight:500;
}
.field-desc{
  font-size:12px;
  color:var(--muted);
  margin-top:4px;
}
.field-control{
  display:flex;
  align-items:center;
  gap:8px;
}

.stat-row{
  display:flex;
  justify-content:space-between;
  align-items:center;
  padding:10px 0;
  border-bottom:1px solid var(--border);
}
.stat-row:last-child{ border-bottom:none; }
.stat-label{
  font-size:13px;
  color:var(--muted);
}
.stat-value{
  font-size:14px;
  font-weight:600;
  font-family: ui-monospace, SFMono-Regular, "SF Mono", Menlo, monospace;
}
.status-dot{
  display:inline-block;
  width:8px;
  height:8px;
  border-radius:50%;
  margin-right:8px;
}
.status-dot.green{ background:var(--green); box-shadow:0 0 8px var(--green); }
.status-dot.yellow{ background:var(--yellow); box-shadow:0 0 8px var(--yellow); }
.status-dot.red{ background:var(--red); box-shadow:0 0 8px var(--red); }
.bar-container{
  width:100%;
  height:8px;
  background:var(--panel);
  border-radius:4px;
  overflow:hidden;
  margin-top:8px;
}
.bar-fill{
  height:100%;
  border-radius:4px;
  transition: width 0.3s ease;
}
.bar-fill.green{ background: linear-gradient(90deg, #4be3a6, #78ffb4); }
.bar-fill.yellow{ background: linear-gradient(90deg, #ffd278, #ffb040); }
.bar-fill.red{ background: linear-gradient(90deg, #ff8080, #ff4040); }

.forecast-box{
  margin-top:12px;
  padding:16px;
  border-radius:14px;
  border:1px solid rgba(255,255,255,0.12);
  background:rgba(255,255,255,0.04);
  text-align:center;
}
.forecast-text{
  font-size:18px;
  font-weight:700;
  margin-bottom:8px;
}
.forecast-detail{
  font-size:12px;
  color:var(--muted);
}

.meter{
  margin-top:12px;
  height:12px;
  background:rgba(255,255,255,0.08);
  border-radius:6px;
  overflow:hidden;
  position:relative;
}
.meter-fill{
  height:100%;
  border-radius:6px;
  transition: width 0.3s ease;
}
.meter-marker{
  position:absolute;
  top:-4px;
  width:4px;
  height:20px;
  background:white;
  border-radius:2px;
  transform:translateX(-50%);
}

input[type="number"], input[type="text"]{
  background:var(--panel);
  border:1px solid var(--border);
  border-radius:10px;
  padding:10px 12px;
  color:var(--text);
  font-size:14px;
  width:100px;
  text-align:right;
}
input[type="number"]:focus, input[type="text"]:focus{
  outline:none;
  border-color:var(--accent);
}
select{
  background:var(--panel);
  border:1px solid var(--border);
  border-radius:10px;
  padding:10px 12px;
  color:var(--text);
  font-size:14px;
  cursor:pointer;
}
select:focus{ outline:none; border-color:var(--accent); }
.toggle{ position:relative; width:52px; height:28px; }
.toggle input{ opacity:0; width:0; height:0; }
.toggle-slider{
  position:absolute;
  cursor:pointer;
  top:0; left:0; right:0; bottom:0;
  background:var(--panel);
  border:1px solid var(--border);
  border-radius:14px;
  transition: background 0.2s, border-color 0.2s;
}
.toggle-slider:before{
  position:absolute;
  content:"";
  height:20px;
  width:20px;
  left:3px;
  bottom:3px;
  background:white;
  border-radius:50%;
  transition: transform 0.2s;
}
.toggle input:checked + .toggle-slider{
  background:var(--accent);
  border-color:var(--accent);
}
.toggle input:checked + .toggle-slider:before{ transform:translateX(24px); }

.btn{
  background:var(--accent);
  color:#fff;
  border:none;
  border-radius:10px;
  padding:12px 20px;
  font-size:14px;
  font-weight:600;
  cursor:pointer;
  transition: opacity 0.2s;
}
.btn:hover{ opacity:0.85; }
.btn-secondary{
  background:var(--panel);
  border:1px solid var(--border);
  color:var(--text);
}
.actions{
  margin-top:20px;
  display:flex;
  gap:10px;
  flex-wrap:wrap;
}
.saved-msg{
  opacity:0;
  transition: opacity 0.2s;
  color:var(--muted);
  font-size:12px;
  align-self:center;
}
.saved-msg.show{ opacity:1; }

.refresh-btn{
  background:var(--panel);
  border:1px solid var(--border);
  color:var(--text);
  padding:8px 16px;
  border-radius:999px;
  font-size:12px;
  font-weight:600;
  cursor:pointer;
  display:flex;
  align-items:center;
  gap:6px;
}
.refresh-btn:hover{ background:rgba(255,255,255,0.08); }
.refresh-btn svg{ width:14px; height:14px; }
.refresh-btn.loading svg{ animation: spin 1s linear infinite; }
@keyframes spin { from { transform: rotate(0deg); } to { transform: rotate(360deg); } }

@media (max-width:860px){
  .kpi-grid{ grid-template-columns:repeat(2,1fr); }
  .kpi{ grid-template-columns:repeat(2,1fr); }
}
@media (max-width:760px){
  .card{ grid-column:span 12; }
  .top, header{ flex-direction:column; align-items:flex-start; }
  .page-index .card{ grid-column:span 12; }
}
)CSS";

static const char APP_JS[] =
R"JS(
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

  return {min: dataMin, max: dataMax, last: data[data.length-1]};
}

function startSimpleSeriesPage(cfg){
  const nowEl = cfg.nowId ? $(cfg.nowId) : null;
  const minEl = cfg.minId ? $(cfg.minId) : null;
  const maxEl = cfg.maxId ? $(cfg.maxId) : null;
  const decimals = cfg.decimals ?? 1;
  const unit = cfg.unit || '';
  const chart = setupCanvas(cfg.canvasId, cfg.height || 360);

  const render = (series) => {
    const stats = drawLineSeries(chart.ctx, chart.cv, series, {
      fixedMin: cfg.fixedMin,
      fixedMax: cfg.fixedMax,
      padFraction: cfg.padFraction,
      minPad: cfg.minPad
    });
    if (!stats) return;
    if (nowEl) nowEl.textContent = stats.last.toFixed(decimals) + unit;
    if (minEl) minEl.textContent = stats.min.toFixed(decimals) + unit;
    if (maxEl) maxEl.textContent = stats.max.toFixed(decimals) + unit;
  };

  const tick = async () => {
    try{
      const r = await fetch(cfg.endpoint, {cache:'no-store'});
      const j = await r.json();
      if(!j.ok) return;
      render(j.series || []);
    }catch(e){}
  };
  tick();
  setInterval(tick, cfg.pollMs || 2000);
  return {tick, render, chart};
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
)JS";

static const char FAVICON_SVG[] =
R"SVG(
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 64 64">
  <defs>
    <radialGradient id="g" cx="30%" cy="30%" r="80%">
      <stop offset="0%" stop-color="#b8d7ff"/>
      <stop offset="45%" stop-color="#7aa7ff"/>
      <stop offset="100%" stop-color="#334cff"/>
    </radialGradient>
  </defs>
  <rect x="6" y="6" width="52" height="52" rx="14" fill="url(#g)"/>
  <path d="M18 40a14 14 0 1 1 28 0" fill="none" stroke="white" stroke-width="4" stroke-linecap="round"/>
  <path d="M32 26l10 12" stroke="white" stroke-width="4" stroke-linecap="round"/>
  <circle cx="32" cy="40" r="4" fill="white"/>
</svg>
)SVG";
