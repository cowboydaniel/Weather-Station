#pragma once
#include <WiFiS3.h>

static void sendPageDerived(WiFiClient &client) {
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
<title>Derived Metrics</title>

<link rel="icon" href="/static/favicon.svg">
<link rel="stylesheet" href="/static/app.css">
<script src="/static/app.js"></script>
</head>

<body class="page-derived" style="--bg-layer: radial-gradient(1200px 700px at 20% 10%, rgba(255,180,100,0.18), transparent 60%), radial-gradient(900px 600px at 80% 25%, rgba(255,140,0,0.14), transparent 55%), radial-gradient(800px 800px at 60% 115%, rgba(110,160,255,0.10), transparent 60%), var(--bg); --wrap-max:1020px; --wrap-pad:24px 16px 44px;">
<div class="wrap">
  <div class="top">
    <div>
      <h1>Derived Metrics</h1>
      <div class="sub">Advanced calculations from temperature, humidity, and pressure data.</div>
    </div>
    <a class="back" href="/">Back</a>
  </div>

  <div class="grid">
    <div class="card">
      <div class="label">Air Quality Index</div>
      <div class="big"><span id="aqi">--</span></div>
      <div class="row"><span>Category</span><b id="aqi-cat">--</b></div>
      <div class="row"><span>Gas resistance</span><b id="gas-raw">-- kOhm</b></div>
      <div class="note">Estimated from gas resistance. Higher resistance = cleaner air. This is a relative indicator, not EPA AQI.</div>
    </div>

    <div class="card">
      <div class="label">Cloud Base</div>
      <div class="big"><span id="cloud-base">--</span><span class="unit">m</span></div>
      <div class="row"><span>Spread (T - DP)</span><b id="spread">-- °C</b></div>
      <div class="row"><span>Conditions</span><b id="cloud-cond">--</b></div>
      <div class="note">Estimated height where clouds may form. Uses the 125m per °C spread rule.</div>
    </div>

    <div class="card">
      <div class="label">Frost Risk</div>
      <div class="big"><span id="frost-tag" class="tag">--</span></div>
      <div class="row"><span>Ground temp estimate</span><b id="ground-temp">-- °C</b></div>
      <div class="row"><span>Hours to frost</span><b id="frost-hours">--</b></div>
      <div class="note">Based on dew point, temperature, and humidity. Ground is typically 2-4°C cooler than air at night.</div>
    </div>

    <div class="card">
      <div class="label">Humidex</div>
      <div class="big"><span id="humidex">--</span><span class="unit">°C</span></div>
      <div class="row"><span>Comfort</span><b id="humidex-cat">--</b></div>
      <div class="row"><span>Vs actual temp</span><b id="humidex-diff">--</b></div>
      <div class="note">Canadian heat-humidity index. Differs from heat index formula.</div>
    </div>

    <div class="card wide">
      <div class="label">Zambretti Weather Forecast</div>
      <div class="forecast-box">
        <div class="forecast-text" id="forecast">--</div>
        <div class="forecast-detail" id="forecast-detail">Analyzing pressure trends...</div>
      </div>
      <div class="kpi-grid">
        <div><div class="k">Pressure trend</div><div class="v" id="trend-dir">--</div></div>
        <div><div class="k">Change rate</div><div class="v" id="trend-rate">--</div></div>
        <div><div class="k">Sea-level pressure</div><div class="v" id="press-slp">--</div></div>
        <div><div class="k">Confidence</div><div class="v" id="forecast-conf">--</div></div>
      </div>
      <div class="note">Classic Zambretti algorithm (1915) adapted for digital use. Based on barometric pressure, trend, and season.</div>
    </div>

    <div class="card">
      <div class="label">Thermal Comfort (ASHRAE)</div>
      <div style="margin-top:10px; display:flex; flex-wrap:wrap; gap:8px;">
        <span class="tag" id="ashrae-tag">--</span>
        <span class="tag" id="pmv-tag">PMV: --</span>
      </div>
      <div class="meter" style="margin-top:14px; background: linear-gradient(90deg, #3b82f6, #22c55e, #22c55e, #eab308, #ef4444);">
        <div class="meter-marker" id="pmv-marker" style="left:50%;"></div>
      </div>
      <div style="display:flex; justify-content:space-between; font-size:10px; color:var(--muted); margin-top:4px;">
        <span>Cold</span><span>Cool</span><span>Neutral</span><span>Warm</span><span>Hot</span>
      </div>
      <div class="note">ASHRAE 55 thermal comfort. PMV scale: -3 (cold) to +3 (hot), 0 is ideal.</div>
    </div>

    <div class="card">
      <div class="label">Growing Degree Days</div>
      <div class="big"><span id="gdd-today">--</span><span class="unit">GDD</span></div>
      <div class="row"><span>Base temp</span><b>10°C</b></div>
      <div class="row"><span>Current contribution</span><b id="gdd-rate">--</b></div>
      <div style="margin-top:12px;">
        <div class="k" style="font-size:11px; color:rgba(255,255,255,0.55);">Plant growth potential</div>
        <div style="margin-top:6px; display:flex; gap:6px; flex-wrap:wrap;">
          <span class="tag" id="plant-tag">--</span>
        </div>
      </div>
      <div class="note">Accumulated heat units for plant growth. Base 10°C is common for many crops.</div>
    </div>

    <div class="card wide">
      <div class="label">Comfort Summary</div>
      <div class="kpi-grid">
        <div><div class="k">Overall</div><div class="v" id="comfort-overall">--</div></div>
        <div><div class="k">Outdoor activity</div><div class="v" id="outdoor-rec">--</div></div>
        <div><div class="k">Sleep quality</div><div class="v" id="sleep-qual">--</div></div>
        <div><div class="k">Respiratory</div><div class="v" id="resp-comfort">--</div></div>
      </div>
      <div class="note" id="comfort-advice">--</div>
    </div>

    <div class="card">
      <div class="label">Mold Risk Index</div>
      <div class="big"><span id="mold-tag" class="tag">--</span></div>
      <div class="meter" style="margin-top:14px;">
        <div class="meter-fill" id="mold-fill" style="width:0%; background: linear-gradient(90deg, #22c55e, #eab308, #ef4444);"></div>
      </div>
      <div class="row"><span>Risk score</span><b id="mold-score">-- %</b></div>
      <div class="row"><span>Humidity</span><b id="mold-rh">-- %</b></div>
      <div class="note">Based on temperature and humidity. Mold thrives at 20-30°C with RH > 70%. Monitor sustained high readings.</div>
    </div>

    <div class="card">
      <div class="label">Air Density</div>
      <div class="big"><span id="air-density">--</span><span class="unit">kg/m³</span></div>
      <div class="row"><span>Pressure altitude</span><b id="press-alt">-- ft</b></div>
      <div class="row"><span>Density altitude</span><b id="density-alt">-- ft</b></div>
      <div class="note">Air density affects combustion, lift, and athletic performance. Density altitude is key for aviation.</div>
    </div>

    <div class="card">
      <div class="label">Thom Discomfort Index</div>
      <div class="big"><span id="thom-idx">--</span><span class="unit">°C</span></div>
      <div class="row"><span>Comfort level</span><b id="thom-cat">--</b></div>
      <div class="row"><span>Vs heat index</span><b id="thom-vs-hi">--</b></div>
      <div class="note">Simple thermal discomfort indicator. More straightforward than heat index for general comfort assessment.</div>
    </div>

    <div class="card">
      <div class="label">Air Enthalpy</div>
      <div class="big"><span id="enthalpy">--</span><span class="unit">kJ/kg</span></div>
      <div class="row"><span>HVAC load indicator</span><b id="hvac-load">--</b></div>
      <div class="row"><span>Energy benchmark</span><b id="enthalpy-bench">--</b></div>
      <div class="note">Total heat content of air. Used in HVAC to calculate cooling/heating loads. Ideal indoor: 40-60 kJ/kg.</div>
    </div>

    <div class="card wide">
      <div class="label">Degree Days (Energy)</div>
      <div class="kpi-grid">
        <div><div class="k">Heating DD</div><div class="v" id="hdd">--</div></div>
        <div><div class="k">Cooling DD</div><div class="v" id="cdd">--</div></div>
        <div><div class="k">Base temp</div><div class="v">18°C</div></div>
        <div><div class="k">Energy mode</div><div class="v" id="energy-mode">--</div></div>
      </div>
      <div class="meter" style="margin-top:14px; background: linear-gradient(90deg, #3b82f6 0%, #3b82f6 50%, #ef4444 50%, #ef4444 100%);">
        <div class="meter-marker" id="dd-marker" style="left:50%;"></div>
      </div>
      <div style="display:flex; justify-content:space-between; font-size:10px; color:var(--muted); margin-top:4px;">
        <span>Heating needed</span><span>Comfort zone</span><span>Cooling needed</span>
      </div>
      <div class="note">Degree days measure energy demand. HDD indicates heating needs, CDD indicates cooling needs. Base 18°C is standard.</div>
    </div>
  </div>
</div>

<script>
const el = (id) => document.getElementById(id);

// Dew point calculation (Magnus formula)
function dewPointC(tC, rh) {
  if (!isFinite(tC) || !isFinite(rh)) return NaN;
  rh = Math.max(1, Math.min(rh, 100));
  const a = 17.62, b = 243.12;
  const gamma = (a * tC) / (b + tC) + Math.log(rh / 100.0);
  return (b * gamma) / (a - gamma);
}

// Humidex (Canadian formula)
function humidex(tC, rh) {
  if (!isFinite(tC) || !isFinite(rh)) return NaN;
  const dp = dewPointC(tC, rh);
  const e = 6.11 * Math.exp(5417.7530 * (1/273.16 - 1/(273.15 + dp)));
  return tC + 0.5555 * (e - 10);
}

function humidexCategory(h) {
  if (!isFinite(h)) return "--";
  if (h < 30) return "comfortable";
  if (h < 35) return "slight discomfort";
  if (h < 40) return "discomfort";
  if (h < 45) return "great discomfort";
  if (h < 54) return "dangerous";
  return "heat stroke risk";
}

// Air Quality Index from gas resistance (relative scale)
function gasToAQI(gasKohm) {
  if (!isFinite(gasKohm) || gasKohm <= 0) return NaN;
  // Higher resistance = cleaner air
  // Typical indoor: 50-500 kOhm, clean outdoor: 100-500+ kOhm
  // Map to 0-500 AQI scale (inverted - high resistance = low AQI = good)
  if (gasKohm > 400) return Math.max(0, 50 - (gasKohm - 400) * 0.1);
  if (gasKohm > 200) return 50 + (400 - gasKohm) * 0.25;
  if (gasKohm > 100) return 100 + (200 - gasKohm) * 0.5;
  if (gasKohm > 50) return 150 + (100 - gasKohm) * 1;
  return Math.min(500, 200 + (50 - gasKohm) * 6);
}

function aqiCategory(aqi) {
  if (!isFinite(aqi)) return "--";
  if (aqi <= 50) return "Good";
  if (aqi <= 100) return "Moderate";
  if (aqi <= 150) return "Sensitive groups";
  if (aqi <= 200) return "Unhealthy";
  if (aqi <= 300) return "Very unhealthy";
  return "Hazardous";
}

// Cloud base estimation (meters)
function cloudBase(tC, dp) {
  if (!isFinite(tC) || !isFinite(dp)) return NaN;
  const spread = tC - dp;
  if (spread < 0) return 0; // Already at dew point
  return spread * 125; // ~125m per degree C spread
}

function cloudCondition(spread) {
  if (!isFinite(spread)) return "--";
  if (spread < 2) return "foggy/misty";
  if (spread < 5) return "low clouds likely";
  if (spread < 10) return "some clouds";
  if (spread < 15) return "fair weather";
  return "clear skies";
}

// Frost risk assessment
function frostRisk(tC, dp, rh) {
  if (!isFinite(tC) || !isFinite(dp)) return { level: "--", tag: "" };

  // Ground is typically 2-4°C cooler than air at night
  const groundTemp = tC - 3;

  if (groundTemp <= 0) return { level: "Frost now", tag: "red", groundTemp, hours: 0 };
  if (tC <= 4 && dp <= 2) return { level: "High risk", tag: "red", groundTemp, hours: 2 };
  if (tC <= 6 && dp <= 4) return { level: "Moderate", tag: "yellow", groundTemp, hours: 4 };
  if (tC <= 10 && dp <= 6) return { level: "Low risk", tag: "blue", groundTemp, hours: 8 };
  return { level: "Unlikely", tag: "green", groundTemp, hours: null };
}

// Weather forecast from pressure trend
function weatherForecast(slp, tendHpaHr, rh) {
  if (!isFinite(slp) || !isFinite(tendHpaHr)) {
    return { text: "Insufficient data", detail: "Need more pressure history", confidence: "Low" };
  }

  let forecast = "";
  let detail = "";
  let confidence = "Medium";

  // Rapid changes = more confidence in short-term forecast
  const absChange = Math.abs(tendHpaHr);
  if (absChange > 3) confidence = "High";
  else if (absChange < 1) confidence = "Low";

  if (tendHpaHr < -3) {
    forecast = "Stormy";
    detail = "Rapid pressure drop. Rain/storms likely within hours.";
  } else if (tendHpaHr < -1.5) {
    forecast = "Deteriorating";
    detail = "Falling pressure suggests approaching rain or unsettled weather.";
  } else if (tendHpaHr < -0.5) {
    forecast = "Becoming unsettled";
    detail = "Slow pressure drop. Clouds and possible rain developing.";
  } else if (tendHpaHr > 3) {
    forecast = "Rapidly improving";
    detail = "Strong pressure rise. Clearing skies expected soon.";
  } else if (tendHpaHr > 1.5) {
    forecast = "Improving";
    detail = "Rising pressure indicates clearing and fair weather ahead.";
  } else if (tendHpaHr > 0.5) {
    forecast = "Slowly improving";
    detail = "Gradual pressure rise. Conditions stabilizing.";
  } else {
    // Steady pressure - use absolute value
    if (slp > 1020) {
      forecast = "Fair & settled";
      detail = "High pressure dominates. Continued fine weather.";
    } else if (slp > 1010) {
      forecast = "Stable";
      detail = "Average pressure. No significant changes expected.";
    } else {
      forecast = "Unsettled";
      detail = "Low pressure. Cloudy with possible showers.";
    }
  }

  // Humidity modifier
  if (rh > 85 && tendHpaHr < 0) {
    detail += " High humidity increases rain probability.";
  }

  return { text: forecast, detail, confidence };
}

// ASHRAE PMV (simplified) - Predicted Mean Vote
function calculatePMV(tC, rh) {
  if (!isFinite(tC) || !isFinite(rh)) return NaN;

  // Simplified PMV based on temp and humidity
  // Assumes: light clothing (0.5 clo), sedentary activity (1.0 met),
  // air velocity 0.1 m/s, radiant temp = air temp

  // Optimal: 22-24°C at 50% RH
  let pmv = (tC - 23) * 0.3;

  // Humidity adjustment
  if (rh > 60) pmv += (rh - 60) * 0.02;
  if (rh < 40) pmv -= (40 - rh) * 0.01;

  return Math.max(-3, Math.min(3, pmv));
}

function pmvCategory(pmv) {
  if (!isFinite(pmv)) return "--";
  if (pmv < -2.5) return "Cold";
  if (pmv < -1.5) return "Cool";
  if (pmv < -0.5) return "Slightly cool";
  if (pmv < 0.5) return "Neutral";
  if (pmv < 1.5) return "Slightly warm";
  if (pmv < 2.5) return "Warm";
  return "Hot";
}

// Growing Degree Days
function calculateGDD(tC, baseTemp = 10) {
  if (!isFinite(tC)) return 0;
  return Math.max(0, tC - baseTemp);
}

function plantGrowthPotential(gdd) {
  if (!isFinite(gdd) || gdd <= 0) return "dormant";
  if (gdd < 5) return "minimal";
  if (gdd < 10) return "slow growth";
  if (gdd < 15) return "moderate";
  if (gdd < 20) return "active growth";
  return "rapid growth";
}

// Zambretti weather forecast algorithm
// Classic barometric forecaster from 1915, adapted for digital use
function zambrettiForecast(slp, tendHpaHr, month) {
  if (!isFinite(slp) || !isFinite(tendHpaHr)) {
    return { text: "Insufficient data", detail: "Need pressure data", z: null, confidence: "Low" };
  }

  // Normalize pressure to Zambretti scale (950-1050 hPa -> 0-32)
  const pNorm = Math.max(0, Math.min(32, Math.round((slp - 950) * 0.32)));

  // Determine trend: rising, falling, or steady
  const trend = tendHpaHr > 0.5 ? 'rising' : tendHpaHr < -0.5 ? 'falling' : 'steady';

  // Season adjustment (Southern hemisphere - swap if needed)
  const isSummer = (month >= 11 || month <= 2); // Nov-Feb for southern hemisphere
  const seasonAdj = isSummer ? 1 : -1;

  // Zambretti lookup tables
  const risingForecast = [
    "Settled fine", "Fine weather", "Becoming fine", "Fine, becoming less settled",
    "Fine, possible showers", "Fairly fine, improving", "Fairly fine, possible showers early",
    "Fairly fine, showery later", "Showery early, improving", "Changeable, mending",
    "Fairly fine, showers likely", "Rather unsettled clearing later", "Unsettled, probably improving",
    "Showery, bright intervals", "Showery, becoming more unsettled", "Changeable, some rain",
    "Unsettled, short fine intervals", "Unsettled, rain later", "Unsettled, rain at times",
    "Very unsettled, finer at times", "Rain at times, worse later", "Rain at times, becoming very unsettled",
    "Rain at frequent intervals", "Very unsettled, rain", "Stormy, possibly improving",
    "Stormy, much rain"
  ];

  const fallingForecast = [
    "Settled fine", "Fine weather", "Fine, becoming less settled", "Fairly fine, showery later",
    "Showery, bright intervals", "Changeable, some rain", "Unsettled, rain at times",
    "Rain at frequent intervals", "Very unsettled, rain", "Stormy, much rain",
    "Stormy, possibly improving", "Stormy"
  ];

  const steadyForecast = [
    "Settled fine", "Fine weather", "Fine, possibly showers", "Fairly fine, showers likely",
    "Showery, bright intervals", "Changeable, light rain", "Unsettled, light rain",
    "Rain at times", "Very unsettled, rain", "Stormy"
  ];

  let zNum, forecast;

  if (trend === 'rising') {
    // Rising: higher pressure = better weather
    zNum = Math.min(25, Math.max(0, Math.round(pNorm * 0.8) + seasonAdj));
    forecast = risingForecast[zNum] || risingForecast[0];
  } else if (trend === 'falling') {
    // Falling: lower pressure = worse weather
    zNum = Math.min(11, Math.max(0, Math.round((32 - pNorm) * 0.35) - seasonAdj));
    forecast = fallingForecast[zNum] || fallingForecast[0];
  } else {
    // Steady: based on absolute pressure
    zNum = Math.min(9, Math.max(0, Math.round((32 - pNorm) * 0.3)));
    forecast = steadyForecast[zNum] || steadyForecast[0];
  }

  // Confidence based on trend strength and pressure stability
  let confidence = "Medium";
  const absChange = Math.abs(tendHpaHr);
  if (absChange > 2) confidence = "High";
  else if (absChange < 0.3) confidence = "Low";

  // Detail based on pressure
  let detail = "";
  if (slp > 1025) detail = "High pressure system dominant.";
  else if (slp > 1015) detail = "Normal pressure range.";
  else if (slp > 1005) detail = "Slightly low pressure.";
  else detail = "Low pressure system present.";

  return { text: forecast, detail, z: zNum, trend, confidence };
}

// Mold Risk Index (based on sustained humidity and temperature)
function moldRisk(tC, rh) {
  if (!isFinite(tC) || !isFinite(rh)) return { level: "--", risk: 0, tag: "" };

  // Mold grows best at 25-30°C and RH > 70%
  // Minimal growth below 5°C or above 40°C
  // Critical threshold is typically 65-70% RH sustained

  let risk = 0;

  // Temperature factor (0-1)
  let tempFactor = 0;
  if (tC >= 15 && tC <= 35) {
    tempFactor = 1 - Math.abs(tC - 25) / 15; // Peak at 25°C
  } else if (tC >= 5 && tC < 15) {
    tempFactor = (tC - 5) / 20;
  } else if (tC > 35 && tC <= 40) {
    tempFactor = (40 - tC) / 10;
  }

  // Humidity factor
  if (rh > 70) {
    risk = tempFactor * ((rh - 60) / 40) * 100;
  } else if (rh > 60) {
    risk = tempFactor * ((rh - 60) / 40) * 50;
  }

  risk = Math.min(100, Math.max(0, risk));

  let level, tag;
  if (risk < 20) { level = "Low"; tag = "green"; }
  else if (risk < 40) { level = "Slight"; tag = "blue"; }
  else if (risk < 60) { level = "Moderate"; tag = "yellow"; }
  else if (risk < 80) { level = "High"; tag = "red"; }
  else { level = "Critical"; tag = "red"; }

  return { level, risk: Math.round(risk), tag };
}

// Air Density (kg/m³) - useful for aviation, sports performance
function airDensity(pressureHpa, tempC) {
  if (!isFinite(pressureHpa) || !isFinite(tempC)) return NaN;
  // Ideal gas law: ρ = P / (R * T)
  // R for dry air = 287.05 J/(kg·K)
  const pressurePa = pressureHpa * 100;
  const tempK = tempC + 273.15;
  return pressurePa / (287.05 * tempK);
}

// Pressure Altitude (feet) - altitude indicated at standard pressure
function pressureAltitude(pressureHpa) {
  if (!isFinite(pressureHpa)) return NaN;
  // Standard atmosphere formula
  return 145366.45 * (1 - Math.pow(pressureHpa / 1013.25, 0.190284));
}

// Density Altitude (feet) - pressure altitude corrected for temperature
function densityAltitude(pressureHpa, tempC) {
  if (!isFinite(pressureHpa) || !isFinite(tempC)) return NaN;
  const pa = pressureAltitude(pressureHpa);
  // Standard temp at sea level = 15°C, lapse rate = 1.98°C per 1000ft
  const stdTemp = 15 - (pa / 1000) * 1.98;
  const tempDev = tempC - stdTemp;
  // Density altitude = PA + (120 * temp deviation)
  return pa + (120 * tempDev);
}

// Thom Discomfort Index (simpler than heat index)
function thomDiscomfort(tC, rh) {
  if (!isFinite(tC) || !isFinite(rh)) return NaN;
  // DI = T - (0.55 - 0.0055 * RH) * (T - 14.5)
  return tC - (0.55 - 0.0055 * rh) * (tC - 14.5);
}

function thomCategory(di) {
  if (!isFinite(di)) return "--";
  if (di < 21) return "No discomfort";
  if (di < 24) return "Under 50% feel discomfort";
  if (di < 27) return "Over 50% feel discomfort";
  if (di < 29) return "Most feel discomfort";
  if (di < 32) return "Severe stress";
  return "Dangerous";
}

// Enthalpy (kJ/kg) - total heat content of air, useful for HVAC
function enthalpy(tC, rh) {
  if (!isFinite(tC) || !isFinite(rh)) return NaN;
  // Saturation vapor pressure (Magnus formula)
  const es = 6.112 * Math.exp((17.67 * tC) / (tC + 243.5));
  // Actual vapor pressure
  const e = (rh / 100) * es;
  // Humidity ratio (kg water / kg dry air)
  const W = 0.622 * e / (1013.25 - e);
  // Enthalpy: h = 1.006*T + W*(2501 + 1.86*T)
  return 1.006 * tC + W * (2501 + 1.86 * tC);
}

// Heating Degree Days (base 18°C)
function heatingDegreeDays(tC, baseTemp = 18) {
  if (!isFinite(tC)) return 0;
  return Math.max(0, baseTemp - tC);
}

// Cooling Degree Days (base 18°C)
function coolingDegreeDays(tC, baseTemp = 18) {
  if (!isFinite(tC)) return 0;
  return Math.max(0, tC - baseTemp);
}

// Overall comfort assessment
function overallComfort(tC, rh, dp) {
  if (!isFinite(tC) || !isFinite(rh)) return { level: "--", advice: "" };

  let score = 100; // Start with perfect
  let issues = [];

  // Temperature penalties
  if (tC < 10) { score -= 30; issues.push("cold"); }
  else if (tC < 18) { score -= 10; issues.push("cool"); }
  else if (tC > 30) { score -= 30; issues.push("hot"); }
  else if (tC > 26) { score -= 10; issues.push("warm"); }

  // Humidity penalties
  if (rh > 80) { score -= 20; issues.push("very humid"); }
  else if (rh > 65) { score -= 10; issues.push("humid"); }
  else if (rh < 30) { score -= 15; issues.push("dry"); }

  // Dew point penalties (mugginess)
  if (dp > 20) { score -= 15; issues.push("muggy"); }

  let level, outdoor, sleep, resp;

  if (score >= 80) {
    level = "Excellent";
    outdoor = "Ideal";
    sleep = "Excellent";
    resp = "Excellent";
  } else if (score >= 60) {
    level = "Good";
    outdoor = "Good";
    sleep = "Good";
    resp = "Good";
  } else if (score >= 40) {
    level = "Fair";
    outdoor = "Moderate";
    sleep = "Fair";
    resp = "Fair";
  } else {
    level = "Poor";
    outdoor = "Limited";
    sleep = "Difficult";
    resp = "Stressed";
  }

  // Specific adjustments
  if (tC > 30 || tC < 5) outdoor = "Caution";
  if (tC > 26 || tC < 16 || rh > 70) sleep = sleep === "Excellent" ? "Good" : sleep;
  if (rh < 30 || rh > 80) resp = "Irritated";

  let advice = "";
  if (issues.length === 0) {
    advice = "Conditions are ideal for all activities.";
  } else {
    advice = "Current conditions: " + issues.join(", ") + ". ";
    if (issues.includes("hot") || issues.includes("humid")) {
      advice += "Stay hydrated and seek shade.";
    } else if (issues.includes("cold")) {
      advice += "Dress warmly for outdoor activities.";
    } else if (issues.includes("dry")) {
      advice += "Consider using a humidifier indoors.";
    }
  }

  return { level, outdoor, sleep, resp, advice };
}

async function tick() {
  try {
    const res = await fetch('/api/derived', { cache: 'no-store' });
    const j = await res.json();
    if (!j.ok) throw new Error('api error');

    const t = j.raw.temp_c;
    const rh = j.raw.hum_pct;
    const gas = j.raw.gas_kohm;
    const slp = j.derived.slp_hpa;
    const tend = j.derived.press_tendency_hpa_hr;
    const dp = j.derived.dew_point_c;

    // AQI
    const aqi = gasToAQI(gas);
    el('aqi').textContent = isFinite(aqi) ? Math.round(aqi) : '--';
    el('aqi-cat').textContent = aqiCategory(aqi);
    el('gas-raw').textContent = isFinite(gas) ? gas.toFixed(1) + ' kOhm' : '-- kOhm';

    // Cloud base
    const spread = t - dp;
    const cloudH = cloudBase(t, dp);
    el('cloud-base').textContent = isFinite(cloudH) ? Math.round(cloudH) : '--';
    el('spread').textContent = isFinite(spread) ? spread.toFixed(1) + ' °C' : '-- °C';
    el('cloud-cond').textContent = cloudCondition(spread);

    // Frost risk
    const frost = frostRisk(t, dp, rh);
    el('frost-tag').textContent = frost.level;
    el('frost-tag').className = 'tag ' + (frost.tag || '');
    el('ground-temp').textContent = isFinite(frost.groundTemp) ? frost.groundTemp.toFixed(1) + ' °C' : '-- °C';
    el('frost-hours').textContent = frost.hours !== null ? (frost.hours === 0 ? 'Now' : '~' + frost.hours + 'h') : 'N/A';

    // Humidex
    const hx = humidex(t, rh);
    el('humidex').textContent = isFinite(hx) ? hx.toFixed(1) : '--';
    el('humidex-cat').textContent = humidexCategory(hx);
    const hxDiff = hx - t;
    el('humidex-diff').textContent = isFinite(hxDiff) ? (hxDiff >= 0 ? '+' : '') + hxDiff.toFixed(1) + ' °C' : '--';

    // Zambretti Weather forecast
    const month = new Date().getMonth() + 1; // 1-12
    const fc = zambrettiForecast(slp, tend, month);
    el('forecast').textContent = fc.text;
    el('forecast-detail').textContent = fc.detail;
    el('trend-dir').textContent = fc.trend ? fc.trend.charAt(0).toUpperCase() + fc.trend.slice(1) : '--';
    el('trend-rate').textContent = isFinite(tend) ? tend.toFixed(2) + ' hPa/hr' : '--';
    el('press-slp').textContent = isFinite(slp) ? slp.toFixed(1) + ' hPa' : '--';
    el('forecast-conf').textContent = fc.confidence;

    // ASHRAE / PMV
    const pmv = calculatePMV(t, rh);
    el('ashrae-tag').textContent = pmvCategory(pmv);
    el('ashrae-tag').className = 'tag ' + (pmv > 1 ? 'yellow' : pmv < -1 ? 'blue' : 'green');
    el('pmv-tag').textContent = 'PMV: ' + (isFinite(pmv) ? pmv.toFixed(2) : '--');
    // Position marker (PMV -3 to +3 mapped to 0-100%)
    const markerPos = isFinite(pmv) ? ((pmv + 3) / 6) * 100 : 50;
    el('pmv-marker').style.left = markerPos + '%';

    // Growing Degree Days
    const gdd = calculateGDD(t, 10);
    el('gdd-today').textContent = isFinite(gdd) ? gdd.toFixed(1) : '--';
    el('gdd-rate').textContent = isFinite(gdd) ? (gdd > 0 ? '+' + gdd.toFixed(2) + '/hr' : 'none') : '--';
    el('plant-tag').textContent = plantGrowthPotential(gdd);
    el('plant-tag').className = 'tag ' + (gdd > 10 ? 'green' : gdd > 0 ? 'yellow' : '');

    // Overall comfort
    const comfort = overallComfort(t, rh, dp);
    el('comfort-overall').textContent = comfort.level;
    el('outdoor-rec').textContent = comfort.outdoor;
    el('sleep-qual').textContent = comfort.sleep;
    el('resp-comfort').textContent = comfort.resp;
    el('comfort-advice').textContent = comfort.advice;

    // Mold Risk
    const mold = moldRisk(t, rh);
    el('mold-tag').textContent = mold.level;
    el('mold-tag').className = 'tag ' + (mold.tag || '');
    el('mold-fill').style.width = mold.risk + '%';
    el('mold-score').textContent = mold.risk + ' %';
    el('mold-rh').textContent = isFinite(rh) ? rh.toFixed(1) + ' %' : '-- %';

    // Air Density
    const stationPress = j.raw.press_hpa;
    const density = airDensity(stationPress, t);
    el('air-density').textContent = isFinite(density) ? density.toFixed(4) : '--';
    const pa = pressureAltitude(stationPress);
    el('press-alt').textContent = isFinite(pa) ? Math.round(pa) + ' ft' : '-- ft';
    const da = densityAltitude(stationPress, t);
    el('density-alt').textContent = isFinite(da) ? Math.round(da) + ' ft' : '-- ft';

    // Thom Discomfort Index
    const thom = thomDiscomfort(t, rh);
    el('thom-idx').textContent = isFinite(thom) ? thom.toFixed(1) : '--';
    el('thom-cat').textContent = thomCategory(thom);
    const hi = j.derived.heat_index_c;
    const thomVsHi = isFinite(thom) && isFinite(hi) ? thom - hi : NaN;
    el('thom-vs-hi').textContent = isFinite(thomVsHi) ? (thomVsHi >= 0 ? '+' : '') + thomVsHi.toFixed(1) + ' °C' : '--';

    // Enthalpy
    const enth = enthalpy(t, rh);
    el('enthalpy').textContent = isFinite(enth) ? enth.toFixed(1) : '--';
    // HVAC load indicator
    let hvacLoad = '--';
    if (isFinite(enth)) {
      if (enth < 30) hvacLoad = 'Low (heating)';
      else if (enth < 50) hvacLoad = 'Optimal';
      else if (enth < 70) hvacLoad = 'Moderate';
      else hvacLoad = 'High (cooling)';
    }
    el('hvac-load').textContent = hvacLoad;
    // Benchmark (ideal is ~50 kJ/kg at 22°C, 50% RH)
    let enthBench = '--';
    if (isFinite(enth)) {
      if (enth >= 40 && enth <= 60) enthBench = 'Ideal range';
      else if (enth < 40) enthBench = (40 - enth).toFixed(0) + ' below ideal';
      else enthBench = (enth - 60).toFixed(0) + ' above ideal';
    }
    el('enthalpy-bench').textContent = enthBench;

    // Heating/Cooling Degree Days
    const hdd = heatingDegreeDays(t, 18);
    const cdd = coolingDegreeDays(t, 18);
    el('hdd').textContent = isFinite(hdd) ? hdd.toFixed(1) : '--';
    el('cdd').textContent = isFinite(cdd) ? cdd.toFixed(1) : '--';
    // Energy mode
    let energyMode = '--';
    if (isFinite(t)) {
      if (t < 16) energyMode = 'Heating';
      else if (t > 20) energyMode = 'Cooling';
      else energyMode = 'Neutral';
    }
    el('energy-mode').textContent = energyMode;
    // Marker position: -10°C = 0%, 18°C = 50%, 35°C = 100%
    const ddMarkerPos = isFinite(t) ? Math.min(100, Math.max(0, ((t + 10) / 45) * 100)) : 50;
    el('dd-marker').style.left = ddMarkerPos + '%';

  } catch (e) {
    // Fail silently
  }
}

tick();
setInterval(tick, 2000);
</script>
</body>
</html>
)HTML");
}
