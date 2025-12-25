#pragma once
#include <WiFiS3.h>

static void sendPageHumidity(WiFiClient &client) {
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
<title>Humidity</title>
<link rel="icon" href="data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 64 64'%3E%3Crect x='6' y='6' width='52' height='52' rx='14' fill='%2350d6b0'/%3E%3Cpath d='M32 16c8 12 12 17 12 24a12 12 0 1 1-24 0c0-7 4-12 12-24z' fill='white'/%3E%3C/svg%3E">
<style>
:root{--bg:#070a18;--text:rgba(255,255,255,.92);--muted:rgba(255,255,255,.62);--panel:rgba(255,255,255,.06);--border:rgba(255,255,255,.12)}
*{box-sizing:border-box}
body{margin:0;color:var(--text);font-family:system-ui;background:radial-gradient(1200px 700px at 20% 10%, rgba(120,255,200,0.18), transparent 60%),var(--bg);min-height:100vh}
.wrap{max-width:1000px;margin:0 auto;padding:22px 14px 30px}
a{color:var(--text);text-decoration:none}
.top{display:flex;justify-content:space-between;align-items:flex-end;gap:12px}
h1{margin:0;font-size:20px}
.sub{color:var(--muted);font-size:13px;margin-top:6px}
.card{margin-top:14px;padding:14px;border-radius:18px;background:var(--panel);border:1px solid var(--border);backdrop-filter:blur(12px)}
.kpis{display:flex;gap:14px;flex-wrap:wrap}
.kpi{padding:10px 12px;border:1px solid var(--border);border-radius:999px;background:rgba(255,255,255,.04);color:var(--muted);font-size:13px}
.kpi b{color:var(--text)}
canvas{width:100%;height:360px;display:block;border-radius:14px;background:rgba(0,0,0,.20)}
</style>
</head>
<body>
<div class="wrap">
  <div class="top">
    <div>
      <h1>Humidity</h1>
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
      <canvas id="cv" width="1000" height="360"></canvas>
    </div>
  </div>
</div>

<script>
const cv = document.getElementById('cv');
const ctx = cv.getContext('2d');

function resize(){
  const r = cv.getBoundingClientRect();
  const dpr = Math.max(1, window.devicePixelRatio || 1);
  cv.width = Math.floor(r.width * dpr);
  cv.height = Math.floor(360 * dpr);
}
window.addEventListener('resize', resize);
resize();

function draw(series){
  const w = cv.width, h = cv.height;
  ctx.clearRect(0,0,w,h);

  const data = series.filter(v => typeof v === 'number' && isFinite(v));
  if (data.length < 2) return;

  let mn = 0, mx = 100; // humidity is naturally bounded
  let rng = mx - mn;

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
    const y = v===null ? h/2 : (h - ((v - mn)/rng)*h);
    if(i===0) ctx.moveTo(x,y); else ctx.lineTo(x,y);
  }
  ctx.stroke();

  ctx.globalAlpha = 0.10;
  ctx.fillStyle = 'rgba(255,255,255,1)';
  ctx.lineTo(w,h); ctx.lineTo(0,h); ctx.closePath();
  ctx.fill();
  ctx.globalAlpha = 1;

  const last = series[series.length-1];
  now.textContent = last.toFixed(1) + " %";
  minv.textContent = Math.min(...data).toFixed(1) + " %";
  maxv.textContent = Math.max(...data).toFixed(1) + " %";
}

async function tick(){
  try{
    const r = await fetch('/api/humidity', {cache:'no-store'});
    const j = await r.json();
    if(!j.ok) return;
    draw(j.series || []);
  }catch(e){}
}
tick(); setInterval(tick, 2000);  // 2s polling - data updates at 1Hz anyway
</script>
</body></html>
)HTML");
}
