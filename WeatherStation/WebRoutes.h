#pragma once
#include <Arduino.h>
#include <WiFiS3.h>

#include "Metrics.h"
#include "SolarMoon.h"

#include "Pages_Dashboard.h"
#include "Pages_GraphTemp.h"
#include "Pages_GraphHumidity.h"
#include "Pages_GraphPressure.h"
#include "Pages_GraphGas.h"
#include "Pages_Comfort.h"
#include "Favicon.h"

// Non-blocking HTTP client state machine
struct HttpClientState {
  WiFiClient client;
  char reqBuffer[512];     // Request buffer
  uint16_t bufPos;
  uint32_t lastActivityMs;
  bool headersReceived;
  String url;

  HttpClientState() : bufPos(0), lastActivityMs(0), headersReceived(false) {}

  bool isConnected() {
    return client.connected();
  }

  bool isTimedOut() {
    return (millis() - lastActivityMs) > 3000; // 3 second timeout
  }
};

// Client connection pool (max 2 concurrent clients)
static HttpClientState clientStates[2];
static uint8_t activeClientCount = 0;

static void send404(WiFiClient& client) {
  client.println("HTTP/1.1 404 Not Found");
  client.println("Content-Type: text/plain; charset=utf-8");
  client.println("Connection: close");
  client.println();
  client.println("404 Not Found");
}

static void sendJson(WiFiClient& client, const String& json) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json; charset=utf-8");
  client.println("Cache-Control: no-store");
  client.println("Connection: close");
  client.println();
  client.print(json);
}

static void sendHtml(WiFiClient& client, const String& html) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html; charset=utf-8");
  client.println("Cache-Control: no-store");
  client.println("Connection: close");
  client.println();
  client.print(html);
}

static String urlParam(const String& url, const char* key) {
  int q = url.indexOf('?');
  if (q < 0) return "";
  String query = url.substring(q + 1);
  String k = String(key) + "=";
  int p = query.indexOf(k);
  if (p < 0) return "";
  int s = p + k.length();
  int e = query.indexOf('&', s);
  if (e < 0) e = query.length();
  return query.substring(s, e);
}

static String jsonLatest(const EnvHistory& env, const GasHistory& gas, float altitudeM) {
  EnvSample e = env.latest();
  GasSample g = gas.latest();

  float tAvg = env.avgTempLast(60);
  float hAvg = env.avgHumLast(60);
  float pAvg = env.avgSeaLast(60);
  float gAvg = gas.avgGasLast(90);

  float dp = isnan(e.tempC) ? NAN : dewPointC(e.tempC, e.humPct);
  float hi = isnan(e.tempC) ? NAN : heatIndexC(e.tempC, e.humPct);

  float perHr = 0;
  PressureTrend tr = env.pressureTendency(10 * 60, &perHr); // 10 minutes

  float trendPts = env.trendPoints(15 * 60); // always numeric

  // Storm heuristic (very rough):
  // - low sea-level pressure and falling fast => higher risk
  // - very high pressure and rising => low risk
  int storm = 0; // 0..100
  if (!isnan(e.seaLevelHpa)) {
    float p = e.seaLevelHpa;
    if (p < 1000) storm += 35;
    if (p < 990)  storm += 20;
    if (perHr < -1.0f) storm += 30;
    if (perHr < -2.0f) storm += 20;
    if (p > 1020 && perHr > 0.5f) storm -= 20;
    if (storm < 0) storm = 0;
    if (storm > 100) storm = 100;
  }

  auto f2 = [](float v) -> String {
    if (isnan(v)) return "null";
    char b[24];
    snprintf(b, sizeof(b), "%.2f", v);
    return String(b);
  };

  String j = "{";
  j += "\"t\":" + String((unsigned long)e.t) + ",";
  j += "\"tempC\":" + f2(e.tempC) + ",";
  j += "\"hum\":" + f2(e.humPct) + ",";
  j += "\"pressHpa\":" + f2(e.pressHpa) + ",";
  j += "\"seaHpa\":" + f2(e.seaLevelHpa) + ",";
  j += "\"gasOhms\":" + f2(g.gasOhms) + ",";
  j += "\"avgTemp60\":" + f2(tAvg) + ",";
  j += "\"avgHum60\":" + f2(hAvg) + ",";
  j += "\"avgSea60\":" + f2(pAvg) + ",";
  j += "\"avgGas90\":" + f2(gAvg) + ",";
  j += "\"dewPointC\":" + f2(dp) + ",";
  j += "\"heatIndexC\":" + f2(hi) + ",";
  j += "\"trend\":\"" + String(pressureTrendLabel(tr)) + "\",";
  j += "\"deltaHpaPerHr\":" + f2(perHr) + ",";
  j += "\"trendPoints\":" + f2(trendPts) + ",";
  j += "\"stormRisk\":" + String(storm) + ",";
  j += "\"altitudeM\":" + f2(altitudeM);
  j += "}";
  return j;
}

static String jsonHistoryEnv(const EnvHistory& env, const char* metric) {
  // returns: [{t:..., v:...}, ...]
  String j = "[";
  for (uint16_t i = 0; i < env.count(); i++) {
    EnvSample s = env.getFromOldest(i);
    float v = NAN;
    if (strcmp(metric, "temp") == 0) v = s.tempC;
    else if (strcmp(metric, "humidity") == 0) v = s.humPct;
    else if (strcmp(metric, "pressure") == 0) v = s.seaLevelHpa;
    else v = NAN;

    if (i) j += ",";
    j += "{\"t\":" + String((unsigned long)s.t) + ",\"v\":";
    if (isnan(v)) j += "null";
    else {
      char b[24];
      snprintf(b, sizeof(b), "%.3f", v);
      j += b;
    }
    j += "}";
  }
  j += "]";
  return j;
}

static String jsonHistoryGas(const GasHistory& gas) {
  String j = "[";
  for (uint16_t i = 0; i < gas.count(); i++) {
    GasSample s = gas.getFromOldest(i);
    if (i) j += ",";
    j += "{\"t\":" + String((unsigned long)s.t) + ",\"v\":";
    if (isnan(s.gasOhms)) j += "null";
    else {
      char b[24];
      snprintf(b, sizeof(b), "%.1f", s.gasOhms);
      j += b;
    }
    j += "}";
  }
  j += "]";
  return j;
}

static String buildComfortJson(const EnvHistory& env, const GasHistory& gas) {
  // Location you gave
  const double LAT = -37.1096;
  const double LON = 142.8484;

  // Melbourne: DST is likely on in late Dec. Use +11 hours.
  // If you want automatic DST, we can add a ruleset later.
  const int tzOffsetMin = 11 * 60;

  // Use env latest time as “now”
  EnvSample e = env.latest();
  uint32_t unixNow = e.t;
  if (unixNow == 0) unixNow = MetricsClock::now();

  // Convert unix->approx Y-M-D locally
  // Minimal conversion: we only need the date for solar calcs.
  // This is a compact civil-from-days conversion.
  uint32_t local = unixNow + (uint32_t)(tzOffsetMin * 60);
  int64_t z = (int64_t)(local / 86400UL) + 719468; // days since 0000-03-01
  int64_t era = (z >= 0 ? z : z - 146096) / 146097;
  uint32_t doe = (uint32_t)(z - era * 146097);
  uint32_t yoe = (doe - doe/1460 + doe/36524 - doe/146096) / 365;
  int y = (int)(yoe) + (int)(era) * 400;
  uint32_t doy = doe - (365*yoe + yoe/4 - yoe/100);
  uint32_t mp = (5*doy + 2)/153;
  uint32_t d = doy - (153*mp + 2)/5 + 1;
  uint32_t m = mp + (mp < 10 ? 3 : -9);
  y += (m <= 2);

  SunTimes sun = computeSunTimes(y, (int)m, (int)d, LAT, LON, tzOffsetMin);

  double ph = moonPhase01(unixNow);
  const char* phName = moonPhaseName(ph);

  auto f2 = [](double v) -> String {
    char b[32];
    snprintf(b, sizeof(b), "%.2f", v);
    return String(b);
  };

  char sunrise[8], sunset[8], noon[8], daylen[16];
  if (sun.valid) {
    formatHHMM(sunrise, sizeof(sunrise), sun.sunriseMin);
    formatHHMM(sunset, sizeof(sunset), sun.sunsetMin);
    formatHHMM(noon, sizeof(noon), sun.solarNoonMin);
    formatDuration(daylen, sizeof(daylen), sun.dayLengthMin);
  } else {
    strcpy(sunrise, "--:--");
    strcpy(sunset, "--:--");
    strcpy(noon, "--:--");
    strcpy(daylen, "--");
  }

  String j = "{";
  j += "\"lat\":" + f2(LAT) + ",";
  j += "\"lon\":" + f2(LON) + ",";
  j += "\"tzOffsetMin\":" + String(tzOffsetMin) + ",";
  j += "\"date\":\"" + String(y) + "-" + (m<10?"0":"") + String((int)m) + "-" + (d<10?"0":"") + String((int)d) + "\",";
  j += "\"sunrise\":\"" + String(sunrise) + "\",";
  j += "\"sunset\":\"" + String(sunset) + "\",";
  j += "\"solarNoon\":\"" + String(noon) + "\",";
  j += "\"dayLength\":\"" + String(daylen) + "\",";
  j += "\"moonPhase01\":" + f2(ph) + ",";
  j += "\"moonName\":\"" + String(phName) + "\"";
  j += "}";
  return j;
}

// Non-blocking HTTP request processing (called repeatedly from main loop)
static void processHttpClientState(HttpClientState& state, const EnvHistory& envHist, const GasHistory& gasHist, float altitudeM) {
  if (!state.isConnected()) {
    return;  // Not active
  }

  if (state.isTimedOut()) {
    Serial.println("Client timeout");
    state.client.stop();
    return;
  }

  // Read available data without blocking
  while (state.client.available() && state.bufPos < sizeof(state.reqBuffer) - 1) {
    char c = state.client.read();
    state.reqBuffer[state.bufPos++] = c;
    state.lastActivityMs = millis();

    // Check for end of headers (\r\n\r\n)
    if (state.bufPos >= 4 &&
        state.reqBuffer[state.bufPos - 4] == '\r' &&
        state.reqBuffer[state.bufPos - 3] == '\n' &&
        state.reqBuffer[state.bufPos - 2] == '\r' &&
        state.reqBuffer[state.bufPos - 1] == '\n') {
      state.headersReceived = true;
      break;
    }
  }

  // If headers received, parse and respond
  if (state.headersReceived) {
    // Parse request line
    String reqLine = "";
    for (uint16_t i = 0; i < state.bufPos && state.reqBuffer[i] != '\n'; i++) {
      if (state.reqBuffer[i] != '\r') {
        reqLine += state.reqBuffer[i];
      }
    }

    Serial.print("Request: ");
    Serial.println(reqLine);

    // Parse: "GET /path HTTP/1.1"
    if (reqLine.startsWith("GET ")) {
      int sp1 = reqLine.indexOf(' ');
      int sp2 = reqLine.indexOf(' ', sp1 + 1);
      String url = reqLine.substring(sp1 + 1, sp2);

      // Route request (all with yield() to prevent blocking)
      if (url == "/favicon.svg") {
        sendFaviconSvg(state.client);
        yield();
      }
      else if (url.startsWith("/api/latest")) {
        sendJson(state.client, jsonLatest(envHist, gasHist, altitudeM));
        yield();
      }
      else if (url.startsWith("/api/history")) {
        String m = urlParam(url, "m");
        if (m == "temp") sendJson(state.client, jsonHistoryEnv(envHist, "temp"));
        else if (m == "humidity") sendJson(state.client, jsonHistoryEnv(envHist, "humidity"));
        else if (m == "pressure") sendJson(state.client, jsonHistoryEnv(envHist, "pressure"));
        else if (m == "gas") sendJson(state.client, jsonHistoryGas(gasHist));
        else sendJson(state.client, "[]");
        yield();
      }
      else if (url.startsWith("/api/comfort")) {
        sendJson(state.client, buildComfortJson(envHist, gasHist));
        yield();
      }
      else if (url == "/" || url.startsWith("/?")) {
        sendHtml(state.client, pageDashboard());
        yield();
      }
      else if (url.startsWith("/graph/temp")) {
        sendHtml(state.client, pageGraphTemp());
        yield();
      }
      else if (url.startsWith("/graph/humidity")) {
        sendHtml(state.client, pageGraphHumidity());
        yield();
      }
      else if (url.startsWith("/graph/pressure")) {
        sendHtml(state.client, pageGraphPressure());
        yield();
      }
      else if (url.startsWith("/graph/gas")) {
        sendHtml(state.client, pageGraphGas());
        yield();
      }
      else if (url.startsWith("/comfort")) {
        sendHtml(state.client, pageComfort());
        yield();
      }
      else {
        send404(state.client);
        yield();
      }
    } else {
      send404(state.client);
      yield();
    }

    // Ensure all response data is sent, but don't close the connection.
    // The browser will close it naturally after receiving "Connection: close" header.
    // This allows large responses to fully transmit before connection closes.
    state.client.flush();

    // Reset buffer for potential keep-alive requests
    state.bufPos = 0;
    state.headersReceived = false;
    // Timeout (3 seconds) will force-close stale connections
  }
}

// Non-blocking HTTP handler - called from main loop to accept new connections
inline void handleHttpClients(const EnvHistory& envHist, const GasHistory& gasHist, float altitudeM, WiFiServer& server) {
  // Clean up old/disconnected clients
  for (uint8_t i = 0; i < 2; i++) {
    if (!clientStates[i].isConnected() && clientStates[i].bufPos > 0) {
      clientStates[i].bufPos = 0;
      clientStates[i].headersReceived = false;
    }
  }

  // Accept new connections
  WiFiClient newClient = server.available();
  if (newClient) {
    // Find empty slot
    for (uint8_t i = 0; i < 2; i++) {
      if (!clientStates[i].isConnected()) {
        Serial.println("=== Client connected ===");
        Serial.print("Client IP: ");
        Serial.println(newClient.remoteIP());

        clientStates[i].client = newClient;
        clientStates[i].bufPos = 0;
        clientStates[i].headersReceived = false;
        clientStates[i].lastActivityMs = millis();
        break;
      }
    }
  }

  // Process all active client states (non-blocking)
  for (uint8_t i = 0; i < 2; i++) {
    if (clientStates[i].isConnected()) {
      processHttpClientState(clientStates[i], envHist, gasHist, altitudeM);
    }
  }
}
