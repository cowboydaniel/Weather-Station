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
#include "page_comfort.h"
#include "page_settings.h"
#include "page_stats.h"

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

// Cached derived values (updated when sensor values change, not per-request)
volatile float cached_slp = NAN;
volatile float cached_dp = NAN;
volatile float cached_hi = NAN;
volatile float cached_tend = NAN;
volatile int cached_storm = -1;

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

// Request statistics
volatile uint32_t req_total = 0;
volatile uint32_t req_api = 0;
volatile uint32_t req_pages = 0;
volatile uint32_t req_errors = 0;
volatile uint32_t req_time_sum = 0;
volatile uint32_t loop_count = 0;
volatile uint32_t last_loop_rate_ms = 0;
volatile float loop_rate_hz = 0;

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

// Buffered JSON series writer (chronological order)
// Uses chunked writes to reduce WiFiClient overhead
static char jsonBuf[256];  // Static buffer for building JSON chunks
static int jsonBufPos = 0;

static void jsonFlush(WiFiClient &c) {
  if (jsonBufPos > 0) {
    c.write((const uint8_t*)jsonBuf, jsonBufPos);
    jsonBufPos = 0;
  }
}

static void jsonAppend(WiFiClient &c, const char* s) {
  int len = strlen(s);
  while (len > 0) {
    int space = sizeof(jsonBuf) - jsonBufPos - 1;
    if (space <= 0) {
      jsonFlush(c);
      space = sizeof(jsonBuf) - 1;
    }
    int chunk = (len < space) ? len : space;
    memcpy(jsonBuf + jsonBufPos, s, chunk);
    jsonBufPos += chunk;
    s += chunk;
    len -= chunk;
  }
}

static void jsonAppendFloat(WiFiClient &c, float v, int decimals) {
  char tmp[16];
  if (!isfinite(v)) {
    strcpy(tmp, "null");
  } else {
    dtostrf(v, 0, decimals, tmp);
  }
  jsonAppend(c, tmp);
}

static void jsonWriteSeries(WiFiClient &c, const RingF &r, int decimals) {
  jsonBufPos = 0;  // Reset buffer
  jsonAppend(c, "[");

  int count = r.count;
  int start = r.next - count;
  while (start < 0) start += r.size;

  for (int i = 0; i < count; i++) {
    int idx = (start + i) % r.size;
    float v = r.buf[idx];
    jsonAppendFloat(c, v, decimals);
    if (i != count - 1) jsonAppend(c, ",");
  }
  jsonAppend(c, "]");
  jsonFlush(c);
}

// Update cached derived values (called when sensor values change)
static void updateCachedDerived() {
  cached_slp = seaLevelPressure_hPa(p_raw_hpa, t_raw, ALTITUDE_M);
  cached_dp = dewPointC(t_raw, h_raw);
  cached_hi = heatIndexC(t_raw, h_raw);
  cached_tend = pressureTendency_hPaPerHour(cached_slp, SLP_TREND_WINDOW_MIN);
  cached_storm = stormScore(cached_slp, cached_tend, h_raw);
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

    // Update derived values when environment data changes
    updateCachedDerived();
  }

  if (doGas) {
    last_gas_ms = now;
    gasSeries.push(g_raw_kohm);
  }

  // 1-minute SLP trend
  if (now - last_slp_trend_ms >= SLP_TREND_SAMPLE_MS) {
    last_slp_trend_ms = now;
    slpTrend.push(cached_slp);
    // Recompute tendency after adding new trend point
    cached_tend = pressureTendency_hPaPerHour(cached_slp, SLP_TREND_WINDOW_MIN);
    cached_storm = stormScore(cached_slp, cached_tend, h_raw);
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
  // Use cached values instead of recomputing
  c.println("HTTP/1.1 200 OK");
  c.println("Content-Type: application/json; charset=utf-8");
  c.println("Cache-Control: no-store");
  c.println("Connection: close");
  c.println();

  c.print("{\"ok\":true,");
  c.print("\"unit\":\"hPa\",");
  c.print("\"interval_ms\":"); c.print(ENV_INTERVAL_MS); c.print(",");
  c.print("\"station_series\":"); jsonWriteSeries(c, pressSeries, 2); c.print(",");
  c.print("\"slp_now\":"); c.print(cached_slp, 2); c.print(",");
  c.print("\"slp_trend_interval_ms\":"); c.print(SLP_TREND_SAMPLE_MS); c.print(",");
  c.print("\"slp_trend_series\":"); jsonWriteSeries(c, slpTrend, 2); c.print(",");
  c.print("\"tendency_hpa_hr\":"); c.print(cached_tend, 2); c.print(",");
  c.print("\"tendency\":\""); c.print(tendencyLabel(cached_tend)); c.print("\"");
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
  // Use cached derived values instead of recomputing
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
  c.print("\"slp_hpa\":"); c.print(cached_slp,2); c.print(",");
  c.print("\"dew_point_c\":"); c.print(cached_dp,2); c.print(",");
  c.print("\"heat_index_c\":"); c.print(cached_hi,2); c.print(",");
  c.print("\"press_tendency_hpa_hr\":"); c.print(cached_tend,2); c.print(",");
  c.print("\"press_tendency\":\""); c.print(tendencyLabel(cached_tend)); c.print("\",");
  c.print("\"storm_score\":"); c.print(cached_storm); c.print(",");
  c.print("\"storm\":\""); c.print(stormLabelFromScore(cached_storm)); c.print("\"");
  c.print("}}");
  c.println();
}

// Combined dashboard endpoint - returns summary + sparkline data in one request
static void sendJSONDashboard(WiFiClient &c) {
  c.println("HTTP/1.1 200 OK");
  c.println("Content-Type: application/json; charset=utf-8");
  c.println("Cache-Control: no-store");
  c.println("Connection: close");
  c.println();

  c.print("{\"ok\":true,");
  // Raw values
  c.print("\"raw\":{");
  c.print("\"temp_c\":"); c.print(t_raw,2); c.print(",");
  c.print("\"hum_pct\":"); c.print(h_raw,2); c.print(",");
  c.print("\"press_hpa\":"); c.print(p_raw_hpa,2); c.print(",");
  c.print("\"gas_kohm\":"); c.print(g_raw_kohm,2);
  c.print("},");
  // Derived values
  c.print("\"derived\":{");
  c.print("\"altitude_m\":"); c.print(ALTITUDE_M,0); c.print(",");
  c.print("\"slp_hpa\":"); c.print(cached_slp,2); c.print(",");
  c.print("\"dew_point_c\":"); c.print(cached_dp,2); c.print(",");
  c.print("\"heat_index_c\":"); c.print(cached_hi,2); c.print(",");
  c.print("\"press_tendency_hpa_hr\":"); c.print(cached_tend,2); c.print(",");
  c.print("\"press_tendency\":\""); c.print(tendencyLabel(cached_tend)); c.print("\",");
  c.print("\"storm_score\":"); c.print(cached_storm); c.print(",");
  c.print("\"storm\":\""); c.print(stormLabelFromScore(cached_storm)); c.print("\"");
  c.print("},");
  // Sparkline data for SLP trend
  c.print("\"slp_trend\":"); jsonWriteSeries(c, slpTrend, 2); c.print(",");
  // Sparkline data for gas
  c.print("\"gas_series\":"); jsonWriteSeries(c, gasSeries, 2);
  c.println("}");
}

// Combined comfort endpoint - returns summary + temp/humidity series for comfort page
static void sendJSONComfort(WiFiClient &c) {
  c.println("HTTP/1.1 200 OK");
  c.println("Content-Type: application/json; charset=utf-8");
  c.println("Cache-Control: no-store");
  c.println("Connection: close");
  c.println();

  c.print("{\"ok\":true,");
  // Raw values
  c.print("\"raw\":{");
  c.print("\"temp_c\":"); c.print(t_raw,2); c.print(",");
  c.print("\"hum_pct\":"); c.print(h_raw,2);
  c.print("},");
  // Derived values
  c.print("\"derived\":{");
  c.print("\"dew_point_c\":"); c.print(cached_dp,2); c.print(",");
  c.print("\"heat_index_c\":"); c.print(cached_hi,2); c.print(",");
  c.print("\"storm_score\":"); c.print(cached_storm);
  c.print("},");
  // Series for computing derived history
  c.print("\"temp_series\":"); jsonWriteSeries(c, tempSeries, 2); c.print(",");
  c.print("\"hum_series\":"); jsonWriteSeries(c, humSeries, 2); c.print(",");
  // Series count for incremental updates
  c.print("\"series_count\":"); c.print(tempSeries.count);
  c.println("}");
}

// Stats API endpoint
static void sendJSONStats(WiFiClient &c) {
  c.println("HTTP/1.1 200 OK");
  c.println("Content-Type: application/json; charset=utf-8");
  c.println("Cache-Control: no-store");
  c.println("Connection: close");
  c.println();

  // Get WiFi info
  long rssi = WiFi.RSSI();
  IPAddress ip = WiFi.localIP();
  byte mac[6];
  WiFi.macAddress(mac);

  // Arduino UNO R4 WiFi has 32KB SRAM
  const uint32_t TOTAL_RAM = 32768;

  // Estimate free RAM (rough approximation for ARM)
  extern char _end;
  extern char *__brkval;
  char stackVar;
  uint32_t freeRam = (uint32_t)&stackVar - (__brkval == 0 ? (uint32_t)&_end : (uint32_t)__brkval);

  // Calculate average response time
  float avgMs = (req_total > 0) ? (float)req_time_sum / (float)req_total : 0;

  c.print("{\"ok\":true,");

  // Network section
  c.print("\"network\":{");
  c.print("\"connected\":"); c.print(WiFi.status() == WL_CONNECTED ? "true" : "false"); c.print(",");
  c.print("\"ip\":\""); c.print(ip); c.print("\",");
  c.print("\"ssid\":\""); c.print(ssid); c.print("\",");
  c.print("\"rssi\":"); c.print(rssi); c.print(",");

  // MAC address
  c.print("\"mac\":\"");
  for (int i = 5; i >= 0; i--) {
    if (mac[i] < 16) c.print("0");
    c.print(mac[i], HEX);
    if (i > 0) c.print(":");
  }
  c.print("\"");
  c.print("},");

  // Arduino section
  c.print("\"arduino\":{");
  c.print("\"uptime_ms\":"); c.print(millis()); c.print(",");
  c.print("\"free_ram\":"); c.print(freeRam); c.print(",");
  c.print("\"total_ram\":"); c.print(TOTAL_RAM); c.print(",");
  c.print("\"loop_rate\":"); c.print(loop_rate_hz, 1);
  c.print("},");

  // Buffer section
  c.print("\"buffers\":{");
  c.print("\"temp_count\":"); c.print(tempSeries.count); c.print(",");
  c.print("\"hum_count\":"); c.print(humSeries.count); c.print(",");
  c.print("\"press_count\":"); c.print(pressSeries.count); c.print(",");
  c.print("\"gas_count\":"); c.print(gasSeries.count); c.print(",");
  c.print("\"slp_count\":"); c.print(slpTrend.count); c.print(",");
  c.print("\"total_samples\":"); c.print(tempSeries.count + gasSeries.count);
  c.print("},");

  // Sensor section
  c.print("\"sensor\":{");
  c.print("\"temp_c\":"); c.print(t_raw, 2); c.print(",");
  c.print("\"hum_pct\":"); c.print(h_raw, 2); c.print(",");
  c.print("\"press_hpa\":"); c.print(p_raw_hpa, 2); c.print(",");
  c.print("\"gas_kohm\":"); c.print(g_raw_kohm, 2);
  c.print("},");

  // Request stats section
  c.print("\"requests\":{");
  c.print("\"total\":"); c.print(req_total); c.print(",");
  c.print("\"api\":"); c.print(req_api); c.print(",");
  c.print("\"pages\":"); c.print(req_pages); c.print(",");
  c.print("\"errors\":"); c.print(req_errors); c.print(",");
  c.print("\"avg_ms\":"); c.print(avgMs, 1);
  c.print("}");

  c.println("}");
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

  // Calculate loop rate (update every second)
  loop_count++;
  uint32_t now = millis();
  if (now - last_loop_rate_ms >= 1000) {
    loop_rate_hz = (float)loop_count * 1000.0f / (float)(now - last_loop_rate_ms);
    loop_count = 0;
    last_loop_rate_ms = now;
  }

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

  unsigned long reqStart = millis();
  bool isApi = false;
  bool isPage = false;
  bool isError = false;

  // Routing (pages)
  if (reqLine.startsWith("GET /temp")) {
    sendPageTemp(client);
    isPage = true;
  } else if (reqLine.startsWith("GET /humidity")) {
    sendPageHumidity(client);
    isPage = true;
  } else if (reqLine.startsWith("GET /pressure")) {
    sendPagePressure(client);
    isPage = true;
  } else if (reqLine.startsWith("GET /gas")) {
    sendPageGas(client);
    isPage = true;
  } else if (reqLine.startsWith("GET /comfort")) {
    sendPageComfort(client);
    isPage = true;
  } else if (reqLine.startsWith("GET /settings")) {
    sendPageSettings(client);
    isPage = true;
  } else if (reqLine.startsWith("GET /stats")) {
    sendPageStats(client);
    isPage = true;
  } else if (reqLine.startsWith("GET / ")) {
    sendPageIndex(client);
    isPage = true;

  // Routing (API)
  } else if (reqLine.startsWith("GET /api/dashboard")) {
    sendJSONDashboard(client);
    isApi = true;
  } else if (reqLine.startsWith("GET /api/comfort")) {
    sendJSONComfort(client);
    isApi = true;
  } else if (reqLine.startsWith("GET /api/stats")) {
    sendJSONStats(client);
    isApi = true;
  } else if (reqLine.startsWith("GET /api/temp")) {
    sendJSONTemp(client);
    isApi = true;
  } else if (reqLine.startsWith("GET /api/humidity")) {
    sendJSONHumidity(client);
    isApi = true;
  } else if (reqLine.startsWith("GET /api/pressure")) {
    sendJSONPressure(client);
    isApi = true;
  } else if (reqLine.startsWith("GET /api/gas")) {
    sendJSONGas(client);
    isApi = true;
  } else if (reqLine.startsWith("GET /api ")) {
    sendJSONSummary(client);
    isApi = true;
  } else {
    send404(client);
    isError = true;
  }

  // Update request statistics
  req_total++;
  if (isApi) req_api++;
  if (isPage) req_pages++;
  if (isError) req_errors++;
  req_time_sum += (millis() - reqStart);

  delay(5);
  client.stop();
}
