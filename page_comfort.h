#pragma once
#include <WiFiS3.h>

static void sendPageComfort(WiFiClient &client) {
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
<title>Comfort</title>

<link rel="icon" href="data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 64 64'%3E%3Cdefs%3E%3CradialGradient id='g' cx='30%25' cy='30%25' r='80%25'%3E%3Cstop offset='0%25' stop-color='%23baffd7'/%3E%3Cstop offset='45%25' stop-color='%234be3a6'/%3E%3Cstop offset='100%25' stop-color='%2306b38a'/%3E%3C/radialGradient%3E%3C/defs%3E%3Crect x='6' y='6' width='52' height='52' rx='14' fill='url(%23g)'/%3E%3Cpath d='M32 18c8 10 12 15 12 22a12 12 0 1 1-24 0c0-7 4-12 12-22z' fill='white' opacity='0.95'/%3E%3C/svg%3E">

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
}
*{ box-sizing:border-box; }
body{
  margin:0;
  font-family: ui-sans-serif, system-ui, -apple-system, Segoe UI, Roboto, Arial;
  color:var(--text);
  background:
    radial-gradient(1200px 700px at 20% 10%, rgba(120,255,200,0.20), transparent 60%),
    radial-gradient(900px 600px at 80% 25%, rgba(110,160,255,0.14), transparent 55%),
    radial-gradient(800px 800px at 60% 115%, rgba(255,110,180,0.10), transparent 60%),
    var(--bg);
  min-height:100vh;
}
.wrap{ max-width: 1020px; margin:0 auto; padding:24px 16px 44px; }
.top{
  display:flex; justify-content:space-between; align-items:flex-end; gap:12px;
}
h1{ margin:0; font-size:20px; }
.sub{ margin-top:6px; font-size:13px; color:var(--muted); line-height:1.4; }
a.back{
  text-decoration:none;
  color:rgba(255,255,255,0.88);
  border:1px solid rgba(255,255,255,0.14);
  background:rgba(255,255,255,0.05);
  padding:10px 12px;
  border-radius:999px;
  font-weight:650;
  font-size:13px;
}
.grid{ margin-top:14px; display:grid; grid-template-columns:repeat(12,1fr); gap:12px; }
.card{
  grid-column:span 6;
  background:linear-gradient(180deg,var(--panel), rgba(255,255,255,0.03));
  border:1px solid var(--border);
  border-radius:var(--radius);
  box-shadow:var(--shadow);
  padding:16px;
  backdrop-filter: blur(12px);
}
.wide{ grid-column:span 12; }
.label{ font-size:12px; text-transform:uppercase; letter-spacing:0.3px; color:var(--muted); }
.big{ margin-top:8px; font-size:34px; font-weight:650; letter-spacing:-0.6px; }
.unit{ font-size:14px; color:var(--muted); margin-left:8px; font-weight:500; }
.row{ margin-top:10px; display:flex; justify-content:space-between; gap:12px; font-size:12px; color:var(--muted); }
.row b{ color:rgba(255,255,255,0.88); font-weight:650; }
.tag{
  display:inline-flex; align-items:center;
  padding:8px 12px;
  border-radius:999px;
  border:1px solid rgba(255,255,255,0.14);
  background:rgba(255,255,255,0.05);
  font-weight:650;
}
.note{ margin-top:10px; color:rgba(255,255,255,0.45); font-size:12px; line-height:1.5; }
canvas{
  width:100%;
  height:220px;
  display:block;
  border-radius:14px;
  background:rgba(0,0,0,0.20);
  border:1px solid rgba(255,255,255,0.10);
}
.kpi{ display:grid; grid-template-columns:repeat(4,1fr); gap:10px; margin-top:12px; }
.kpi > div{
  border:1px solid rgba(255,255,255,0.12);
  border-radius:14px;
  padding:10px 12px;
  background:rgba(255,255,255,0.04);
}
.kpi .k{ font-size:11px; color:rgba(255,255,255,0.55); text-transform:uppercase; letter-spacing:0.3px; }
.kpi .v{ margin-top:6px; font-size:16px; font-weight:650; color:rgba(255,255,255,0.9); }
@media (max-width:860px){ .kpi{ grid-template-columns:repeat(2,1fr);} }
@media (max-width:760px){ .card{ grid-column:span 12; } }
</style>
</head>

<body>
<div class="wrap">
  <div class="top">
    <div>
      <h1>Comfort</h1>
      <div class="sub">Dew point, heat index, extra comfort metrics, plus sun and moon data computed locally.</div>
    </div>
    <a class="back" href="/">Back</a>
  </div>

  <div class="grid">
    <div class="card">
      <div class="label">Dew point</div>
      <div class="big"><span id="dp">--</span><span class="unit">°C</span></div>
      <div class="row"><span>Rule of thumb</span><b id="dp_rule">--</b></div>
      <div class="note">Dew point tracks “sticky air” better than humidity percent.</div>
    </div>

    <div class="card">
      <div class="label">Heat index</div>
      <div class="big"><span id="hi">--</span><span class="unit">°C</span></div>
      <div class="row"><span>Effect</span><b id="hi_rule">--</b></div>
      <div class="note">Heat index diverges from temp mainly when warm and humid.</div>
    </div>

    <div class="card wide">
      <div class="label">Quick comfort read</div>
      <div style="margin-top:10px; display:flex; flex-wrap:wrap; gap:10px; align-items:center;">
        <span class="tag" id="comfort_tag">--</span>
        <span class="tag" id="mold_tag">--</span>
        <span class="tag" id="time_tag">Time: --</span>
      </div>
      <div class="note" id="comfort_note">--</div>

      <div class="kpi">
        <div><div class="k">Wet-bulb</div><div class="v" id="wb">--</div></div>
        <div><div class="k">Abs humidity</div><div class="v" id="ah">--</div></div>
        <div><div class="k">VPD</div><div class="v" id="vpd">--</div></div>
        <div><div class="k">Storm score</div><div class="v" id="storm">--</div></div>
      </div>
      <div class="note">Wet-bulb matters for heat stress. VPD is “drying power” of the air (plants and skin care about it).</div>
    </div>

    <div class="card wide">
      <div class="label">Dew point history</div>
      <div style="margin-top:12px">
        <canvas id="cv_dp"></canvas>
      </div>
      <div class="note">Derived in-browser from your temp and humidity history.</div>
    </div>

    <div class="card wide">
      <div class="label">Heat index history</div>
      <div style="margin-top:12px">
        <canvas id="cv_hi"></canvas>
      </div>
      <div class="note">Derived in-browser from your temp and humidity history.</div>
    </div>

    <div class="card wide">
      <div class="label">Sun and moon</div>
      <div class="kpi">
        <div><div class="k">Sunrise</div><div class="v" id="sunrise">--</div></div>
        <div><div class="k">Sunset</div><div class="v" id="sunset">--</div></div>
        <div><div class="k">Day length</div><div class="v" id="daylen">--</div></div>
        <div><div class="k">Solar elevation</div><div class="v" id="elev">--</div></div>
      </div>
      <div class="kpi" style="margin-top:10px">
        <div><div class="k">Solar azimuth</div><div class="v" id="az">--</div></div>
        <div><div class="k">Moon phase</div><div class="v" id="mphase">--</div></div>
        <div><div class="k">Moon illum</div><div class="v" id="millum">--</div></div>
        <div><div class="k">Notes</div><div class="v" id="astro_note">--</div></div>
      </div>
      <div class="note">
        These are computed locally using NTP time plus the lat/lon you set in the sketch. If time is not valid yet, it will show placeholders.
      </div>
    </div>
  </div>
</div>

<script>
const el = (id) => document.getElementById(id);

const cvDP = el('cv_dp');
const cvHI = el('cv_hi');
const ctxDP = cvDP.getContext('2d');
const ctxHI = cvHI.getContext('2d');

function resizeCanvas(cv){
  const r = cv.getBoundingClientRect();
  const dpr = Math.max(1, window.devicePixelRatio || 1);
  cv.width = Math.floor(r.width * dpr);
  cv.height = Math.floor(220 * dpr);
}

function resizeAll(){
  resizeCanvas(cvDP);
  resizeCanvas(cvHI);
}
window.addEventListener('resize', resizeAll);
resizeAll();

function dewPointC(tC, rh){
  rh = Math.max(1, Math.min(rh, 100));
  const a = 17.62, b = 243.12;
  const gamma = (a * tC) / (b + tC) + Math.log(rh / 100.0);
  return (b * gamma) / (a - gamma);
}

function heatIndexC(tC, rh){
  if (!isFinite(tC) || !isFinite(rh)) return NaN;
  if (tC < 26 || rh < 40) return tC;
  const T = tC * 9/5 + 32;
  const R = rh;
  const HI =
    -42.379 +
    2.04901523*T +
    10.14333127*R +
    -0.22475541*T*R +
    -0.00683783*T*T +
    -0.05481717*R*R +
    0.00122874*T*T*R +
    0.00085282*T*R*R +
    -0.00000199*T*T*R*R;
  return (HI - 32) * 5/9;
}

function dpRule(dp){
  if (!isFinite(dp)) return "--";
  if (dp < 10) return "dry";
  if (dp < 16) return "comfortable";
  if (dp < 19) return "a bit humid";
  if (dp < 22) return "humid";
  if (dp < 24) return "very humid";
  return "oppressive";
}

function hiRule(hi, t){
  if (!isFinite(hi) || !isFinite(t)) return "--";
  const d = hi - t;
  if (Math.abs(d) < 0.5) return "near actual temp";
  if (d < 2) return "slightly warmer";
  if (d < 5) return "noticeably warmer";
  return "much warmer";
}

function comfortTag(dp, hi){
  if (!isFinite(dp) || !isFinite(hi)) return "unknown";
  if (hi >= 35 || dp >= 24) return "high stress";
  if (hi >= 30 || dp >= 21) return "uncomfortable";
  if (dp >= 19) return "sticky";
  if (dp >= 16) return "comfortable";
  return "crisp";
}

function moldTag(dp, rh){
  if (!isFinite(dp) || !isFinite(rh)) return "mold: unknown";
  if (rh >= 70) return "mold: elevated";
  if (rh >= 60) return "mold: moderate";
  return "mold: low";
}

function drawSeries(ctx, cv, series){
  const w = cv.width, h = cv.height;
  ctx.clearRect(0,0,w,h);

  const data = series.filter(v => typeof v === 'number' && isFinite(v));
  if (data.length < 2) return;

  let mn = data[0], mx = data[0];
  for (const v of data){ if (v < mn) mn = v; if (v > mx) mx = v; }
  let pad = Math.max(0.2, (mx - mn) * 0.12);
  mn -= pad; mx += pad;
  let rng = mx - mn; if (rng < 1e-6) rng = 1;

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
}

// ======= Astronomy (browser-side) =======
// We compute sunrise/sunset using a compact NOAA-style approximation.
// Good enough for a hobby station. Expect a few minutes error sometimes.

const DEG2RAD = Math.PI/180;
const RAD2DEG = 180/Math.PI;

function clamp(x,a,b){ return Math.max(a, Math.min(b,x)); }

function julianDayFromUnix(utcEpoch){
  return (utcEpoch / 86400.0) + 2440587.5;
}

// Return fractional year in radians, approximate
function solarDeclinationAndEqTime(utcEpoch){
  const jd = julianDayFromUnix(utcEpoch);
  const n = jd - 2451545.0; // days since J2000
  const g = (357.529 + 0.98560028 * n) * DEG2RAD; // mean anomaly
  const q = (280.459 + 0.98564736 * n) * DEG2RAD; // mean longitude
  const L = (q + (1.915*Math.sin(g) + 0.020*Math.sin(2*g)) * DEG2RAD); // ecliptic longitude
  const e = (23.439 - 0.00000036 * n) * DEG2RAD; // obliquity

  const sinDecl = Math.sin(e) * Math.sin(L);
  const decl = Math.asin(sinDecl);

  // Equation of time (minutes) approximate
  const y = Math.tan(e/2); 
  const y2 = y*y;
  const eqTime = 4 * RAD2DEG * (
    y2*Math.sin(2*q) - 2*0.016708*Math.sin(g) + 4*0.016708*y2*Math.sin(g)*Math.cos(2*q)
    - 0.5*y2*y2*Math.sin(4*q) - 1.25*0.016708*0.016708*Math.sin(2*g)
  );

  return { decl, eqTime };
}

function sunriseSunset(utcEpoch, latDeg, lonDeg, tzOffsetSec){
  // Compute for local date (at noon local)
  const localEpoch = utcEpoch + tzOffsetSec;
  const localDays = Math.floor(localEpoch / 86400);
  const localNoonEpoch = localDays*86400 + 12*3600 - tzOffsetSec;

  const { decl, eqTime } = solarDeclinationAndEqTime(localNoonEpoch);

  const lat = latDeg * DEG2RAD;
  const zenith = 90.833 * DEG2RAD; // includes refraction

  const cosH =
    (Math.cos(zenith) - Math.sin(lat)*Math.sin(decl)) /
    (Math.cos(lat)*Math.cos(decl));

  if (cosH > 1) return { polar:"night" };
  if (cosH < -1) return { polar:"day" };

  const H = Math.acos(cosH) * RAD2DEG; // degrees
  const solarNoonMin = 720 - 4*lonDeg - eqTime + (tzOffsetSec/60);
  const sunriseMin = solarNoonMin - 4*H;
  const sunsetMin  = solarNoonMin + 4*H;

  return { sunriseMin, sunsetMin, solarNoonMin };
}

function formatHM(minutes){
  if (!isFinite(minutes)) return "--";
  minutes = ((minutes % 1440) + 1440) % 1440;
  const h = Math.floor(minutes/60);
  const m = Math.floor(minutes%60);
  const hh = String(h).padStart(2,'0');
  const mm = String(m).padStart(2,'0');
  return hh + ":" + mm;
}

function solarPosition(utcEpoch, latDeg, lonDeg){
  const { decl, eqTime } = solarDeclinationAndEqTime(utcEpoch);

  const jd = julianDayFromUnix(utcEpoch);
  const n = jd - 2451545.0;

  // True solar time (minutes)
  const timeUTCmin = (utcEpoch % 86400) / 60.0;
  const tst = (timeUTCmin + eqTime + 4*lonDeg + 1440) % 1440;
  const haDeg = (tst / 4) - 180; // hour angle degrees

  const lat = latDeg * DEG2RAD;
  const ha = haDeg * DEG2RAD;

  const sinAlt = Math.sin(lat)*Math.sin(decl) + Math.cos(lat)*Math.cos(decl)*Math.cos(ha);
  const alt = Math.asin(sinAlt);

  const cosAz = (Math.sin(decl) - Math.sin(alt)*Math.sin(lat)) / (Math.cos(alt)*Math.cos(lat));
  let az = Math.acos(clamp(cosAz, -1, 1));
  if (Math.sin(ha) > 0) az = 2*Math.PI - az;

  return { elevDeg: alt*RAD2DEG, azDeg: az*RAD2DEG };
}

// Moon phase: simple synodic month approximation
function moonPhase(utcEpoch){
  // Reference new moon near 2000-01-06 18:14 UTC (known epoch)
  const ref = 947182440; // seconds
  const synodic = 29.53058867; // days
  const days = (utcEpoch - ref) / 86400.0;
  const frac = ((days / synodic) % 1 + 1) % 1; // 0..1
  const ageDays = frac * synodic;

  // Illumination fraction approx: (1 - cos(phaseAngle))/2
  const phaseAngle = 2*Math.PI*frac;
  const illum = (1 - Math.cos(phaseAngle)) / 2;

  let name;
  if (frac < 0.03 || frac > 0.97) name = "new";
  else if (frac < 0.22) name = "waxing crescent";
  else if (frac < 0.28) name = "first quarter";
  else if (frac < 0.47) name = "waxing gibbous";
  else if (frac < 0.53) name = "full";
  else if (frac < 0.72) name = "waning gibbous";
  else if (frac < 0.78) name = "last quarter";
  else name = "waning crescent";

  return { frac, ageDays, illum, name };
}
// ==========================================

async function tick(){
  try{
    const [sum, tj, hj, astro] = await Promise.all([
      fetch('/api', {cache:'no-store'}).then(r=>r.json()),
      fetch('/api/temp', {cache:'no-store'}).then(r=>r.json()),
      fetch('/api/humidity', {cache:'no-store'}).then(r=>r.json()),
      fetch('/api/astro', {cache:'no-store'}).then(r=>r.json()),
    ]);

    if (sum.ok) {
      const t = sum.raw.temp_c;
      const rh = sum.raw.hum_pct;
      const dp = sum.derived.dew_point_c;
      const hi = sum.derived.heat_index_c;

      el('dp').textContent = isFinite(dp) ? dp.toFixed(1) : '--';
      el('hi').textContent = isFinite(hi) ? hi.toFixed(1) : '--';
      el('dp_rule').textContent = dpRule(dp);
      el('hi_rule').textContent = hiRule(hi, t);

      const ctag = comfortTag(dp, hi);
      el('comfort_tag').textContent = "comfort: " + ctag;
      el('mold_tag').textContent = moldTag(dp, rh);

      el('wb').textContent = isFinite(sum.derived.wet_bulb_c) ? (sum.derived.wet_bulb_c.toFixed(1) + " °C") : "--";
      el('ah').textContent = isFinite(sum.derived.abs_humidity_gm3) ? (sum.derived.abs_humidity_gm3.toFixed(1) + " g/m³") : "--";
      el('vpd').textContent = isFinite(sum.derived.vpd_kpa) ? (sum.derived.vpd_kpa.toFixed(3) + " kPa") : "--";
      el('storm').textContent = (typeof sum.derived.storm_score === "number") ? (sum.derived.storm_score + "/100") : "--";

      let note = "Temp " + t.toFixed(1) + " °C, RH " + rh.toFixed(1) + " %, DP " + dp.toFixed(1) + " °C.";
      if (ctag === "high stress") note += " Hydrate, airflow, reduce exertion.";
      else if (ctag === "uncomfortable") note += " Airflow helps a lot.";
      else if (ctag === "sticky") note += " Still air will feel gross.";
      else if (ctag === "comfortable") note += " Solid conditions.";
      else note += " Dry air feel.";
      el('comfort_note').textContent = note;
    }

    // Time tag + astronomy
    if (astro.ok && astro.time_valid && astro.utc_epoch > 0) {
      const localEpoch = astro.utc_epoch + astro.tz_offset_seconds;
      const d = new Date(localEpoch * 1000);
      el('time_tag').textContent = "Time: " + d.toLocaleString();

      const lat = astro.lat;
      const lon = astro.lon;

      const ss = sunriseSunset(astro.utc_epoch, lat, lon, astro.tz_offset_seconds);

      if (ss.polar === "day") {
        el('sunrise').textContent = "polar day";
        el('sunset').textContent  = "polar day";
        el('daylen').textContent  = "24:00";
      } else if (ss.polar === "night") {
        el('sunrise').textContent = "polar night";
        el('sunset').textContent  = "polar night";
        el('daylen').textContent  = "00:00";
      } else {
        el('sunrise').textContent = formatHM(ss.sunriseMin);
        el('sunset').textContent  = formatHM(ss.sunsetMin);
        const dayLenMin = ss.sunsetMin - ss.sunriseMin;
        const hrs = Math.floor(dayLenMin/60);
        const mins = Math.floor(dayLenMin%60);
        el('daylen').textContent = String(hrs).padStart(2,'0') + ":" + String(mins).padStart(2,'0');
      }

      const sp = solarPosition(astro.utc_epoch, lat, lon);
      el('elev').textContent = isFinite(sp.elevDeg) ? (sp.elevDeg.toFixed(1) + "°") : "--";
      el('az').textContent = isFinite(sp.azDeg) ? (sp.azDeg.toFixed(0) + "°") : "--";

      const mp = moonPhase(astro.utc_epoch);
      el('mphase').textContent = mp.name;
      el('millum').textContent = Math.round(mp.illum*100) + "%";

      let an = "";
      if (sp.elevDeg < -6) an = "night";
      else if (sp.elevDeg < 0) an = "civil twilight";
      else an = "daylight";
      el('astro_note').textContent = an;
    } else {
      el('time_tag').textContent = "Time: syncing...";
      el('astro_note').textContent = "NTP not locked";
    }

    // Build derived history series
    const ts = (tj.series || []);
    const hs = (hj.series || []);
    const n = Math.min(ts.length, hs.length);

    let dps = new Array(n);
    let his = new Array(n);

    for (let i=0;i<n;i++){
      const tv = ts[i], hv = hs[i];
      if (typeof tv === 'number' && isFinite(tv) && typeof hv === 'number' && isFinite(hv)) {
        dps[i] = dewPointC(tv, hv);
        his[i] = heatIndexC(tv, hv);
      } else {
        dps[i] = null;
        his[i] = null;
      }
    }

    drawSeries(ctxDP, cvDP, dps);
    drawSeries(ctxHI, cvHI, his);

  } catch(e) {
    // fail quietly
  }
}

tick();
setInterval(tick, 1000);
</script>
</body>
</html>
)HTML");
}
