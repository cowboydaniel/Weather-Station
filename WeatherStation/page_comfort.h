#pragma once
#include <WiFiS3.h>

static void sendPageComfort(WiFiClient &client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html; charset=utf-8");
  client.println("Cache-Control: public, max-age=300");
  client.println("Connection: close");
  client.println();

  client.println(R"HTML(
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Comfort</title>

<link rel="icon" href="/static/favicon.svg">
<link rel="stylesheet" href="/static/app.css">
<script src="/static/app.js"></script>
</head>

<body class="page-comfort" style="--bg-layer: radial-gradient(1200px 700px at 20% 10%, rgba(120,255,200,0.20), transparent 60%), radial-gradient(900px 600px at 80% 25%, rgba(110,160,255,0.14), transparent 55%), radial-gradient(800px 800px at 60% 115%, rgba(255,110,180,0.10), transparent 60%), var(--bg); --wrap-max:1020px; --wrap-pad:24px 16px 44px;">
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
        <canvas id="cv_dp" class="chart chart-sm"></canvas>
      </div>
      <div class="note">Derived in-browser from your temp and humidity history.</div>
    </div>

    <div class="card wide">
      <div class="label">Heat index history</div>
      <div style="margin-top:12px">
        <canvas id="cv_hi" class="chart chart-sm"></canvas>
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

const dpChart = setupCanvas('cv_dp', 220);
const hiChart = setupCanvas('cv_hi', 220);

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

function renderSeries(chart, series){
  drawLineSeries(chart.ctx, chart.cv, series, {padFraction:0.12, minPad:0.2});
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

// Cached derived series for incremental updates
let cachedDps = [];
let cachedHis = [];
let lastSeriesCount = 0;

// Compute wet bulb temperature (Stull approximation)
function wetBulbC(tC, rh) {
  if (!isFinite(tC) || !isFinite(rh)) return NaN;
  return tC * Math.atan(0.151977 * Math.sqrt(rh + 8.313659)) +
         Math.atan(tC + rh) - Math.atan(rh - 1.676331) +
         0.00391838 * Math.pow(rh, 1.5) * Math.atan(0.023101 * rh) - 4.686035;
}

// Compute absolute humidity (g/m³)
function absHumidity(tC, rh) {
  if (!isFinite(tC) || !isFinite(rh)) return NaN;
  const es = 6.112 * Math.exp((17.67 * tC) / (tC + 243.5));
  const e = (rh / 100.0) * es;
  return (e * 2.1674) / (273.15 + tC);
}

// Compute VPD (kPa)
function vpdKpa(tC, rh) {
  if (!isFinite(tC) || !isFinite(rh)) return NaN;
  const es = 0.6108 * Math.exp((17.27 * tC) / (tC + 237.3));
  const ea = (rh / 100.0) * es;
  return es - ea;
}

async function tick(){
  try{
    // Single combined API call instead of 4 separate calls
    const j = await fetch('/api/comfort', {cache:'no-store'}).then(r=>r.json());
    if (!j.ok) return;

    const t = j.raw.temp_c;
    const rh = j.raw.hum_pct;
    const dp = j.derived.dew_point_c;
    const hi = j.derived.heat_index_c;

    el('dp').textContent = isFinite(dp) ? dp.toFixed(1) : '--';
    el('hi').textContent = isFinite(hi) ? hi.toFixed(1) : '--';
    el('dp_rule').textContent = dpRule(dp);
    el('hi_rule').textContent = hiRule(hi, t);

    const ctag = comfortTag(dp, hi);
    el('comfort_tag').textContent = "comfort: " + ctag;
    el('mold_tag').textContent = moldTag(dp, rh);

    // Compute additional comfort metrics locally
    const wb = wetBulbC(t, rh);
    const ah = absHumidity(t, rh);
    const vpd = vpdKpa(t, rh);
    el('wb').textContent = isFinite(wb) ? (wb.toFixed(1) + " °C") : "--";
    el('ah').textContent = isFinite(ah) ? (ah.toFixed(1) + " g/m³") : "--";
    el('vpd').textContent = isFinite(vpd) ? (vpd.toFixed(3) + " kPa") : "--";
    el('storm').textContent = (typeof j.derived.storm_score === "number") ? (j.derived.storm_score + "/100") : "--";

    let note = "Temp " + t.toFixed(1) + " °C, RH " + rh.toFixed(1) + " %, DP " + dp.toFixed(1) + " °C.";
    if (ctag === "high stress") note += " Hydrate, airflow, reduce exertion.";
    else if (ctag === "uncomfortable") note += " Airflow helps a lot.";
    else if (ctag === "sticky") note += " Still air will feel gross.";
    else if (ctag === "comfortable") note += " Solid conditions.";
    else note += " Dry air feel.";
    el('comfort_note').textContent = note;

    // Time tag + astronomy (use browser's local time)
    const now = Date.now() / 1000;
    const tzOffset = -new Date().getTimezoneOffset() * 60;
    el('time_tag').textContent = "Time: " + new Date().toLocaleString();

    // Use hardcoded lat/lon (same as Arduino would have)
    const lat = -33.85;  // Sydney, adjust as needed
    const lon = 151.21;

    const ss = sunriseSunset(now, lat, lon, tzOffset);
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

    const sp = solarPosition(now, lat, lon);
    el('elev').textContent = isFinite(sp.elevDeg) ? (sp.elevDeg.toFixed(1) + "°") : "--";
    el('az').textContent = isFinite(sp.azDeg) ? (sp.azDeg.toFixed(0) + "°") : "--";

    const mp = moonPhase(now);
    el('mphase').textContent = mp.name;
    el('millum').textContent = Math.round(mp.illum*100) + "%";

    let an = "";
    if (sp.elevDeg < -6) an = "night";
    else if (sp.elevDeg < 0) an = "civil twilight";
    else an = "daylight";
    el('astro_note').textContent = an;

    // Incremental derived history series update
    const ts = j.temp_series || [];
    const hs = j.hum_series || [];
    const n = Math.min(ts.length, hs.length);
    const serverCount = j.series_count || n;

    // If series was reset or this is first load, rebuild entirely
    if (serverCount < lastSeriesCount || cachedDps.length === 0) {
      cachedDps = new Array(n);
      cachedHis = new Array(n);
      for (let i = 0; i < n; i++) {
        const tv = ts[i], hv = hs[i];
        if (typeof tv === 'number' && isFinite(tv) && typeof hv === 'number' && isFinite(hv)) {
          cachedDps[i] = dewPointC(tv, hv);
          cachedHis[i] = heatIndexC(tv, hv);
        } else {
          cachedDps[i] = null;
          cachedHis[i] = null;
        }
      }
    } else {
      // Incremental update: only compute new values
      const newCount = serverCount - lastSeriesCount;
      if (newCount > 0 && newCount < n) {
        // Shift existing values and add new ones
        cachedDps = cachedDps.slice(newCount);
        cachedHis = cachedHis.slice(newCount);
        for (let i = n - newCount; i < n; i++) {
          const tv = ts[i], hv = hs[i];
          if (typeof tv === 'number' && isFinite(tv) && typeof hv === 'number' && isFinite(hv)) {
            cachedDps.push(dewPointC(tv, hv));
            cachedHis.push(heatIndexC(tv, hv));
          } else {
            cachedDps.push(null);
            cachedHis.push(null);
          }
        }
      }
    }
    lastSeriesCount = serverCount;

    renderSeries(dpChart, cachedDps);
    renderSeries(hiChart, cachedHis);

  } catch(e) {
    // fail quietly
  }
}

tick();
setInterval(tick, 2000);  // Reduced from 1s to 2s
</script>
</body>
</html>
)HTML");
}
