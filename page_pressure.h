#pragma once
#include <WiFiS3.h>

static void sendPagePressure(WiFiClient &client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html; charset=utf-8");
  client.println("Cache-Control: no-store");
  client.println("Connection: close");
  client.println();

  client.println(R"HTML(
<!doctype html>
<html><head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Pressure</title>
<link rel="icon" href="data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 64 64'%3E%3Crect x='6' y='6' width='52' height='52' rx='14' fill='%23ff7ab7'/%3E%3Cpath d='M18 40a14 14 0 1 1 28 0' fill='none' stroke='white' stroke-width='4' stroke-linecap='round'/%3E%3Cpath d='M32 26l10 12' stroke='white' stroke-width='4' stroke-linecap='round'/%3E%3Ccircle cx='32' cy='40' r='4' fill='white'/%3E%3C/svg%3E">
<style>
:root{--bg:#070a18;--text:rgba(255,255,255,.92);--muted:rgba(255,255,255,.62);--panel:rgba(255,255,255,.06);--border:rgba(255,255,255,.12)}
*{box-sizing:border-box}
body{margin:0;color:var(--text);font-family:system-ui;background:radial-gradient(1200px 700px at 80% 10%, rgba(255,110,180,0.18), transparent 60%),var(--bg);min-height:100vh}
.wrap{max-width:1000px;margin:0 auto;padding:22px 14px 30px}
a{color:var(--text);text-decoration:none}
.top{display:flex;justify-content:space-between;align-items:flex-end;gap:12px}
h1{margin:0;font-size:20px}
.sub{color:var(--muted);font-size:13px;margin-top:6px}
.card{margin-top:14px;padding:14px;border-radius:18px;background:var(--panel);border:1px solid var(--border);backdrop-filter:blur(12px)}
.kpis{display:flex;gap:14px;flex-wrap:wrap}
.kpi{padding:10px 12px;border:1px solid var(--border);border-radius:999px;background:rgba(255,255,255,.04);color:var(--muted);font-size:13px}
.kpi b{color:var(--text)}
canvas{width:100%;height:260px;display:block;border-radius:14px;background:rgba(0,0,0,.20)}
.sep{height:10px}
</style>
</head>
<body>
<div class="wrap">
  <div class="top">
    <div>
      <h1>Pressure</h1>
      <div class="sub">Station pressure (10 min, 1 Hz) and sea-level trend (60 min, 1/min)</div>
    </div>
    <a class="kpi" href="/">Back</a>
  </div>

  <div class="card">
    <div class="kpis">
      <div class="kpi">SLP now: <b id="slp">--</b></div>
      <div class="kpi">Tendency: <b id="tend">--</b></div>
      <div class="kpi">Slope: <b id="slope">--</b></div>
    </div>

    <div class="sep"></div>
    <div class="sub">Station pressure (hPa)</div>
    <canvas id="cv1" width="1000" height="260"></canvas>

    <div class="sep"></div>
    <div class="sub">Sea-level pressure trend (hPa)</div>
    <canvas id="cv2" width="1000" height="260"></canvas>
  </div>
</div>

<script>
function setupCanvas(id, hPx){
  const cv = document.getElementById(id);
  const ctx = cv.getContext('2d');
  function resize(){
    const r = cv.getBoundingClientRect();
    const dpr = Math.max(1, window.devicePixelRatio || 1);
    cv.width = Math.floor(r.width * dpr);
    cv.height = Math.floor(hPx * dpr);
  }
  window.addEventListener('resize', resize);
  resize();
  return {cv, ctx};
}

const A = setupCanvas('cv1', 260);
const B = setupCanvas('cv2', 260);

function draw(ctx, cv, series){
  const w = cv.width, h = cv.height;
  ctx.clearRect(0,0,w,h);

  const data = series.filter(v => typeof v === 'number' && isFinite(v));
  if (data.length < 2) return;

  let mn = data[0], mx = data[0];
  for (const v of data){ if(v<mn) mn=v; if(v>mx) mx=v; }
  let pad = Math.max(0.2, (mx - mn) * 0.15);
  mn -= pad; mx += pad;
  let rng = mx - mn; if (rng < 1e-6) rng = 1;

  ctx.strokeStyle = 'rgba(255,255,255,0.06)';
  ctx.lineWidth = 1;
  for(let i=1;i<6;i++){
    const y=(h*i)/6;
    ctx.beginPath(); ctx.moveTo(0,y); ctx.lineTo(w,y); ctx.stroke();
  }

  ctx.lineWidth = 3;
  ctx.strokeStyle = 'rgba(255,255,255,0.88)';
  ctx.beginPath();
  for(let i=0;i<series.length;i++){
    const v = (typeof series[i] === 'number' && isFinite(series[i])) ? series[i] : null;
    const x = (i/(series.length-1))*w;
    const y = v===null ? h/2 : (h - ((v - mn)/rng)*h);
    if(i===0) ctx.moveTo(x,y); else ctx.lineTo(x,y);
  }
  ctx.stroke();

  ctx.globalAlpha = 0.10;
  ctx.fillStyle = 'rgba(255,255,255,1)';
  ctx.lineTo(w,h); ctx.lineTo(0,h); ctx.closePath();
  ctx.fill();
  ctx.globalAlpha = 1;
}

async function tick(){
  try{
    const r = await fetch('/api/pressure', {cache:'no-store'});
    const j = await r.json();
    if(!j.ok) return;

    slp.textContent = j.slp_now.toFixed(1) + " hPa";
    tend.textContent = j.tendency;
    slope.textContent = j.tendency_hpa_hr.toFixed(2) + " hPa/hr";

    draw(A.ctx, A.cv, j.station_series || []);
    draw(B.ctx, B.cv, j.slp_trend_series || []);
  }catch(e){}
}
tick(); setInterval(tick, 2000);  // 2s polling - data updates at 1Hz anyway
</script>
</body></html>
)HTML");
}
