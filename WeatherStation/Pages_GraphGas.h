#pragma once
#include <Arduino.h>

inline String pageGraphGas() {
  String h;
  h.reserve(9000);

  h += "<!doctype html><html><head>";
  h += "<meta charset='utf-8'/><meta name='viewport' content='width=device-width,initial-scale=1'/>";
  h += "<link rel='icon' href='/favicon.svg' type='image/svg+xml'/>";
  h += "<title>Gas Graph</title>";
  h += "<style>";
  h += "html,body{height:100%;margin:0;background:#070b14;color:#e6e9f2;font-family:ui-sans-serif,system-ui,-apple-system,Segoe UI,Roboto,Arial;}";
  h += ".wrap{max-width:1100px;margin:0 auto;padding:22px}";
  h += ".top{display:flex;justify-content:space-between;align-items:center;gap:12px;flex-wrap:wrap}";
  h += "a{color:#9cd6ff;text-decoration:none} a:hover{text-decoration:underline}";
  h += ".card{margin-top:14px;border-radius:18px;background:rgba(255,255,255,.04);border:1px solid rgba(255,255,255,.08);box-shadow:0 12px 30px rgba(0,0,0,.35);padding:14px}";
  h += "canvas{width:100%;height:420px;display:block}";
  h += ".k{color:#9bb0d9;font-size:12px;text-transform:uppercase;letter-spacing:.25px}";
  h += ".v{font-size:18px;margin-top:6px}";
  h += ".note{color:#94a6ca;font-size:12px;margin-top:10px;line-height:1.4}";
  h += "</style></head><body><div class='wrap'>";
  h += "<div class='top'><div><div class='k'>Graph</div><div class='v'>Gas Resistance (Ω)</div></div>";
  h += "<div><a href='/'>Dashboard</a></div></div>";
  h += "<div class='card'><canvas id='c' width='1200' height='520'></canvas>";
  h += "<div class='note'>This is raw resistance, sampled every 3 seconds. For “AQI/IAQ” you normally need Bosch BSEC, but resistance trend still gives useful signals.</div>";
  h += "</div>";
  h += "<script>";
  h += "const c=document.getElementById('c'),ctx=c.getContext('2d');";
  h += "function draw(points){ctx.clearRect(0,0,c.width,c.height);const pad=50,w=c.width,h=c.height;";
  h += "const xs=points.map(p=>p.t), ys=points.map(p=>p.v).filter(v=>v!==null);";
  h += "if(points.length<2||ys.length<2){ctx.fillText('No data yet',pad,pad);return;}";
  h += "let xmin=Math.min(...xs), xmax=Math.max(...xs); let ymin=Math.min(...ys), ymax=Math.max(...ys);";
  h += "if(ymax-ymin<10){ymax=ymin+10}";
  h += "function X(t){return pad+(t-xmin)/(xmax-xmin)*(w-2*pad)}";
  h += "function Y(v){return h-pad-(v-ymin)/(ymax-ymin)*(h-2*pad)}";
  h += "ctx.strokeStyle='rgba(255,255,255,.12)'; ctx.lineWidth=1;";
  h += "for(let i=0;i<6;i++){let y=pad+i*(h-2*pad)/5;ctx.beginPath();ctx.moveTo(pad,y);ctx.lineTo(w-pad,y);ctx.stroke();}";
  h += "ctx.strokeStyle='rgba(125,211,252,.8)'; ctx.lineWidth=2; ctx.beginPath();";
  h += "let started=false; for(const p of points){if(p.v===null) continue; let x=X(p.t), y=Y(p.v); if(!started){ctx.moveTo(x,y);started=true;} else ctx.lineTo(x,y);} ctx.stroke();";
  h += "ctx.fillStyle='rgba(230,233,242,.9)'; ctx.font='14px ui-sans-serif,system-ui';";
  h += "ctx.fillText('min '+ymin.toFixed(0)+' Ω', pad, h-pad+24);";
  h += "ctx.fillText('max '+ymax.toFixed(0)+' Ω', pad+240, h-pad+24);";
  h += "}";
  h += "async function tick(){let r=await fetch('/api/history?m=gas',{cache:'no-store'}); let j=await r.json(); draw(j);} ";
  h += "tick(); setInterval(tick,2000);";
  h += "</script></div></body></html>";
  return h;
}
