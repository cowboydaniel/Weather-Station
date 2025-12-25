#include <Wire.h>
#include <WiFiS3.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME680.h>
#include "RTC.h"

#include "arduino_secrets.h"

#include "WebRoutes.h"            // routing + handlers (no HTML here)
#include "Metrics.h"              // moving averages + derived calculations
#include "SolarMoon.h"            // sunrise/sunset + moon phase (offline)
#include "Pages_Dashboard.h"      // HTML templates
#include "Pages_GraphTemp.h"
#include "Pages_GraphHumidity.h"
#include "Pages_GraphPressure.h"
#include "Pages_GraphGas.h"
#include "Pages_Comfort.h"
#include "Favicon.h"

static const float ALTITUDE_M = 242.0f;

// Sampling
static const uint32_t SAMPLE_ENV_MS = 1000;   // temp/hum/press
static const uint32_t SAMPLE_GAS_MS = 3000;   // gas

// History sizes (graph pages)
static const uint16_t HIST_ENV_POINTS = 900;  // 15 minutes @ 1s
static const uint16_t HIST_GAS_POINTS = 600;  // 30 minutes @ 3s

// NTP sync cadence (RTC is not super accurate, so resync occasionally)
static const uint32_t NTP_RESYNC_MS = 30UL * 60UL * 1000UL; // 30 minutes

WiFiServer server(80);
WiFiUDP udp;
NTPClient ntp(udp, "pool.ntp.org", 0, 10UL * 60UL * 1000UL); // update() when asked; internal interval not relied on

Adafruit_BME680 bme; // I2C

// Metrics + history
EnvHistory envHist(HIST_ENV_POINTS);
GasHistory gasHist(HIST_GAS_POINTS);

uint32_t lastEnvMs = 0;
uint32_t lastGasMs = 0;
uint32_t lastNtpSyncMs = 0;

int wifiStatus = WL_IDLE_STATUS;

static void printWiFiStatus() {
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  IPAddress ip = WiFi.localIP();
  Serial.print("IP address: ");
  Serial.println(ip);

  long rssi = WiFi.RSSI();
  Serial.print("RSSI: ");
  Serial.print(rssi);
  Serial.println(" dBm");
}

static bool connectWiFi() {
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("WiFi module not found.");
    return false;
  }

  String fv = WiFi.firmwareVersion();
  Serial.print("WiFi firmware: ");
  Serial.println(fv);

  while (wifiStatus != WL_CONNECTED) {
    Serial.println("Connecting to WiFi...");
    wifiStatus = WiFi.begin(SECRET_SSID, SECRET_PASS);
    delay(1500);
    yield();  // Allow WiFi stack to process during connection
  }

  Serial.println("WiFi connected");
  printWiFiStatus();
  return true;
}

static bool initBME680() {
  if (!bme.begin()) {
    Serial.println("BME680 not found on I2C. Check wiring/address.");
    return false;
  }

  // “Accurate but not ridiculous” settings:
  // - Temp: x8, Pressure: x4, Humidity: x2
  // - IIR filter: 3 (smooths short spikes)
  // - Gas heater: 320C for 150ms (typical general-purpose profile)
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);

  // Gas: for BSEC-style IAQ you normally need Bosch’s BSEC library.
  // Here we just expose raw resistance (still useful), sampled every 3s.
  bme.setGasHeater(320, 150);

  return true;
}

static bool syncRTCFromNTP() {
  Serial.println("Syncing RTC from NTP...");
  ntp.begin();

  // Ensure WiFi is up
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("NTP sync skipped: WiFi not connected.");
    return false;
  }

  if (!ntp.update()) {
    // Try a couple quick retries (NTP can be flaky on hotspots)
    for (int i = 0; i < 3; i++) {
      delay(250);
      yield();  // Allow WiFi to process during blocking delay
      if (ntp.update()) break;
    }
  }

  unsigned long unixTime = ntp.getEpochTime();
  if (unixTime < 1700000000UL) { // sanity check (roughly late 2023+)
    Serial.print("NTP gave suspicious time: ");
    Serial.println(unixTime);
    return false;
  }

  RTC.begin();
  RTCTime t = RTCTime(unixTime);
  RTC.setTime(t);

  RTCTime now;
  RTC.getTime(now);
  Serial.println("RTC set to: " + String(now));
  lastNtpSyncMs = millis();
  return true;
}

static uint32_t nowUnix() {
  // RTC read
  RTCTime now;
  RTC.getTime(now);
  // RTCTime has a constructor from unix; but we need unix out.
  // Arduino’s RTC API doesn’t always expose “getEpoch” cleanly,
  // so we keep our own unix counter using millis deltas if needed.
  // HOWEVER: we can approximate using NTPClient cache if RTC is unset.
  // For practicality: use NTP epoch if RTC looks unset (year 2000 default).
  String s = String(now);
  // If RTC is not started, it commonly sits near 2000-01-01.
  // We detect that by string prefix. Ugly but works reliably on these cores.
  if (s.startsWith("2000-01-01") || s.startsWith("2020-01-01")) {
    return ntp.getEpochTime();
  }

  // Best-effort: rebuild unix by reading RTC again after setting from NTP periodically.
  // We store last known good NTP unix and then advance by millis.
  // This is handled inside Metrics as a monotonic clock.
  return MetricsClock::now();
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {;}

  Wire.begin();

  if (!connectWiFi()) {
    while (true) { delay(1000); }
  }

  // Initialize RTC early; if it wasn’t set, we’ll NTP sync it.
  RTC.begin();

  // Init sensor
  if (!initBME680()) {
    while (true) { delay(1000); }
  }

  // Establish a baseline clock
  if (syncRTCFromNTP()) {
    MetricsClock::setUnix(ntp.getEpochTime());
  } else {
    MetricsClock::setUnix(ntp.getEpochTime()); // still gives “some” time if NTP worked partially
  }

  // Start server
  server.begin();
  Serial.println("Web server started");
  Serial.print("Open browser to: http://");
  Serial.println(WiFi.localIP());

  lastEnvMs = millis();
  lastGasMs = millis();
}

static void sampleEnv() {
  if (!bme.performReading()) {
    Serial.println("BME680 reading failed.");
    return;
  }

  // Allow WiFi stack to process events during blocking operation
  yield();

  const float tempC = bme.temperature;
  const float humPct = bme.humidity;
  const float pressHpa = bme.pressure / 100.0f;

  const float seaLevelHpa = calcSeaLevelPressureHpa(pressHpa, ALTITUDE_M);

  // Timestamp (monotonic clock maintained from NTP sync)
  uint32_t t = MetricsClock::tickToNowUnix();

  envHist.push(t, tempC, humPct, pressHpa, seaLevelHpa);
}

static void sampleGas() {
  // bme.performReading() already updates gas too, but we only *record* gas at 3s cadence.
  // We can reuse last reading by calling performReading() in env sample, but simplest is just read again.
  if (!bme.performReading()) {
    Serial.println("BME680 reading failed (gas).");
    return;
  }

  // Allow WiFi stack to process events during blocking operation
  yield();

  float gasOhms = bme.gas_resistance; // ohms
  uint32_t t = MetricsClock::tickToNowUnix();
  gasHist.push(t, gasOhms);
}

void loop() {
  // Periodic NTP resync to correct RTC drift
  if (WiFi.status() == WL_CONNECTED && (millis() - lastNtpSyncMs) > NTP_RESYNC_MS) {
    if (syncRTCFromNTP()) {
      MetricsClock::setUnix(ntp.getEpochTime());
    }
  }

  // Handle HTTP clients (non-blocking state machine - processes all clients)
  handleHttpClients(envHist, gasHist, ALTITUDE_M, server);

  // Sampling
  uint32_t nowMs = millis();

  if (nowMs - lastEnvMs >= SAMPLE_ENV_MS) {
    lastEnvMs += SAMPLE_ENV_MS;
    sampleEnv();
  }

  if (nowMs - lastGasMs >= SAMPLE_GAS_MS) {
    lastGasMs += SAMPLE_GAS_MS;
    sampleGas();
  }

  // Yield to let WiFi stack process events
  yield();
}
