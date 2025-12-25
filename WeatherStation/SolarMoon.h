#pragma once
#include <Arduino.h>
#include <math.h>

struct SunTimes {
  bool valid;
  int sunriseMin;  // minutes from midnight local
  int sunsetMin;
  int solarNoonMin;
  int dayLengthMin;
};

inline double deg2rad(double d) { return d * M_PI / 180.0; }
inline double rad2deg(double r) { return r * 180.0 / M_PI; }

inline int clampMin(int v) { if (v < 0) return 0; if (v > 24*60) return 24*60; return v; }

// Approx NOAA sunrise/sunset. Good enough for a comfort page.
inline SunTimes computeSunTimes(int year, int month, int day, double latDeg, double lonDeg, int tzOffsetMinutes) {
  // Julian day
  int a = (14 - month) / 12;
  int y = year + 4800 - a;
  int m = month + 12*a - 3;
  long JDN = day + (153*m + 2)/5 + 365L*y + y/4 - y/100 + y/400 - 32045;
  double jd = JDN + 0.5; // noon

  double n = jd - 2451545.0;
  double meanLong = fmod(280.460 + 0.9856474 * n, 360.0);
  if (meanLong < 0) meanLong += 360.0;
  double meanAnom = fmod(357.528 + 0.9856003 * n, 360.0);
  if (meanAnom < 0) meanAnom += 360.0;

  double lambda = meanLong + 1.915 * sin(deg2rad(meanAnom)) + 0.020 * sin(2 * deg2rad(meanAnom));
  double epsilon = 23.439 - 0.0000004 * n;

  double decl = rad2deg(asin(sin(deg2rad(epsilon)) * sin(deg2rad(lambda))));
  double eqTime = meanLong - 0.0057183 - lambda + 2.466 * sin(2*deg2rad(lambda)) - 0.053 * sin(4*deg2rad(lambda));
  eqTime = 4.0 * eqTime; // minutes
  // normalize
  while (eqTime > 720) eqTime -= 1440;
  while (eqTime < -720) eqTime += 1440;

  // Solar zenith for “official sunrise/sunset”: 90.833°
  double zenith = 90.833;
  double lat = latDeg;

  double cosH =
    (cos(deg2rad(zenith)) - sin(deg2rad(lat)) * sin(deg2rad(decl))) /
    (cos(deg2rad(lat)) * cos(deg2rad(decl)));

  SunTimes out;
  out.valid = (cosH >= -1.0 && cosH <= 1.0);
  if (!out.valid) {
    out.sunriseMin = out.sunsetMin = out.solarNoonMin = out.dayLengthMin = 0;
    return out;
  }

  double H = rad2deg(acos(cosH)); // degrees
  double solarNoon = (720 - 4.0 * lonDeg - eqTime + tzOffsetMinutes); // minutes
  double sunrise = solarNoon - 4.0 * H;
  double sunset  = solarNoon + 4.0 * H;

  out.solarNoonMin = clampMin((int)lround(solarNoon));
  out.sunriseMin   = clampMin((int)lround(sunrise));
  out.sunsetMin    = clampMin((int)lround(sunset));
  out.dayLengthMin = out.sunsetMin - out.sunriseMin;
  if (out.dayLengthMin < 0) out.dayLengthMin += 24*60;
  return out;
}

inline const char* moonPhaseName(double phase01) {
  // phase: 0=new, 0.5=full
  if (phase01 < 0.03 || phase01 > 0.97) return "New Moon";
  if (phase01 < 0.22) return "Waxing Crescent";
  if (phase01 < 0.28) return "First Quarter";
  if (phase01 < 0.47) return "Waxing Gibbous";
  if (phase01 < 0.53) return "Full Moon";
  if (phase01 < 0.72) return "Waning Gibbous";
  if (phase01 < 0.78) return "Last Quarter";
  return "Waning Crescent";
}

// Simple moon phase approximation from unix time.
// Good for “comfort page” display.
inline double moonPhase01(uint32_t unixTime) {
  // Known new moon reference: 2000-01-06 18:14 UTC ~ 947182440
  const double ref = 947182440.0;
  const double synodic = 29.530588853 * 86400.0;
  double age = fmod((double)unixTime - ref, synodic);
  if (age < 0) age += synodic;
  return age / synodic; // 0..1
}

inline void formatHHMM(char* out, size_t outLen, int minutesFromMidnight) {
  int h = (minutesFromMidnight / 60) % 24;
  int m = minutesFromMidnight % 60;
  snprintf(out, outLen, "%02d:%02d", h, m);
}

inline void formatDuration(char* out, size_t outLen, int totalMin) {
  int h = totalMin / 60;
  int m = totalMin % 60;
  snprintf(out, outLen, "%dh %02dm", h, m);
}
