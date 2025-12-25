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

<link rel="icon" href="/static/favicon.svg">
<link rel="stylesheet" href="/static/app.css">
<script src="/static/app.js"></script>
</head>

<body class="page-index" style="--wrap-max:1080px; --wrap-pad:28px 16px 44px; --bg-layer: radial-gradient(1200px 700px at 15% 10%, rgba(110,160,255,0.22), transparent 60%), radial-gradient(900px 600px at 85% 25%, rgba(255,110,180,0.18), transparent 55%), radial-gradient(1000px 900px at 55% 110%, rgba(120,255,200,0.12), transparent 60%), radial-gradient(600px 600px at 55% 30%, rgba(255,255,255,0.06), transparent 50%), var(--bg);">
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

    <a class="card" href="/derived">
      <div class="label">Derived Metrics</div>
      <div class="big"><span id="forecast">--</span></div>
      <div class="pair"><span>Air quality</span><b id="aqi">--</b></div>
      <div class="pair"><span>Frost risk</span><b id="frost">--</b></div>
      <div class="footer">AQI, cloud base, frost, humidex, GDD, forecast.</div>
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

// Derived metrics helpers
function gasToAQI(g) {
  if (!isFinite(g) || g <= 0) return NaN;
  if (g > 400) return Math.max(0, 50 - (g - 400) * 0.1);
  if (g > 200) return 50 + (400 - g) * 0.25;
  if (g > 100) return 100 + (200 - g) * 0.5;
  if (g > 50) return 150 + (100 - g) * 1;
  return Math.min(500, 200 + (50 - g) * 6);
}

function aqiLabel(a) {
  if (!isFinite(a)) return '--';
  if (a <= 50) return 'Good';
  if (a <= 100) return 'OK';
  if (a <= 150) return 'Sens.';
  return 'Poor';
}

function frostRisk(t, dp) {
  if (!isFinite(t) || !isFinite(dp)) return '--';
  const ground = t - 3;
  if (ground <= 0) return 'Now';
  if (t <= 4 && dp <= 2) return 'High';
  if (t <= 6 && dp <= 4) return 'Mod';
  if (t <= 10 && dp <= 6) return 'Low';
  return 'None';
}

function quickForecast(slp, tend) {
  if (!isFinite(slp) || !isFinite(tend)) return '--';
  if (tend < -3) return 'Stormy';
  if (tend < -1.5) return 'Rain likely';
  if (tend < -0.5) return 'Unsettled';
  if (tend > 3) return 'Clearing';
  if (tend > 1.5) return 'Improving';
  if (tend > 0.5) return 'Fair';
  if (slp > 1020) return 'Settled';
  if (slp > 1010) return 'Stable';
  return 'Cloudy';
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

    // Derived metrics card
    const aqi = gasToAQI(j.raw.gas_kohm);
    el('forecast').textContent = quickForecast(j.derived.slp_hpa, j.derived.press_tendency_hpa_hr);
    el('aqi').textContent = isFinite(aqi) ? Math.round(aqi) + ' ' + aqiLabel(aqi) : '--';
    el('frost').textContent = frostRisk(j.raw.temp_c, j.derived.dew_point_c);

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
