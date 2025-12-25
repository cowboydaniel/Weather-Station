#pragma once
#include <Arduino.h>

inline String pageComfort() {
  String h;
  h.reserve(12000);

  h += "<!doctype html><html><head>";
  h += "<meta charset='utf-8'/><meta name='viewport' content='width=device-width,initial-scale=1'/>";
  h += "<link rel='icon' href='/favicon.svg' type='image/svg+xml'/>";
  h += "<title>Comfort</title>";
  h += "<style>";
  h += "html,body{height:100%;margin:0;background:#070b14;color:#e6e9f2;font-family:ui-sans-serif,system-ui,-apple-system,Segoe UI,Roboto,Arial;}";
  h += ".wrap{max-width:1100px;margin:0 auto;padding:22px}";
  h += ".top{display:flex;justify-content:space-between;align-items:center;gap:12px;flex-wrap:wrap}";
  h += "a{color:#9cd6ff;text-decoration:none} a:hover{text-decoration:underline}";
  h += ".grid{display:grid;grid-template-columns:repeat(12,1fr);gap:14px;margin-top:14px}";
  h += ".card{grid-column:span 6;padding:16px;border-radius:18px;background:rgba(255,255,255,.04);border:1px solid rgba(255,255,255,.08);box-shadow:0 12px 30px rgba(0,0,0,.35)}";
  h += ".wide{grid-column:span 12}";
  h += ".k{color:#9bb0d9;font-size:12px;text-transform:uppercase;letter-spacing:.25px}";
  h += ".v{font-size:22px;margin-top:6px}";
  h += ".row{margin-top:10px;color:#b7c6e8;font-size:14px;line-height:1.6}";
  h += ".row b{color:#e6e9f2}";
  h += "@media (max-width:760px){.card{grid-column:span 12}}";
  h += "</style></head><body><div class='wrap'>";
  h += "<div class='top'><div><div class='k'>Page</div><div class='v'>Comfort</div></div>";
  h += "<div><a href='/'>Dashboard</a></div></div>";

  h += "<div class='grid'>";
  h += "<div class='card'><div class='k'>Thermal</div><div class='v' id='thermal'>--</div>";
  h += "<div class='row'>Temp: <b id='t'>--</b><br/>Humidity: <b id='h'>--</b><br/>Dew point: <b id='dp'>--</b><br/>Heat index: <b id='hi'>--</b></div></div>";

  h += "<div class='card'><div class='k'>Pressure and Weather</div><div class='v' id='wx'>--</div>";
  h += "<div class='row'>Sea-level: <b id='sea'>--</b><br/>Trend: <b id='tr'>--</b><br/>Δ hPa/hr: <b id='dph'>--</b><br/>Storm risk: <b id='sr'>--</b></div></div>";

  h += "<div class='card wide'><div class='k'>Sun and Moon (offline calc)</div><div class='v' id='date'>--</div>";
  h += "<div class='row'>";
  h += "Sunrise: <b id='sunrise'>--</b> &nbsp; Sunset: <b id='sunset'>--</b> &nbsp; Solar noon: <b id='noon'>--</b><br/>";
  h += "Day length: <b id='daylen'>--</b><br/>";
  h += "Moon: <b id='moon'>--</b> (phase <b id='phase'>--</b>)";
  h += "</div></div>";

  h += "</div>"; // grid

  h += "<script>";
  h += "function fmt(n,d,suf){if(n===null||Number.isNaN(n))return'--';return Number(n).toFixed(d)+(suf||'');}";
  h += "async function tick(){";
  h += "let a=await fetch('/api/latest',{cache:'no-store'}); let j=await a.json();";
  h += "document.getElementById('t').textContent=fmt(j.tempC,2,' °C');";
  h += "document.getElementById('h').textContent=fmt(j.hum,1,' %');";
  h += "document.getElementById('dp').textContent=fmt(j.dewPointC,2,' °C');";
  h += "document.getElementById('hi').textContent=fmt(j.heatIndexC,2,' °C');";
  h += "document.getElementById('sea').textContent=fmt(j.seaHpa,2,' hPa');";
  h += "document.getElementById('tr').textContent=j.trend||'--';";
  h += "document.getElementById('dph').textContent=fmt(j.deltaHpaPerHr,2);";
  h += "document.getElementById('sr').textContent=(j.stormRisk!==null?j.stormRisk:'--')+'%';";

  h += "let b=await fetch('/api/comfort',{cache:'no-store'}); let c=await b.json();";
  h += "document.getElementById('date').textContent='Date: '+(c.date||'--')+' (Melbourne +11)';";
  h += "document.getElementById('sunrise').textContent=c.sunrise||'--';";
  h += "document.getElementById('sunset').textContent=c.sunset||'--';";
  h += "document.getElementById('noon').textContent=c.solarNoon||'--';";
  h += "document.getElementById('daylen').textContent=c.dayLength||'--';";
  h += "document.getElementById('moon').textContent=c.moonName||'--';";
  h += "document.getElementById('phase').textContent=(c.moonPhase01!==null?Number(c.moonPhase01).toFixed(2):'--');";

  // Nice friendly summary line
  h += "let thermal='';";
  h += "if(j.tempC!==null&&j.hum!==null){";
  h += "if(j.heatIndexC!==null&&j.heatIndexC>32) thermal='Hot';";
  h += "else if(j.tempC<10) thermal='Cold';";
  h += "else if(j.hum>75) thermal='Humid';";
  h += "else thermal='Comfortable';";
  h += "} else thermal='--';";
  h += "document.getElementById('thermal').textContent=thermal;";
  h += "let wx='';";
  h += "if(j.stormRisk!==null){";
  h += "if(j.stormRisk>70) wx='Stormy pattern likely';";
  h += "else if(j.stormRisk>40) wx='Unsettled';";
  h += "else wx='Stable';";
  h += "} else wx='--';";
  h += "document.getElementById('wx').textContent=wx;";
  h += "}";
  h += "tick(); setInterval(tick,2000);";
  h += "</script>";

  h += "</div></body></html>";
  return h;
}
