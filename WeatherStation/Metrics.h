#pragma once
#include <Arduino.h>
#include <math.h>

class MetricsClock {
public:
  static void setUnix(uint32_t unixNow) {
    s_unixBase = unixNow;
    s_msBase = millis();
    s_initialized = true;
  }

  static uint32_t now() {
    if (!s_initialized) return 0;
    uint32_t delta = (millis() - s_msBase) / 1000UL;
    return s_unixBase + delta;
  }

  static uint32_t tickToNowUnix() {
    // keeps monotonic-ish time
    uint32_t t = now();
    if (t == 0) return 0;
    if (t < s_last) t = s_last; // prevent backwards
    s_last = t;
    return t;
  }

private:
  static inline bool s_initialized = false;
  static inline uint32_t s_unixBase = 0;
  static inline uint32_t s_msBase = 0;
  static inline uint32_t s_last = 0;
};

inline float calcSeaLevelPressureHpa(float stationHpa, float altitudeM) {
  // Standard atmosphere approximation
  // p0 = p / (1 - h/44330) ^ 5.255
  float x = 1.0f - (altitudeM / 44330.0f);
  if (x < 0.01f) x = 0.01f;
  return stationHpa / powf(x, 5.255f);
}

inline float dewPointC(float tempC, float rh) {
  // Magnus formula
  const float a = 17.62f;
  const float b = 243.12f;
  float gamma = (a * tempC) / (b + tempC) + logf(rh / 100.0f);
  return (b * gamma) / (a - gamma);
}

inline float heatIndexC(float tempC, float rh) {
  // NOAA-ish; valid mainly when hot. We’ll return temp for cool conditions.
  if (tempC < 26.0f) return tempC;

  float tF = tempC * 9.0f / 5.0f + 32.0f;
  float hiF =
    -42.379f + 2.04901523f * tF + 10.14333127f * rh
    - 0.22475541f * tF * rh - 0.00683783f * tF * tF
    - 0.05481717f * rh * rh + 0.00122874f * tF * tF * rh
    + 0.00085282f * tF * rh * rh - 0.00000199f * tF * tF * rh * rh;

  return (hiF - 32.0f) * 5.0f / 9.0f;
}

enum class PressureTrend {
  Rising,
  Falling,
  Steady
};

inline const char* pressureTrendLabel(PressureTrend t) {
  switch (t) {
    case PressureTrend::Rising:  return "Rising";
    case PressureTrend::Falling: return "Falling";
    default: return "Steady";
  }
}

struct EnvSample {
  uint32_t t;
  float tempC;
  float humPct;
  float pressHpa;
  float seaLevelHpa;
};

struct GasSample {
  uint32_t t;
  float gasOhms;
};

class EnvHistory {
public:
  explicit EnvHistory(uint16_t cap)
  : m_cap(cap), m_head(0), m_count(0) {
    m_buf = (EnvSample*)malloc(sizeof(EnvSample) * m_cap);
  }

  ~EnvHistory() {
    if (m_buf) free(m_buf);
  }

  void push(uint32_t t, float tempC, float humPct, float pressHpa, float seaLevelHpa) {
    if (!m_buf) return;
    m_buf[m_head] = { t, tempC, humPct, pressHpa, seaLevelHpa };
    m_head = (m_head + 1) % m_cap;
    if (m_count < m_cap) m_count++;
  }

  uint16_t count() const { return m_count; }
  uint16_t cap() const { return m_cap; }

  EnvSample getFromOldest(uint16_t idx) const {
    // idx=0 is oldest
    if (m_count == 0) return {0, NAN, NAN, NAN, NAN};
    if (idx >= m_count) idx = m_count - 1;
    uint16_t oldest = (m_head + m_cap - m_count) % m_cap;
    uint16_t pos = (oldest + idx) % m_cap;
    return m_buf[pos];
  }

  EnvSample latest() const {
    if (m_count == 0) return {0, NAN, NAN, NAN, NAN};
    uint16_t lastPos = (m_head + m_cap - 1) % m_cap;
    return m_buf[lastPos];
  }

  // Moving average over last N seconds (best-effort)
  float avgTempLast(uint32_t seconds) const { return avgField(seconds, 0); }
  float avgHumLast(uint32_t seconds)  const { return avgField(seconds, 1); }
  float avgSeaLast(uint32_t seconds)  const { return avgField(seconds, 2); }

  PressureTrend pressureTendency(uint32_t lookbackSeconds, float* outDeltaHpaPerHr) const {
    if (m_count < 2) {
      if (outDeltaHpaPerHr) *outDeltaHpaPerHr = 0;
      return PressureTrend::Steady;
    }

    EnvSample now = latest();
    // Find sample closest to lookbackSeconds ago
    uint32_t target = (now.t > lookbackSeconds) ? (now.t - lookbackSeconds) : 0;

    EnvSample past = getFromOldest(0);
    for (uint16_t i = 0; i < m_count; i++) {
      EnvSample s = getFromOldest(i);
      if (s.t >= target) { past = s; break; }
      past = s;
    }

    float dt = float(now.t - past.t);
    if (dt < 1.0f) dt = 1.0f;

    float delta = now.seaLevelHpa - past.seaLevelHpa; // hPa over dt seconds
    float perHr = delta * (3600.0f / dt);

    if (outDeltaHpaPerHr) *outDeltaHpaPerHr = perHr;

    if (perHr > 0.2f) return PressureTrend::Rising;
    if (perHr < -0.2f) return PressureTrend::Falling;
    return PressureTrend::Steady;
  }

  // “Trend points”: absolute hPa/hour rate scaled a bit, always numeric (no "--")
  float trendPoints(uint32_t lookbackSeconds) const {
    float perHr = 0;
    pressureTendency(lookbackSeconds, &perHr);
    return fabsf(perHr);
  }

private:
  // field: 0 temp, 1 hum, 2 seaLevel
  float avgField(uint32_t seconds, int field) const {
    if (m_count == 0) return NAN;
    EnvSample now = latest();
    uint32_t minT = (now.t > seconds) ? (now.t - seconds) : 0;

    double sum = 0;
    uint32_t n = 0;

    for (int i = int(m_count) - 1; i >= 0; i--) {
      EnvSample s = getFromOldest(i);
      if (s.t < minT) break;
      float v = NAN;
      if (field == 0) v = s.tempC;
      else if (field == 1) v = s.humPct;
      else v = s.seaLevelHpa;

      if (!isnan(v)) { sum += v; n++; }
    }

    if (n == 0) return NAN;
    return float(sum / double(n));
  }

  EnvSample* m_buf;
  uint16_t m_cap;
  uint16_t m_head;
  uint16_t m_count;
};

class GasHistory {
public:
  explicit GasHistory(uint16_t cap)
  : m_cap(cap), m_head(0), m_count(0) {
    m_buf = (GasSample*)malloc(sizeof(GasSample) * m_cap);
  }

  ~GasHistory() {
    if (m_buf) free(m_buf);
  }

  void push(uint32_t t, float gasOhms) {
    if (!m_buf) return;
    m_buf[m_head] = { t, gasOhms };
    m_head = (m_head + 1) % m_cap;
    if (m_count < m_cap) m_count++;
  }

  uint16_t count() const { return m_count; }
  uint16_t cap() const { return m_cap; }

  GasSample getFromOldest(uint16_t idx) const {
    if (m_count == 0) return {0, NAN};
    if (idx >= m_count) idx = m_count - 1;
    uint16_t oldest = (m_head + m_cap - m_count) % m_cap;
    uint16_t pos = (oldest + idx) % m_cap;
    return m_buf[pos];
  }

  GasSample latest() const {
    if (m_count == 0) return {0, NAN};
    uint16_t lastPos = (m_head + m_cap - 1) % m_cap;
    return m_buf[lastPos];
  }

  float avgGasLast(uint32_t seconds) const {
    if (m_count == 0) return NAN;
    GasSample now = latest();
    uint32_t minT = (now.t > seconds) ? (now.t - seconds) : 0;

    double sum = 0;
    uint32_t n = 0;

    for (int i = int(m_count) - 1; i >= 0; i--) {
      GasSample s = getFromOldest(i);
      if (s.t < minT) break;
      if (!isnan(s.gasOhms)) { sum += s.gasOhms; n++; }
    }

    if (n == 0) return NAN;
    return float(sum / double(n));
  }

private:
  GasSample* m_buf;
  uint16_t m_cap;
  uint16_t m_head;
  uint16_t m_count;
};
