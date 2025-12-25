#pragma once
#include <Arduino.h>

inline String pageDashboard() {
  String h;
  h.reserve(12000);

  h += "<!doctype html><html lang='en'><head>";
  h += "<meta charset='utf-8'/>";
  h += "<meta name='viewport' content='width=device-width,initial-scale=1'/>";
  h += "<link rel='icon' href='/favicon.svg' type='image/svg+xml'/>";
  h += "<title>UNO R4 Weather</title>";
  h += "<style>";
  h += "html,body{height:100%;margin:0;background:#070b14;color:#e6e9f2;font-family:ui-sans-serif,system-ui,-apple-system,Segoe UI,Roboto,Arial;}";
  h += ".wrap{max-width:1100px;margin:0 auto;padding:22px;}";
  h += ".top{display:flex;gap:14px;align-items:center;justify-content:space-between;flex-wrap:wrap;}";
  h += ".brand{display:flex;gap:12px;align-items:center;}";
  h += ".dot{width:14px;height:14px;border-radius:999px;background:linear-gradient(135deg,#7dd3fc,#a78bfa);box-shadow:0 0 20px rgba(167,139,250,.35);}";
  h += "h1{font-size:20px;margin:0;letter-spacing:.2px}";
  h += ".sub{color:#93a4c7;font-size:13px;margin-top:2px}";
  h += ".pill{padding:10px 12px;border:1px solid rgba(255,255,255,.08);border-radius:999px;background:rgba(255,255,255,.03);color:#cfe0ff;font-size:13px}";
  h += ".grid{display:grid;grid-template-columns:repeat(12,1fr);gap:14px;margin-top:16px}";
  h += ".card{grid-column:span 6;position:relative;padding:16px;border-radius:18px;background:linear-gradient(180deg,rgba(255,255,255,.06),rgba(255,255,255,.03));border:1px solid rgba(255,255,255,.08);box-shadow:0 12px 30px rgba(0,0,0,.35);cursor:pointer;transition:transform .12s ease, border-color .12s ease;}";
  h += ".card:hover{transform:translateY(-2px);border-color:rgba(125,211,252,.35)}";
  h += ".k{color:#9bb0d9;font-size:12px;letter-spacing:.25px;text-transform:uppercase}";
  h += ".v{font-size:34px;margin-top:6px;line-height:1.1}";
  h += ".u{font-size:14px;color:#a8b7db;margin-left:8px}";
  h += ".meta{margin-top:10px;color:#9bb0d9;font-size:13px;display:flex;gap:14px;flex-wrap:wrap}";
  h += ".meta b{color:#e6e9f2;font-weight:600}";
  h += ".wide{grid-column:span 12;cursor:default}";
  h += ".row{display:flex;gap:14px;flex-wrap:wrap;margin-top:10px}";
  h += ".chip{padding:10px 12px;border-radius:14px;border:1px solid rgba(255,255,255,.08);background:rgba(255,255,255,.03);}";
  h += ".chip .k{font-size:11px}";
  h += ".chip .v2{font-size:16px;margin-top:4px}";
  h += ".hint{color:#7f93bb;font-size:12px;margin-top:8px}";
  h += "@media (max-width:760px){.card{grid-column:span 12}.v{font-size:32px}}";
  h += "</style></head><body><div class='wrap'>";
  h += "<div class='top'><div class='brand'><div class='dot'></div><div>";
  h += "<h1>UNO R4 Weather Station</h1><div class='sub'>BME680 telemetry, moving averages, trends, graphs</div>";
  h += "</div></div><div class='pill' id='ip'>Loading…</div></div>";

  h += "<div class='grid'>";
  h += "<div class='card' onclick=\"location.href='/graph/temp'\">";
  h += "<div class='k'>Temperature</div><div class='v' id='temp'>--</div><div class='meta'><span>Avg 60s: <b id='avgTemp'>--</b></span><span>Dew point: <b id='dew'>--</b></span></div></div>";

  h += "<div class='card' onclick=\"location.href='/graph/humidity'\">";
  h += "<div class='k'>Humidity</div><div class='v' id='hum'>--</div><div class='meta'><span>Avg 60s: <b id='avgHum'>--</b></span><span>Heat index: <b id='heat'>--</b></span></div></div>";

  h += "<div class='card' onclick=\"location.href='/graph/pressure'\">";
  h += "<div class='k'>Sea-level Pressure</div><div class='v' id='sea'>--</div>";
  h += "<div class='meta'><span>Trend: <b id='trend'>--</b></span><span>Δ hPa/hr: <b id='dph'>--</b></span><span>Trend points: <b id='tp'>0.00</b></span></div></div>";

  h += "<div class='card' onclick=\"location.href='/graph/gas'\">";
  h += "<div class='k'>Gas Resistance</div><div class='v' id='gas'>--</div>";
  h += "<div class='meta'><span>Avg 90s: <b id='avgGas'>--</b></span><span>Storm risk: <b id='storm'>--</b></span></div></div>";

  h += "<div class='card wide'>";
  h += "<div class='k'>Quick links</div>";
  h += "<div class='row'>";
  h += "<div class='chip' style='cursor:pointer' onclick=\"location.href='/comfort'\"><div class='k'>Comfort</div><div class='v2'>Sun, moon, comfort stats</div></div>";
  h += "<div class='chip'><div class='k'>Altitude</div><div class='v2' id='alt'>--</div></div>";
  h += "<div class='chip'><div class='k'>Station pressure</div><div class='v2' id='press'>--</div></div>";
  h += "</div>";
  h += "<div class='hint'>Tip: click any big card to open its full graph page.</div>";
  h += "</div>";

  h += "</div>"; // grid
  h += "<script>";
  h += "function fmt(n,d){if(n===null||Number.isNaN(n))return'--';return Number(n).toFixed(d);}";

  h += "async function tick(){";
  h += "let r=await fetch('/api/latest',{cache:'no-store'});";
  h += "let j=await r.json();";
  h += "document.getElementById('ip').textContent='Live: '+(j.t?('t='+j.t):'no time');";
  h += "document.getElementById('temp').innerHTML=fmt(j.tempC,2)+\"<span class='u'>°C</span>\";";
  h += "document.getElementById('hum').innerHTML=fmt(j.hum,1)+\"<span class='u'>%</span>\";";
  h += "document.getElementById('sea').innerHTML=fmt(j.seaHpa,2)+\"<span class='u'>hPa</span>\";";
  h += "document.getElementById('gas').innerHTML=fmt(j.gasOhms,0)+\"<span class='u'>Ω</span>\";";
  h += "document.getElementById('press').textContent=fmt(j.pressHpa,2)+' hPa';";
  h += "document.getElementById('alt').textContent=fmt(j.altitudeM,0)+' m';";
  h += "document.getElementById('avgTemp').textContent=fmt(j.avgTemp60,2)+' °C';";
  h += "document.getElementById('avgHum').textContent=fmt(j.avgHum60,1)+' %';";
  h += "document.getElementById('avgGas').textContent=fmt(j.avgGas90,0)+' Ω';";
  h += "document.getElementById('dew').textContent=fmt(j.dewPointC,2)+' °C';";
  h += "document.getElementById('heat').textContent=fmt(j.heatIndexC,2)+' °C';";
  h += "document.getElementById('trend').textContent=j.trend||'--';";
  h += "document.getElementById('dph').textContent=fmt(j.deltaHpaPerHr,2);";
  h += "document.getElementById('tp').textContent=fmt(j.trendPoints,2);";
  h += "document.getElementById('storm').textContent=(j.stormRisk!==null?j.stormRisk:'--')+'%';";
  h += "}";
  h += "tick(); setInterval(tick,1000);";
  h += "</script>";

  h += "</div></body></html>";
  return h;
}
