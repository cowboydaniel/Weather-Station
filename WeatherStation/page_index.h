#pragma once
#include <WiFiS3.h>

static void sendPageIndex(WiFiClient &client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html; charset=utf-8");
  client.println("Cache-Control: no-store");
  client.println("Connection: close");
  client.println();

  client.println(R"HTML(
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>UNO R4 Weather</title>

<link rel="icon" href="data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 64 64'%3E%3Cdefs%3E%3CradialGradient id='g' cx='30%25' cy='30%25' r='80%25'%3E%3Cstop offset='0%25' stop-color='%23b8d7ff'/%3E%3Cstop offset='45%25' stop-color='%237aa7ff'/%3E%3Cstop offset='100%25' stop-color='%23334cff'/%3E%3C/radialGradient%3E%3C/defs%3E%3Crect x='6' y='6' width='52' height='52' rx='14' fill='url(%23g)'/%3E%3Cpath d='M18 40a14 14 0 1 1 28 0' fill='none' stroke='white' stroke-width='4' stroke-linecap='round'/%3E%3Cpath d='M32 26l10 12' stroke='white' stroke-width='4' stroke-linecap='round'/%3E%3Ccircle cx='32' cy='40' r='4' fill='white'/%3E%3C/svg%3E">

<style>
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
}
*{ box-sizing:border-box; }
body{
  margin:0;
  font-family: ui-sans-serif, system-ui, -apple-system, Segoe UI, Roboto, Arial;
  color:var(--text);
  background:
    radial-gradient(1200px 700px at 15% 10%, rgba(110,160,255,0.22), transparent 60%),
    radial-gradient(900px 600px at 85% 25%, rgba(255,110,180,0.18), transparent 55%),
    radial-gradient(1000px 900px at 55% 110%, rgba(120,255,200,0.12), transparent 60%),
    radial-gradient(600px 600px at 55% 30%, rgba(255,255,255,0.06), transparent 50%),
    var(--bg);
  min-height:100vh;
}
.wrap{ max-width: 1080px; margin:0 auto; padding:28px 16px 44px; }
header{
  display:flex; justify-content:space-between; align-items:flex-end; gap:12px;
  margin-bottom:16px;
}
h1{ margin:0; font-size:22px; letter-spacing:0.2px; }
.sub{ margin-top:6px; font-size:13px; color:var(--muted); line-height:1.4; }

.pill{
  background:linear-gradient(180deg,var(--panel), var(--panel2));
  border:1px solid var(--border);
  border-radius:999px;
  padding:10px 12px;
  box-shadow:var(--shadow);
  font-size:12px;
  color:var(--muted);
  display:flex; gap:10px; align-items:center;
  white-space:nowrap;
  backdrop-filter: blur(10px);
}
.dot{
  width:10px; height:10px; border-radius:50%;
  background:#ffd278;
  box-shadow:0 0 0 6px rgba(255,210,120,0.10);
}

.grid{ display:grid; grid-template-columns:repeat(12,1fr); gap:12px; }
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
.card:hover{
  transform: translateY(-1px);
  border-color: rgba(255,255,255,0.18);
}
.card:active{
  transform: translateY(0px);
}
.card::after{
  content:"";
  position:absolute; inset:-70px -70px auto auto;
  width:200px; height:200px;
  background:radial-gradient(circle at 30% 30%, rgba(255,255,255,0.14), transparent 55%);
  transform:rotate(20deg);
  pointer-events:none;
}

.wide{ grid-column:span 12; cursor:default; }
.wide:hover{ transform:none; border-color: var(--border); }

.label{ font-size:12px; text-transform:uppercase; letter-spacing:0.3px; color:var(--muted); }
.big{ margin-top:8px; font-size:34px; font-weight:650; letter-spacing:-0.6px; }
.unit{ font-size:14px; color:var(--muted); margin-left:8px; font-weight:500; }
.pair{
  margin-top:10px;
  display:flex; justify-content:space-between; gap:12px;
  font-size:12px; color:var(--muted);
}
.pair b{ color:rgba(255,255,255,0.88); font-weight:650; }
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
.icon-link svg{
  width:18px;
  height:18px;
}

@media (max-width:760px){
  .card{ grid-column:span 12; }
  header{ flex-direction:column; align-items:flex-start; }
}
</style>
</head>

<body>
<div class="wrap">
  <header>
    <div>
      <h1>UNO R4 Weather Station</h1>
      <div class="sub">Tap a card to open the full graph page. Comfort has its own page too.</div>
    </div>

    <div style="display:flex; gap:12px; align-items:center;">
      <div class="icon-links">
        <a class="icon-link" href="/stats" title="System Stats">
          <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><rect x="3" y="3" width="7" height="9"/><rect x="14" y="3" width="7" height="5"/><rect x="14" y="12" width="7" height="9"/><rect x="3" y="16" width="7" height="5"/></svg>
        </a>
        <a class="icon-link" href="/settings" title="Settings">
          <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="3"/><path d="M19.4 15a1.65 1.65 0 00.33 1.82l.06.06a2 2 0 010 2.83 2 2 0 01-2.83 0l-.06-.06a1.65 1.65 0 00-1.82-.33 1.65 1.65 0 00-1 1.51V21a2 2 0 01-2 2 2 2 0 01-2-2v-.09A1.65 1.65 0 009 19.4a1.65 1.65 0 00-1.82.33l-.06.06a2 2 0 01-2.83 0 2 2 0 010-2.83l.06-.06a1.65 1.65 0 00.33-1.82 1.65 1.65 0 00-1.51-1H3a2 2 0 01-2-2 2 2 0 012-2h.09A1.65 1.65 0 004.6 9a1.65 1.65 0 00-.33-1.82l-.06-.06a2 2 0 010-2.83 2 2 0 012.83 0l.06.06a1.65 1.65 0 001.82.33H9a1.65 1.65 0 001-1.51V3a2 2 0 012-2 2 2 0 012 2v.09a1.65 1.65 0 001 1.51 1.65 1.65 0 001.82-.33l.06-.06a2 2 0 012.83 0 2 2 0 010 2.83l-.06.06a1.65 1.65 0 00-.33 1.82V9a1.65 1.65 0 001.51 1H21a2 2 0 012 2 2 2 0 01-2 2h-.09a1.65 1.65 0 00-1.51 1z"/></svg>
        </a>
      </div>
      <div class="pill">
        <div class="dot" id="dot"></div>
        <div>
          <div id="status">Connecting…</div>
          <div style="font-size:11px; color: rgba(255,255,255,0.45);" id="meta"></div>
        </div>
      </div>
    </div>
  </header>

  <div class="grid">
    <a class="card" href="/temp">
      <div class="label">Temperature</div>
      <div class="big"><span id="t">--</span><span class="unit">°C</span></div>
      <div class="pair"><span>Raw</span><b id="t_raw">--</b></div>
      <div class="footer">10-minute history (1 Hz).</div>
    </a>

    <a class="card" href="/humidity">
      <div class="label">Humidity</div>
      <div class="big"><span id="h">--</span><span class="unit">%</span></div>
      <div class="pair"><span>Raw</span><b id="h_raw">--</b></div>
      <div class="footer">10-minute history (1 Hz).</div>
    </a>

    <a class="card" href="/pressure">
      <div class="label">Sea-level Pressure</div>
      <div class="big"><span id="slp">--</span><span class="unit">hPa</span></div>
      <div class="pair"><span>Tendency</span><b id="pt">--</b></div>
      <div class="pair"><span>Slope</span><b id="ps">--</b></div>

      <div class="spark">
        <svg viewBox="0 0 100 44" preserveAspectRatio="none">
          <line class="gridline" x1="0" y1="22" x2="100" y2="22"></line>
          <path class="fill" id="slpFill" d=""></path>
          <polyline id="slpLine" points=""></polyline>
        </svg>
      </div>
      <div class="footer">Sea-level pressure trend (minute samples). Points: <span id="trend_pts">--</span></div>
    </a>

    <a class="card" href="/gas">
      <div class="label">Gas Resistance</div>
      <div class="big"><span id="g">--</span><span class="unit">kΩ</span></div>
      <div class="pair"><span>Raw</span><b id="g_raw">--</b></div>

      <div class="spark">
        <svg viewBox="0 0 100 44" preserveAspectRatio="none">
          <line class="gridline" x1="0" y1="22" x2="100" y2="22"></line>
          <path class="fill" id="gasFill" d=""></path>
          <polyline id="gasLine" points=""></polyline>
        </svg>
      </div>
      <div class="footer">Gas history (3-second samples).</div>
    </a>

    <a class="card" href="/comfort">
      <div class="label">Comfort</div>
      <div class="big"><span id="dp">--</span><span class="unit">°C dew point</span></div>
      <div class="pair"><span>Heat index</span><b id="hi">--</b></div>
      <div class="footer">Dew point, heat index, and quick human heuristics.</div>
    </a>

    <div class="card wide">
      <div class="label">Storm heuristic</div>
      <div style="margin-top:10px; display:flex; flex-wrap:wrap; gap:10px; align-items:center;">
        <span class="badge" id="storm_badge">--</span>
        <span class="chip">Score: <span id="storm_score">--</span>/100</span>
      </div>
      <div class="footer">
        Score increases with lower sea-level pressure, falling pressure, and higher humidity. It is a local change detector.
      </div>
    </div>
  </div>
</div>

<script>
const el = (id) => document.getElementById(id);

function setDot(ok){
  const dot = el('dot');
  dot.style.background = ok ? 'rgba(120,255,180,0.95)' : 'rgba(255,120,120,0.95)';
  dot.style.boxShadow = ok ? '0 0 0 6px rgba(120,255,180,0.10)' : '0 0 0 6px rgba(255,120,120,0.10)';
}

function f(x, d=1){ return (typeof x === 'number' && isFinite(x)) ? x.toFixed(d) : '--'; }

function sparkline(points, polyId, fillId){
  const pl = el(polyId);
  const pf = el(fillId);

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

// Use combined dashboard endpoint for efficiency (single request instead of 3)
async function tick(){
  try{
    const res = await fetch('/api/dashboard', { cache:'no-store' });
    const j = await res.json();
    if(!j.ok) throw new Error('api_error');

    el('t').textContent = f(j.raw.temp_c, 1);
    el('h').textContent = f(j.raw.hum_pct, 1);
    el('g').textContent = f(j.raw.gas_kohm, 1);
    el('t_raw').textContent = f(j.raw.temp_c, 2);
    el('h_raw').textContent = f(j.raw.hum_pct, 2);
    el('g_raw').textContent = f(j.raw.gas_kohm, 2);

    el('slp').textContent = f(j.derived.slp_hpa, 1);
    el('dp').textContent = f(j.derived.dew_point_c, 1);
    el('hi').textContent = f(j.derived.heat_index_c, 1) + " °C";

    el('pt').textContent = j.derived.press_tendency || '--';
    el('ps').textContent = f(j.derived.press_tendency_hpa_hr, 2) + " hPa/hr";

    el('storm_score').textContent = (typeof j.derived.storm_score === 'number') ? j.derived.storm_score : '--';
    el('storm_badge').textContent = (j.derived.storm || '--') + " (" + (j.derived.storm_score ?? '--') + ")";

    // Sparklines from combined response
    sparkline(j.slp_trend || [], "slpLine", "slpFill");
    el('trend_pts').textContent = (j.slp_trend && j.slp_trend.length) ? j.slp_trend.length : '--';
    sparkline(j.gas_series || [], "gasLine", "gasFill");

    el('status').textContent = 'Live';
    el('meta').textContent = 'Updated ' + new Date().toLocaleTimeString();
    setDot(true);
  } catch(e){
    el('status').textContent = 'Offline';
    el('meta').textContent = e.message;
    setDot(false);
  }
}

tick();
setInterval(tick, 2000);  // Reduced from 1s to 2s - data only updates at 1Hz anyway
</script>
</body>
</html>
)HTML");
}
