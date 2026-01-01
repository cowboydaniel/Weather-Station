#include <WiFiS3.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME680.h>
#include <RTC.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <math.h>
#include <time.h>

// SD card logging with software SPI
#include "sd_logging.h"

// WiFi connection manager with exponential backoff
#include "wifi_manager.h"

// Pages (HTML lives in these headers)
#include "page_index.h"
#include "page_graphs.h"
#include "page_comfort.h"
#include "page_derived.h"
#include "page_settings.h"
#include "page_stats.h"
// #include "static_assets.h"  -- REMOVED: static assets now served from SD card

// ================= USER CONFIG =================
const char ssid[] = "outbackhut";
const char pass[] = "wildmonkeys2810";
const char device_name[] = "weather-station";  // Device name for WiFi network
const float ALTITUDE_M = 242.0f;
// Timezone: AEST is UTC+10, AEDT (daylight) is UTC+11
const char* timezone_str = "AEST-10AEDT,M10.1.0,M4.1.0";  // Australian Eastern Time
// ===============================================

WiFiServer server(80);
Adafruit_BME680 bme;
uint8_t BME_ADDR = 0x76;

// NTP and RTC
WiFiUDP ntpUdp;
NTPClient timeClient(ntpUdp, "pool.ntp.org", 0, 60000);  // 0 offset (UTC), 60s update interval

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

// Hourly history for longer-term visualization (24 samples = 24 hours)
RingF tempSeries_hourly(24);
RingF humSeries_hourly(24);
RingF pressSeries_hourly(24);              // station pressure hPa
RingF gasSeries_hourly(24);                // gas kOhm

RingF slpTrend(SLP_TREND_POINTS);         // sea-level pressure trend (1/min)
uint32_t last_slp_trend_ms = 0;

// Hourly sampling for long-term history
const uint32_t HOURLY_SAMPLE_MS = 3600000;  // 1 hour in milliseconds
uint32_t last_hourly_sample_ms = 0;

// Request statistics
volatile uint32_t req_total = 0;
volatile uint32_t req_api = 0;
volatile uint32_t req_pages = 0;
volatile uint32_t req_errors = 0;
volatile uint32_t req_time_sum = 0;
volatile uint32_t loop_count = 0;
volatile uint32_t last_loop_rate_ms = 0;
volatile float loop_rate_hz = 0;

// NTP time sync tracking
static bool ntp_sync_attempted = false;
static bool ntp_sync_successful = false;
static bool sd_initialized = false;

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

    // Log to SD card (even if WiFi is disconnected) but only after time is synced
    // This prevents creating CSV files with invalid timestamps before NTP succeeds
    if (sd_info.initialized && ntp_sync_successful) {
      logSensorReading(now, t_raw, h_raw, p_raw_hpa, cached_slp,
                       cached_dp, cached_hi, cached_tend, cached_storm, g_raw_kohm);
    }
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

  // Hourly sampling for long-term history (24-hour window)
  if (now - last_hourly_sample_ms >= HOURLY_SAMPLE_MS) {
    last_hourly_sample_ms = now;
    // Push latest readings to hourly buffers
    tempSeries_hourly.push(t_raw);
    humSeries_hourly.push(h_raw);
    pressSeries_hourly.push(p_raw_hpa);
    gasSeries_hourly.push(g_raw_kohm);
  }
}

// Helper to extract date parameter from request line
// e.g., "GET /api/temp?date=2024-01-15 HTTP/1.1" -> "2024-01-15"
static bool extractDateParam(const String& reqLine, char* dateStr, int maxLen) {
  int qPos = reqLine.indexOf('?');
  if (qPos < 0) return false;

  int datePos = reqLine.indexOf("date=", qPos);
  if (datePos < 0) return false;

  int startPos = datePos + 5;
  int endPos = reqLine.indexOf(' ', startPos);
  if (endPos < 0) endPos = reqLine.length();

  int len = min(endPos - startPos, maxLen - 1);
  reqLine.substring(startPos, startPos + len).toCharArray(dateStr, len + 1);
  return len > 0;
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

// Hourly API endpoints (24-hour history)
static void sendJSONTempHourly(WiFiClient &c) {
  c.println("HTTP/1.1 200 OK");
  c.println("Content-Type: application/json; charset=utf-8");
  c.println("Cache-Control: no-store");
  c.println("Connection: close");
  c.println();
  c.print("{\"ok\":true,\"unit\":\"C\",\"interval_ms\":"); c.print(HOURLY_SAMPLE_MS);
  c.print(",\"series\":"); jsonWriteSeries(c, tempSeries_hourly, 2);
  c.println("}");
}

static void sendJSONHumidityHourly(WiFiClient &c) {
  c.println("HTTP/1.1 200 OK");
  c.println("Content-Type: application/json; charset=utf-8");
  c.println("Cache-Control: no-store");
  c.println("Connection: close");
  c.println();
  c.print("{\"ok\":true,\"unit\":\"%\",\"interval_ms\":"); c.print(HOURLY_SAMPLE_MS);
  c.print(",\"series\":"); jsonWriteSeries(c, humSeries_hourly, 2);
  c.println("}");
}

static void sendJSONPressureHourly(WiFiClient &c) {
  c.println("HTTP/1.1 200 OK");
  c.println("Content-Type: application/json; charset=utf-8");
  c.println("Cache-Control: no-store");
  c.println("Connection: close");
  c.println();
  c.print("{\"ok\":true,\"unit\":\"hPa\",\"interval_ms\":"); c.print(HOURLY_SAMPLE_MS);
  c.print(",\"series\":"); jsonWriteSeries(c, pressSeries_hourly, 2);
  c.println("}");
}

static void sendJSONGasHourly(WiFiClient &c) {
  c.println("HTTP/1.1 200 OK");
  c.println("Content-Type: application/json; charset=utf-8");
  c.println("Cache-Control: no-store");
  c.println("Connection: close");
  c.println();
  c.print("{\"ok\":true,\"unit\":\"kOhm\",\"interval_ms\":"); c.print(HOURLY_SAMPLE_MS);
  c.print(",\"series\":"); jsonWriteSeries(c, gasSeries_hourly, 2);
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

// Available dates endpoint - returns list of dates available in SD card
static void sendAvailableDates(WiFiClient &c) {
  c.println("HTTP/1.1 200 OK");
  c.println("Content-Type: application/json; charset=utf-8");
  c.println("Cache-Control: no-store");
  c.println("Connection: close");
  c.println();

  c.print("{\"ok\":true,\"dates\":[");

  if (sd_info.initialized) {
    File dataFile = sd.open("data.csv", FILE_READ);
    if (dataFile) {
      char line[256];
      char lastDate[11] = "";
      bool first = true;

      while (dataFile.available()) {
        int len = 0;
        while (dataFile.available() && len < 255) {
          char c = dataFile.read();
          if (c == '\n') break;
          line[len++] = c;
        }
        line[len] = '\0';

        if (len > 10) {
          // Extract date from timestamp (first column is timestamp_ms)
          // Convert timestamp_ms to date string
          uint32_t ts_ms = 0;
          for (int i = 0; i < len && line[i] != ','; i++) {
            ts_ms = ts_ms * 10 + (line[i] - '0');
          }

          uint32_t ts_sec = ts_ms / 1000;
          uint32_t days_since_epoch = ts_sec / 86400;

          // Simple date calculation (valid from 1970)
          uint16_t year = 1970;
          uint8_t month = 1, day = 1;
          uint16_t days_in_year[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};

          while (days_since_epoch >= 365) {
            if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
              if (days_since_epoch >= 366) {
                days_since_epoch -= 366;
                year++;
              } else break;
            } else {
              days_since_epoch -= 365;
              year++;
            }
          }

          for (int m = 11; m >= 0; m--) {
            uint16_t day_limit = days_in_year[m];
            if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
              if (m > 1) day_limit++;
            }
            if (days_since_epoch >= day_limit) {
              month = m + 1;
              day = days_since_epoch - day_limit + 1;
              break;
            }
          }

          char dateStr[11];
          snprintf(dateStr, 11, "%04u-%02u-%02u", year, month, day);

          if (strcmp(dateStr, lastDate) != 0) {
            if (!first) c.print(",");
            c.print("\""); c.print(dateStr); c.print("\"");
            strcpy(lastDate, dateStr);
            first = false;
          }
        }
      }
      dataFile.close();
    }
  }

  c.println("]}");
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

// Derived metrics API endpoint
static void sendJSONDerived(WiFiClient &c) {
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
  c.print("\"slp_hpa\":"); c.print(cached_slp,2); c.print(",");
  c.print("\"dew_point_c\":"); c.print(cached_dp,2); c.print(",");
  c.print("\"heat_index_c\":"); c.print(cached_hi,2); c.print(",");
  c.print("\"press_tendency_hpa_hr\":"); c.print(cached_tend,2); c.print(",");
  c.print("\"press_tendency\":\""); c.print(tendencyLabel(cached_tend)); c.print("\",");
  c.print("\"storm_score\":"); c.print(cached_storm);
  c.print("}}");
  c.println();
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

  // Estimate free RAM by measuring gap between heap and stack
  // Stack grows downward from high memory; measure current stack pointer
  static uint32_t minStackPointer = 0;
  char stackVar;
  uint32_t currentStackPointer = (uint32_t)&stackVar;

  // Track the lowest stack pointer seen (closest to heap)
  if (minStackPointer == 0 || currentStackPointer < minStackPointer) {
    minStackPointer = currentStackPointer;
  }

  // Estimate free RAM from heap to lowest stack pointer observed
  uint32_t estimatedHeapEnd = 0x20000000;  // Start of SRAM on Renesas RA4M1
  uint32_t freeRam = (minStackPointer > estimatedHeapEnd) ? (minStackPointer - estimatedHeapEnd) : 0;

  // Calculate average response time
  float avgMs = (req_total > 0) ? (float)req_time_sum / (float)req_total : 0;

  c.print("{\"ok\":true,");

  // Network section
  c.print("\"network\":{");
  c.print("\"connected\":"); c.print(WiFi.status() == WL_CONNECTED ? "true" : "false"); c.print(",");
  c.print("\"ip\":\""); c.print(ip); c.print("\",");
  c.print("\"ssid\":\""); c.print(ssid); c.print("\",");
  c.print("\"device_name\":\""); c.print(device_name); c.print("\",");
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
  c.print("},");

  // SD Card section
  c.print("\"sd_card\":{");
  c.print("\"initialized\":"); c.print(sd_info.initialized ? "true" : "false"); c.print(",");
  c.print("\"total_mb\":"); c.print(getSDTotalSpaceMB()); c.print(",");
  c.print("\"free_mb\":"); c.print(getSDFreeSpaceMB()); c.print(",");
  c.print("\"logged_samples\":"); c.print(getSDSampleCount());
  if (sd_info.error_msg[0] != '\0') {
    c.print(",\"error\":\""); c.print(sd_info.error_msg); c.print("\"");
  }
  c.print("}");

  c.println("}");
}

// Static assets
// Serve static files from SD card
static void sendStaticFile(WiFiClient &c, const char* filename, const char* mimeType) {
  // Try to open file from SD card
  SdFile file;
  if (!file.open(filename, O_RDONLY)) {
    c.println("HTTP/1.1 404 Not Found");
    c.println("Content-Type: text/plain; charset=utf-8");
    c.println("Connection: close");
    c.println();
    c.println("404");
    return;
  }

  c.println("HTTP/1.1 200 OK");
  c.print("Content-Type: ");
  c.println(mimeType);
  c.println("Cache-Control: public, max-age=31536000, immutable");
  c.println("Connection: close");
  c.println();

  // Stream file to client
  while (file.available()) {
    byte buf[256];
    int n = file.read(buf, sizeof(buf));
    if (n > 0) {
      c.write(buf, n);
    }
  }
  file.close();
}

static void sendStaticCSS(WiFiClient &c) {
  sendStaticFile(c, "app.css", "text/css; charset=utf-8");
}

static void sendStaticJS(WiFiClient &c) {
  sendStaticFile(c, "app.js", "application/javascript; charset=utf-8");
}

static void sendStaticFavicon(WiFiClient &c) {
  sendStaticFile(c, "favicon.svg", "image/svg+xml");
}

// Basic 404
static void send404(WiFiClient &c) {
  c.println("HTTP/1.1 404 Not Found");
  c.println("Content-Type: text/plain; charset=utf-8");
  c.println("Connection: close");
  c.println();
  c.println("404");
}

// ============ PER-DATE DATA FETCH ============
// Fetch sensor data for a specific date (YYYY-MM-DD) and return as JSON
static void sendJSONDateData(WiFiClient &c, const String& dateStr, const char* metric) {
  c.println("HTTP/1.1 200 OK");
  c.println("Content-Type: application/json; charset=utf-8");
  c.println("Cache-Control: no-store");
  c.println("Connection: close");
  c.println();

  // Try to open the daily CSV file
  char filename[32];
  dateStr.toCharArray(filename, sizeof(filename) - 5);
  strcat(filename, ".csv");

  if (!logFile.open(filename, O_RDONLY)) {
    c.print("{\"ok\":false,\"error\":\"Date not found\"}");
    return;
  }

  // Log file access
  Serial.print("[API] Loading ");
  Serial.print(filename);
  Serial.print(" for ");
  Serial.println(metric);

  // Determine which field to extract based on metric
  int metric_field = -1;
  if (strcmp(metric, "temp") == 0) metric_field = 1;
  else if (strcmp(metric, "humidity") == 0) metric_field = 2;
  else if (strcmp(metric, "pressure") == 0) metric_field = 3;
  else if (strcmp(metric, "gas") == 0) metric_field = 9;

  if (metric_field < 0) {
    logFile.close();
    c.print("{\"ok\":false,\"error\":\"Invalid metric\"}");
    return;
  }

  c.print("{\"ok\":true,\"unit\":");
  if (metric_field == 1) c.print("\"C\"");
  else if (metric_field == 2) c.print("\"%\"");
  else if (metric_field == 3) c.print("\"hPa\"");
  else c.print("\"kOhm\"");
  c.print(",\"interval_ms\":1000,\"data\":[");

  // Parse and output data with timestamps
  char line[128];
  int line_len = 0;
  bool first_data = true;
  int line_count = 0;

  while (logFile.available() && line_count < 100000) {  // Safety limit
    int c_byte = logFile.read();

    if (c_byte == '\n' || c_byte == -1) {
      if (line_len > 0) {
        line[line_len] = '\0';

        // Skip header
        if (line[0] < '0' || line[0] > '9') {
          line_len = 0;
          if (c_byte == -1) break;
          continue;
        }

        // Parse line to extract timestamp and metric value
        unsigned long ts = 0;
        float value = NAN;
        int field = 0;
        int start = 0;
        char field_str[32];

        for (int i = 0; i <= line_len; i++) {
          if (line[i] == ',' || line[i] == '\0') {
            int len = i - start;
            if (len > 0 && len < (int)sizeof(field_str)) {
              strncpy(field_str, &line[start], len);
              field_str[len] = '\0';

              if (field == 0) {
                ts = strtoul(field_str, NULL, 10);
              } else if (field == metric_field) {
                value = strtof(field_str, NULL);
                break;  // Got what we need
              }
            }
            field++;
            start = i + 1;
          }
        }

        // Output as [timestamp, value] pair
        if (isfinite(value)) {
          if (!first_data) c.print(",");
          c.print("[");
          c.print(ts);
          c.print(",");
          c.print(value, 2);
          c.print("]");
          first_data = false;
        }

        line_count++;
      }

      line_len = 0;
      if (c_byte == -1) break;
    } else if (c_byte >= 32 && c_byte < 127) {
      if (line_len < sizeof(line) - 1) {
        line[line_len++] = c_byte;
      }
    }
  }

  logFile.close();
  c.println("]}");
}

// ============ LOAD HISTORY FROM CSV (implementation) ============
// Helper to load and parse a single CSV file
static uint32_t loadCSVFile(const char* filename,
                            RingF &tempSeries, RingF &humSeries, RingF &pressSeries,
                            RingF &gasSeries, RingF &slpTrend) {
  if (!logFile.open(filename, O_RDONLY)) {
    return 0;
  }

  uint32_t sample_count = 0;
  uint32_t skipped_lines = 0;
  char line[128];
  int line_len = 0;
  bool first_line = true;

  // Read and parse each line
  while (logFile.available()) {
    int c = logFile.read();

    if (c == '\n' || c == -1) {
      if (line_len > 0) {
        line[line_len] = '\0';

        // Skip header line (if first line contains non-numeric data)
        if (first_line) {
          first_line = false;
          if (line[0] < '0' || line[0] > '9') {
            skipped_lines++;
            line_len = 0;
            if (c == -1) break;
            continue;
          }
        }

        // Parse CSV
        unsigned long ts = 0;
        float temp = NAN, hum = NAN, press_station = NAN, press_slp = NAN;
        float dp = NAN, hi = NAN, tend = NAN, storm = NAN, gas = NAN;

        int field = 0;
        int start = 0;
        char field_str[32];
        int parsed = 0;

        for (int i = 0; i <= line_len; i++) {
          if (line[i] == ',' || line[i] == '\0') {
            int len = i - start;
            if (len > 0 && len < (int)sizeof(field_str)) {
              strncpy(field_str, &line[start], len);
              field_str[len] = '\0';

              switch(field) {
                case 0: ts = strtoul(field_str, NULL, 10); parsed++; break;
                case 1: temp = strtof(field_str, NULL); parsed++; break;
                case 2: hum = strtof(field_str, NULL); parsed++; break;
                case 3: press_station = strtof(field_str, NULL); parsed++; break;
                case 4: press_slp = strtof(field_str, NULL); parsed++; break;
                case 5: dp = strtof(field_str, NULL); parsed++; break;
                case 6: hi = strtof(field_str, NULL); parsed++; break;
                case 7: tend = strtof(field_str, NULL); parsed++; break;
                case 8: storm = strtof(field_str, NULL); parsed++; break;
                case 9: gas = strtof(field_str, NULL); parsed++; break;
              }
            }
            field++;
            start = i + 1;
          }
        }

        if (field == 5) {  // Old format
          gas = press_slp;
          press_slp = press_station;
        }

        if (parsed >= 4 && isfinite(temp) && isfinite(hum) && isfinite(press_station)) {
          tempSeries.push(temp);
          humSeries.push(hum);
          pressSeries.push(press_station);
          if (isfinite(gas)) {
            gasSeries.push(gas);
          }
          if (field >= 5 && isfinite(press_slp)) {
            slpTrend.push(press_slp);
          }
          sample_count++;
        } else {
          skipped_lines++;
        }
      }

      line_len = 0;
      if (c == -1) break;
    } else if (c >= 32 && c < 127) {
      if (line_len < sizeof(line) - 1) {
        line[line_len++] = c;
      }
    }
  }

  logFile.close();
  return sample_count;
}

// ============ LIST CSV FILES ============
// List all YYYY-MM-DD.csv files on SD card in alphabetical order
void listCSVFiles() {
  if (!sd_info.initialized) {
    return;
  }

  Serial.println("\n=== Available CSV Data Files ===");

  // Open root directory
  SdFile root;
  if (!root.open("/")) {
    Serial.println("Failed to open root directory");
    return;
  }

  // Collect all CSV filenames
  char filenames[100][11];  // Store up to 100 filenames, max 10 chars each (YYYY-MM-DD.csv = 14 chars, but we'll keep it safe)
  int file_count = 0;

  SdFile file;
  while (file.openNext(&root, O_RDONLY) && file_count < 100) {
    char name[32];
    file.getName(name, sizeof(name));

    // Check if filename matches YYYY-MM-DD.csv pattern (14 chars exactly)
    if (strlen(name) == 14 && name[4] == '-' && name[7] == '-' &&
        name[10] == '.' && name[11] == 'c' && name[12] == 's' && name[13] == 'v') {
      strncpy(filenames[file_count], name, 10);
      filenames[file_count][10] = '\0';
      file_count++;
    }
    file.close();
  }
  root.close();

  if (file_count == 0) {
    Serial.println("No per-day CSV files found yet");
  } else {
    // Sort filenames alphabetically (simple bubble sort for small lists)
    for (int i = 0; i < file_count - 1; i++) {
      for (int j = i + 1; j < file_count; j++) {
        if (strcmp(filenames[i], filenames[j]) > 0) {
          char temp[11];
          strcpy(temp, filenames[i]);
          strcpy(filenames[i], filenames[j]);
          strcpy(filenames[j], temp);
        }
      }
    }

    // Print sorted list
    for (int i = 0; i < file_count; i++) {
      Serial.print("  [");
      Serial.print(i + 1);
      Serial.print("] ");
      Serial.println(filenames[i]);
    }
  }
  Serial.println("===================================\n");
}

// Load historical data from per-day CSV files or legacy data.csv
bool loadHistoryFromSD(RingF &tempSeries, RingF &humSeries, RingF &pressSeries,
                       RingF &gasSeries, RingF &slpTrend) {
  if (!sd_info.initialized) {
    Serial.println("SD card not initialized");
    return false;
  }

  // Per-day CSV files (YYYY-MM-DD.csv) are loaded on-demand via API endpoints
  // This avoids loading all historical data at startup
  // Legacy data.csv support: if it exists, load it into ring buffers for live display

  uint32_t total_samples = 0;

  if (sd.exists("data.csv")) {
    Serial.println("Found legacy data.csv - loading for live display...");
    uint32_t samples = loadCSVFile("data.csv", tempSeries, humSeries, pressSeries, gasSeries, slpTrend);
    total_samples += samples;
    Serial.print("Loaded ");
    Serial.print(samples);
    Serial.println(" samples from legacy data.csv");
  } else {
    Serial.println("Per-day CSV files ready for on-demand loading (YYYY-MM-DD.csv)");
  }

  sd_info.logged_samples = total_samples;

  // List all available CSV files
  listCSVFiles();

  return true;
}

// ============ NTP TIME CONFIGURATION ============
// Configure time via NTP and set the internal RTC
static bool configureNTPTime() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[Time] WiFi not connected, cannot sync time");
    return false;
  }

  Serial.println("[Time] Syncing time via NTP...");

  // Initialize NTP client and wait for first update
  timeClient.begin();

  // Wait for NTP update (up to 10 seconds with 100ms retries)
  int attempts = 0;
  while (!timeClient.update() && attempts < 100) {
    delay(100);
    attempts++;
  }

  if (attempts >= 100) {
    Serial.println("[Time] NTP sync failed - timeout");
    return false;
  }

  // Get Unix timestamp from NTP
  unsigned long unixTime = timeClient.getEpochTime();
  Serial.print("[Time] Got NTP timestamp: ");
  Serial.println(unixTime);

  // Set the internal RTC with the NTP time
  RTC.begin();
  RTCTime timeToSet = RTCTime(unixTime);
  RTC.setTime(timeToSet);

  // Set timezone for correct local time display
  setenv("TZ", timezone_str, 1);
  tzset();

  // Verify RTC was set correctly
  RTCTime currentTime;
  RTC.getTime(currentTime);
  Serial.print("[Time] RTC set successfully. Current time: ");
  Serial.println(String(currentTime));

  return true;
}

// Get current date as YYYY-MM-DD string (from RTC)
void getCurrentDateString(char* dateStr, int maxLen) {
  RTCTime currentTime;
  RTC.getTime(currentTime);

  // Format: YYYY-MM-DD
  snprintf(dateStr, maxLen, "%04d-%02d-%02d",
           currentTime.getYear(),
           Month2int(currentTime.getMonth()),
           currentTime.getDayOfMonth());
}

// Get current timestamp in milliseconds (real clock time, not millis())
static unsigned long getCurrentTimeMs() {
  RTCTime currentTime;
  RTC.getTime(currentTime);

  // Convert RTCTime to Unix timestamp
  struct tm tm_info;
  memset(&tm_info, 0, sizeof(struct tm));

  tm_info.tm_sec = currentTime.getSeconds();
  tm_info.tm_min = currentTime.getMinutes();
  tm_info.tm_hour = currentTime.getHour();
  tm_info.tm_mday = currentTime.getDayOfMonth();
  tm_info.tm_mon = Month2int(currentTime.getMonth()) - 1;  // tm_mon is 0-11
  tm_info.tm_year = currentTime.getYear() - 1900;  // tm_year is years since 1900
  tm_info.tm_isdst = -1;  // Let mktime determine DST

  time_t unixTime = mktime(&tm_info);
  return (unsigned long)unixTime * 1000;  // Convert seconds to milliseconds
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {}

  // Initialize WiFi manager for non-blocking connection with exponential backoff
  initWiFiManager();

  // Set hostname BEFORE attempting WiFi connection
  WiFi.setHostname(device_name);
  Serial.print("[Setup] WiFi hostname set to: ");
  Serial.println(device_name);

  Serial.println("Connecting to WiFi (non-blocking)...");
  WiFi.begin(ssid, pass);  // Initiate connection attempt
  // Connection happens in background; status checked periodically in loop()

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

  // SD card initialization is deferred until after NTP time sync succeeds
  // This ensures files are created with correct timestamps

  last_env_ms = millis() - ENV_INTERVAL_MS;
  last_gas_ms = millis() - GAS_INTERVAL_MS;
  last_slp_trend_ms = millis() - SLP_TREND_SAMPLE_MS;
  last_hourly_sample_ms = millis() - HOURLY_SAMPLE_MS;

  // Prime
  updateSampling();

  // Initialize SD card before starting server (needed for static asset files)
  Serial.println("Initializing SD card...");
  if (initSDCard()) {
    // Load historical data from CSV into ring buffers
    loadHistoryFromSD(tempSeries, humSeries, pressSeries, gasSeries, slpTrend);
    sd_initialized = true;

    // Check for required static asset files
    SdFile test_file;
    bool has_css = test_file.open("app.css", O_RDONLY);
    if (has_css) test_file.close();

    bool has_js = test_file.open("app.js", O_RDONLY);
    if (has_js) test_file.close();

    bool has_favicon = test_file.open("favicon.svg", O_RDONLY);
    if (has_favicon) test_file.close();

    if (!has_css || !has_js || !has_favicon) {
      Serial.println("ERROR: Missing required static files on SD card!");
      if (!has_css) Serial.println("  - app.css is missing");
      if (!has_js) Serial.println("  - app.js is missing");
      if (!has_favicon) Serial.println("  - favicon.svg is missing");
      Serial.println("Please copy app.css, app.js, and favicon.svg to SD card root");
      Serial.println("Web server will NOT start until files are present");
      while (1) {
        delay(1000);
        Serial.println("Waiting for static files...");
      }
    }
  } else {
    Serial.println("ERROR: SD card initialization failed!");
    Serial.println("Cannot start web server without SD card");
    while (1) {
      delay(1000);
      Serial.println("Waiting for SD card...");
    }
  }

  server.begin();
  Serial.println("Web server started");
  Serial.print("Open browser to: http://");
  Serial.println(WiFi.localIP());
}

void loop() {
  updateSampling();

  // Check WiFi status and attempt reconnection if needed
  updateWiFiStatus(ssid, pass);

  // Sync time via NTP once WiFi is connected (happens once)
  if (!ntp_sync_attempted && WiFi.status() == WL_CONNECTED) {
    ntp_sync_attempted = true;
    ntp_sync_successful = configureNTPTime();
  }

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
  if (reqLine.startsWith("GET /static/app.css")) {
    sendStaticCSS(client);
    isPage = true;
  } else if (reqLine.startsWith("GET /static/app.js")) {
    sendStaticJS(client);
    isPage = true;
  } else if (reqLine.startsWith("GET /static/favicon.svg") || reqLine.startsWith("GET /favicon.ico")) {
    sendStaticFavicon(client);
    isPage = true;
  } else if (reqLine.startsWith("GET /graphs/1m")) {
    sendPageGraphs1m(client);
    isPage = true;
  } else if (reqLine.startsWith("GET /graphs/5m")) {
    sendPageGraphs5m(client);
    isPage = true;
  } else if (reqLine.startsWith("GET /graphs/24h")) {
    sendPageGraphs24h(client);
    isPage = true;
  } else if (reqLine.startsWith("GET /graphs")) {
    // Default graphs page is 10 minutes
    sendPageGraphs10m(client);
    isPage = true;
  } else if (reqLine.startsWith("GET /comfort")) {
    sendPageComfort(client);
    isPage = true;
  } else if (reqLine.startsWith("GET /derived")) {
    sendPageDerived(client);
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
  } else if (reqLine.startsWith("GET /api/derived")) {
    sendJSONDerived(client);
    isApi = true;
  } else if (reqLine.startsWith("GET /api/stats")) {
    sendJSONStats(client);
    isApi = true;
  } else if (reqLine.startsWith("GET /api/available-dates")) {
    sendAvailableDates(client);
    isApi = true;
  } else if (reqLine.startsWith("GET /api/temp-date")) {
    // Date-specific temperature: /api/temp-date?date=YYYY-MM-DD
    char dateStr[16] = "";
    if (extractDateParam(reqLine, dateStr, sizeof(dateStr))) {
      sendJSONDateData(client, String(dateStr), "temp");
    } else {
      send404(client);
      isError = true;
    }
    isApi = true;
  } else if (reqLine.startsWith("GET /api/humidity-date")) {
    // Date-specific humidity: /api/humidity-date?date=YYYY-MM-DD
    char dateStr[16] = "";
    if (extractDateParam(reqLine, dateStr, sizeof(dateStr))) {
      sendJSONDateData(client, String(dateStr), "humidity");
    } else {
      send404(client);
      isError = true;
    }
    isApi = true;
  } else if (reqLine.startsWith("GET /api/pressure-date")) {
    // Date-specific pressure: /api/pressure-date?date=YYYY-MM-DD
    char dateStr[16] = "";
    if (extractDateParam(reqLine, dateStr, sizeof(dateStr))) {
      sendJSONDateData(client, String(dateStr), "pressure");
    } else {
      send404(client);
      isError = true;
    }
    isApi = true;
  } else if (reqLine.startsWith("GET /api/gas-date")) {
    // Date-specific gas: /api/gas-date?date=YYYY-MM-DD
    char dateStr[16] = "";
    if (extractDateParam(reqLine, dateStr, sizeof(dateStr))) {
      sendJSONDateData(client, String(dateStr), "gas");
    } else {
      send404(client);
      isError = true;
    }
    isApi = true;
  } else if (reqLine.startsWith("GET /api/temp-hourly")) {
    sendJSONTempHourly(client);
    isApi = true;
  } else if (reqLine.startsWith("GET /api/temp")) {
    sendJSONTemp(client);
    isApi = true;
  } else if (reqLine.startsWith("GET /api/humidity-hourly")) {
    sendJSONHumidityHourly(client);
    isApi = true;
  } else if (reqLine.startsWith("GET /api/humidity")) {
    sendJSONHumidity(client);
    isApi = true;
  } else if (reqLine.startsWith("GET /api/pressure-hourly")) {
    sendJSONPressureHourly(client);
    isApi = true;
  } else if (reqLine.startsWith("GET /api/pressure")) {
    sendJSONPressure(client);
    isApi = true;
  } else if (reqLine.startsWith("GET /api/gas-hourly")) {
    sendJSONGasHourly(client);
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
