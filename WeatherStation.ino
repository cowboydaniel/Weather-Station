#include <WiFiS3.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME680.h>
#include <math.h>

// Pages (HTML lives in these headers)
#include "page_index.h"
#include "page_temp.h"
#include "page_humidity.h"
#include "page_pressure.h"
#include "page_gas.h"
#include "page_comfort.h"   // NEW

// ================= USER CONFIG =================
const char ssid[] = "outback_hut";
const char pass[] = "wildmonkeys2810";
const float ALTITUDE_M = 242.0f;
// ===============================================

WiFiServer server(80);
Adafruit_BME680 bme;
uint8_t BME_ADDR = 0x76;

// Sampling intervals
const uint32_t ENV_INTERVAL_MS = 1000;  // temp/hum/pressure
const uint32_t GAS_INTERVAL_MS = 3000;  // gas resistance

// History sizes
const int ENV_SERIES_POINTS = 600;  // 10 minutes @ 1 Hz
const int GAS_SERIES_POINTS = 200;  // 10 minutes @ 3 s
const uint32_t SLP_TREND_SAMPLE_MS = 60000; // 1-minute samples
const int SLP_TREND_POINTS = 60;            // 60 minutes history
const int SLP_TREND_WINDOW_MIN = 15;        // slope over 15 minutes

// Latest raw readings
volatile float t_raw = NAN, h_raw = NAN, p_raw_hpa = NAN, g_raw_kohm = NAN;
uint32_t last_env_ms = 0;
uint32_t last_gas_ms = 0;

// Simple ring buffer for series
struct RingF {
  float *buf;
  int size;
  int next;
  int count;

  RingF(int n) : size(n), next(0), count(0) {
    buf = new float[n];
    for (int i = 0; i < n; i++) buf[i] = NAN;
  }

  void push(float v) {
    buf[next] = v;
    next = (next + 1) % size;
    if (count < size) count++;
  }
};

RingF tempSeries(ENV_SERIES_POINTS);
RingF humSeries(ENV_SERIES_POINTS);
RingF pressSeries(ENV_SERIES_POINTS);     // station pressure hPa
RingF gasSeries(GAS_SERIES_POINTS);       // gas kOhm

RingF slpTrend(SLP_TREND_POINTS);         // sea-level pressure trend (1/min)
uint32_t last_slp_trend_ms = 0;

static void configureBME() {
  bme.setTemperatureOversampling(BME680_OS_4X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_7);
  bme.setGasHeater(320, 150);
}

static bool sampleBME(bool wantGas) {
  if (!bme.performReading()) return false;

  t_raw = bme.temperature;
  h_raw = bme.humidity;
  p_raw_hpa = bme.pressure / 100.0f;

  if (wantGas) {
    g_raw_kohm = bme.gas_resistance / 1000.0f;
  }
  return true;
}

// Sea-level pressure reduction
static float seaLevelPressure_hPa(float station_hPa, float tempC, float altitude_m) {
  if (!isfinite(station_hPa) || !isfinite(tempC) || !isfinite(altitude_m)) return NAN;
  float tempK = tempC + 273.15f;
  float term = 1.0f - (0.0065f * altitude_m) / (tempK + 0.0065f * altitude_m);
  if (term <= 0.0f) return NAN;
  return station_hPa * powf(term, -5.257f);
}

// Dew point (Magnus)
static float dewPointC(float tC, float rh) {
  if (!isfinite(tC) || !isfinite(rh)) return NAN;
  rh = fmaxf(1.0f, fminf(rh, 100.0f));
  const float a = 17.62f, b = 243.12f;
  float gamma = (a * tC) / (b + tC) + logf(rh / 100.0f);
  return (b * gamma) / (a - gamma);
}

// Heat index (NOAA)
static float heatIndexC(float tC, float rh) {
  if (!isfinite(tC) || !isfinite(rh)) return NAN;
  if (tC < 26.0f || rh < 40.0f) return tC;

  float T = tC * 9.0f / 5.0f + 32.0f;
  float R = rh;

  float HI =
    -42.379f +
    2.04901523f * T +
    10.14333127f * R +
    -0.22475541f * T * R +
    -0.00683783f * T * T +
    -0.05481717f * R * R +
    0.00122874f * T * T * R +
    0.00085282f * T * R * R +
    -0.00000199f * T * T * R * R;

  return (HI - 32.0f) * 5.0f / 9.0f;
}

// Pressure tendency from SLP trend (hPa/hr)
static float slpMinutesAgo(int minutesAgo) {
  if (slpTrend.count == 0) return NAN;
  if (minutesAgo <= 0) return NAN;
  if (minutesAgo >= slpTrend.count) minutesAgo = slpTrend.count - 1;

  int newest = slpTrend.next - 1;
  if (newest < 0) newest += slpTrend.size;

  int idx = newest - minutesAgo;
  while (idx < 0) idx += slpTrend.size;

  return slpTrend.buf[idx];
}

static float pressureTendency_hPaPerHour(float slp_now, int windowMin) {
  if (!isfinite(slp_now)) return NAN;
  float slp_then = slpMinutesAgo(windowMin);
  if (!isfinite(slp_then)) return NAN;
  float delta = slp_now - slp_then;
  return delta * (60.0f / (float)windowMin);
}

static const char* tendencyLabel(float hPaPerHr) {
  if (!isfinite(hPaPerHr)) return "unknown";
  if (hPaPerHr > 1.0f) return "rising";
  if (hPaPerHr < -1.0f) return "falling";
  return "steady";
}

static int stormScore(float slp_hpa, float tend_hpa_hr, float rh) {
  if (!isfinite(slp_hpa) || !isfinite(tend_hpa_hr) || !isfinite(rh)) return -1;
  float score = 0.0f;

  if (slp_hpa < 1010) score += (1010 - slp_hpa) * 2.0f;
  if (slp_hpa < 1000) score += (1000 - slp_hpa) * 3.0f;
  if (slp_hpa < 990)  score += (990 - slp_hpa) * 4.0f;

  if (tend_hpa_hr < 0) score += (-tend_hpa_hr) * 15.0f;
  if (rh > 70) score += (rh - 70) * 0.8f;

  if (score < 0) score = 0;
  if (score > 100) score = 100;
  return (int)(score + 0.5f);
}

static const char* stormLabelFromScore(int s) {
  if (s < 0) return "unknown";
  if (s < 25) return "low";
  if (s < 55) return "moderate";
  if (s < 80) return "high";
  return "very high";
}

// JSON series writer (chronological order)
static void jsonWriteSeries(WiFiClient &c, const RingF &r, int decimals) {
  c.print("[");
  int count = r.count;
  int start = r.next - count;
  while (start < 0) start += r.size;

  for (int i = 0; i < count; i++) {
    int idx = (start + i) % r.size;
    float v = r.buf[idx];
    if (!isfinite(v)) c.print("null");
    else c.print(v, decimals);
    if (i != count - 1) c.print(",");
  }
  c.print("]");
}

// Sampling scheduler
static void updateSampling() {
  uint32_t now = millis();
  bool doEnv = (now - last_env_ms >= ENV_INTERVAL_MS);
  bool doGas = (now - last_gas_ms >= GAS_INTERVAL_MS);

  if (!doEnv && !doGas) return;
  if (!sampleBME(doGas)) return;

  if (doEnv) {
    last_env_ms = now;
    tempSeries.push(t_raw);
    humSeries.push(h_raw);
    pressSeries.push(p_raw_hpa);
  }

  if (doGas) {
    last_gas_ms = now;
    gasSeries.push(g_raw_kohm);
  }

  // 1-minute SLP trend
  if (now - last_slp_trend_ms >= SLP_TREND_SAMPLE_MS) {
    last_slp_trend_ms = now;
    float slp = seaLevelPressure_hPa(p_raw_hpa, t_raw, ALTITUDE_M);
    slpTrend.push(slp);
  }
}

// API endpoints
static void sendJSONTemp(WiFiClient &c) {
  c.println("HTTP/1.1 200 OK");
  c.println("Content-Type: application/json; charset=utf-8");
  c.println("Cache-Control: no-store");
  c.println("Connection: close");
  c.println();
  c.print("{\"ok\":true,\"unit\":\"C\",\"interval_ms\":"); c.print(ENV_INTERVAL_MS);
  c.print(",\"series\":"); jsonWriteSeries(c, tempSeries, 2);
  c.println("}");
}

static void sendJSONHumidity(WiFiClient &c) {
  c.println("HTTP/1.1 200 OK");
  c.println("Content-Type: application/json; charset=utf-8");
  c.println("Cache-Control: no-store");
  c.println("Connection: close");
  c.println();
  c.print("{\"ok\":true,\"unit\":\"%\",\"interval_ms\":"); c.print(ENV_INTERVAL_MS);
  c.print(",\"series\":"); jsonWriteSeries(c, humSeries, 2);
  c.println("}");
}

static void sendJSONPressure(WiFiClient &c) {
  float slp_now = seaLevelPressure_hPa(p_raw_hpa, t_raw, ALTITUDE_M);
  float tend = pressureTendency_hPaPerHour(slp_now, SLP_TREND_WINDOW_MIN);

  c.println("HTTP/1.1 200 OK");
  c.println("Content-Type: application/json; charset=utf-8");
  c.println("Cache-Control: no-store");
  c.println("Connection: close");
  c.println();

  c.print("{\"ok\":true,");
  c.print("\"unit\":\"hPa\",");
  c.print("\"interval_ms\":"); c.print(ENV_INTERVAL_MS); c.print(",");
  c.print("\"station_series\":"); jsonWriteSeries(c, pressSeries, 2); c.print(",");
  c.print("\"slp_now\":"); c.print(slp_now, 2); c.print(",");
  c.print("\"slp_trend_interval_ms\":"); c.print(SLP_TREND_SAMPLE_MS); c.print(",");
  c.print("\"slp_trend_series\":"); jsonWriteSeries(c, slpTrend, 2); c.print(",");
  c.print("\"tendency_hpa_hr\":"); c.print(tend, 2); c.print(",");
  c.print("\"tendency\":\""); c.print(tendencyLabel(tend)); c.print("\"");
  c.println("}");
}

static void sendJSONGas(WiFiClient &c) {
  c.println("HTTP/1.1 200 OK");
  c.println("Content-Type: application/json; charset=utf-8");
  c.println("Cache-Control: no-store");
  c.println("Connection: close");
  c.println();
  c.print("{\"ok\":true,\"unit\":\"kOhm\",\"interval_ms\":"); c.print(GAS_INTERVAL_MS);
  c.print(",\"series\":"); jsonWriteSeries(c, gasSeries, 2);
  c.println("}");
}

static void sendJSONSummary(WiFiClient &c) {
  float slp_now = seaLevelPressure_hPa(p_raw_hpa, t_raw, ALTITUDE_M);
  float dp = dewPointC(t_raw, h_raw);
  float hi = heatIndexC(t_raw, h_raw);
  float tend = pressureTendency_hPaPerHour(slp_now, SLP_TREND_WINDOW_MIN);
  int storm = stormScore(slp_now, tend, h_raw);

  c.println("HTTP/1.1 200 OK");
  c.println("Content-Type: application/json; charset=utf-8");
  c.println("Cache-Control: no-store");
  c.println("Connection: close");
  c.println();

  c.print("{\"ok\":true,");
  c.print("\"raw\":{");
  c.print("\"temp_c\":"); c.print(t_raw,2); c.print(",");
  c.print("\"hum_pct\":"); c.print(h_raw,2); c.print(",");
  c.print("\"press_hpa\":"); c.print(p_raw_hpa,2); c.print(",");
  c.print("\"gas_kohm\":"); c.print(g_raw_kohm,2);
  c.print("},");
  c.print("\"derived\":{");
  c.print("\"altitude_m\":"); c.print(ALTITUDE_M,0); c.print(",");
  c.print("\"slp_hpa\":"); c.print(slp_now,2); c.print(",");
  c.print("\"dew_point_c\":"); c.print(dp,2); c.print(",");
  c.print("\"heat_index_c\":"); c.print(hi,2); c.print(",");
  c.print("\"press_tendency_hpa_hr\":"); c.print(tend,2); c.print(",");
  c.print("\"press_tendency\":\""); c.print(tendencyLabel(tend)); c.print("\",");
  c.print("\"storm_score\":"); c.print(storm); c.print(",");
  c.print("\"storm\":\""); c.print(stormLabelFromScore(storm)); c.print("\"");
  c.print("}}");
  c.println();
}

// Basic 404
static void send404(WiFiClient &c) {
  c.println("HTTP/1.1 404 Not Found");
  c.println("Content-Type: text/plain; charset=utf-8");
  c.println("Connection: close");
  c.println();
  c.println("404");
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {}

  Serial.println("Connecting to WiFi...");
  while (WiFi.begin(ssid, pass) != WL_CONNECTED) delay(1000);

  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  Wire.begin();

  Serial.println("Starting BME680...");
  if (!bme.begin(BME_ADDR)) {
    Serial.println("BME680 not found at 0x76, trying 0x77...");
    BME_ADDR = 0x77;
    if (!bme.begin(BME_ADDR)) {
      Serial.println("BME680 not found. Check wiring/power.");
      while (true) delay(1000);
    }
  }
  configureBME();

  last_env_ms = millis() - ENV_INTERVAL_MS;
  last_gas_ms = millis() - GAS_INTERVAL_MS;
  last_slp_trend_ms = millis() - SLP_TREND_SAMPLE_MS;

  // Prime
  updateSampling();

  server.begin();
  Serial.println("Web server started");
  Serial.print("Open browser to: http://");
  Serial.println(WiFi.localIP());
}

void loop() {
  updateSampling();

  WiFiClient client = server.available();
  if (!client) return;

  IPAddress rip = client.remoteIP();

  // Request line
  unsigned long start = millis();
  String reqLine = "";
  while (client.connected() && (millis() - start < 1500)) {
    if (client.available()) {
      reqLine = client.readStringUntil('\n');
      break;
    }
  }

  // Drain headers
  start = millis();
  while (client.connected() && (millis() - start < 1500)) {
    if (client.available()) {
      String line = client.readStringUntil('\n');
      if (line == "\r") break;
    }
  }

  if (reqLine.length() == 0) {
    Serial.print("Client "); Serial.print(rip);
    Serial.println(" -> (timeout)");
    client.stop();
    return;
  }

  Serial.print("Client "); Serial.print(rip);
  Serial.print(" -> "); Serial.println(reqLine);

  // Routing (pages)
  if (reqLine.startsWith("GET /temp")) {
    sendPageTemp(client);
  } else if (reqLine.startsWith("GET /humidity")) {
    sendPageHumidity(client);
  } else if (reqLine.startsWith("GET /pressure")) {
    sendPagePressure(client);
  } else if (reqLine.startsWith("GET /gas")) {
    sendPageGas(client);
  } else if (reqLine.startsWith("GET /comfort")) {        // NEW
    sendPageComfort(client);
  } else if (reqLine.startsWith("GET / ")) {
    sendPageIndex(client);

  // Routing (API)
  } else if (reqLine.startsWith("GET /api/temp")) {
    sendJSONTemp(client);
  } else if (reqLine.startsWith("GET /api/humidity")) {
    sendJSONHumidity(client);
  } else if (reqLine.startsWith("GET /api/pressure")) {
    sendJSONPressure(client);
  } else if (reqLine.startsWith("GET /api/gas")) {
    sendJSONGas(client);
  } else if (reqLine.startsWith("GET /api ")) {
    sendJSONSummary(client);
  } else {
    send404(client);
  }

  delay(5);
  client.stop();
}
