// ============ WiFi Connection Manager with Exponential Backoff ============
// Handles periodic WiFi status checking and automatic reconnection
// with exponential backoff to avoid hammering the AP

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFiS3.h>

// WiFi manager state
struct WiFiManager {
  bool is_connected;
  uint32_t last_check_ms;
  uint32_t last_attempt_ms;
  uint32_t backoff_delay_ms;

  const uint32_t CHECK_INTERVAL_MS = 5000;    // Check WiFi status every 5 seconds
  const uint32_t MIN_BACKOFF_MS = 1000;       // Start with 1 second
  const uint32_t MAX_BACKOFF_MS = 600000;     // Cap at 10 minutes

  WiFiManager() : is_connected(false), last_check_ms(0), last_attempt_ms(0),
                  backoff_delay_ms(MIN_BACKOFF_MS) {}
};

static WiFiManager wifi_mgr;

// Initialize WiFi manager (call once during setup, before attempting connection)
static void initWiFiManager() {
  wifi_mgr.is_connected = false;
  wifi_mgr.last_check_ms = 0;
  wifi_mgr.last_attempt_ms = 0;
  wifi_mgr.backoff_delay_ms = wifi_mgr.MIN_BACKOFF_MS;

  Serial.println("[WiFiMgr] Initialized - ready for non-blocking WiFi operations");
}

// Attempt non-blocking WiFi connection (call periodically from loop)
// Returns true if currently connected, false if disconnected
static bool updateWiFiStatus(const char* ssid, const char* pass, const char* device_name = nullptr) {
  uint32_t now = millis();

  // Don't check more frequently than CHECK_INTERVAL
  if (now - wifi_mgr.last_check_ms < wifi_mgr.CHECK_INTERVAL_MS) {
    return wifi_mgr.is_connected;
  }

  wifi_mgr.last_check_ms = now;

  // Check current connection status
  bool currently_connected = (WiFi.status() == WL_CONNECTED);

  // Status changed from disconnected to connected
  if (!wifi_mgr.is_connected && currently_connected) {
    wifi_mgr.is_connected = true;
    wifi_mgr.backoff_delay_ms = wifi_mgr.MIN_BACKOFF_MS;  // Reset backoff

    // Set hostname if provided
    if (device_name) {
      WiFi.setHostname(device_name);
      Serial.print("[WiFiMgr] Hostname set to: ");
      Serial.println(device_name);
    }

    Serial.print("[WiFiMgr] Connected! IP: ");
    Serial.println(WiFi.localIP());
    return true;
  }

  // Status changed from connected to disconnected
  if (wifi_mgr.is_connected && !currently_connected) {
    wifi_mgr.is_connected = false;
    wifi_mgr.last_attempt_ms = 0;  // Force immediate reconnect attempt
    Serial.println("[WiFiMgr] Disconnected - will attempt reconnect");
    return false;
  }

  // If already connected, nothing to do
  if (currently_connected) {
    return true;
  }

  // We're disconnected and need to reconnect
  // Check if enough time has passed for the current backoff interval
  if (now - wifi_mgr.last_attempt_ms >= wifi_mgr.backoff_delay_ms) {
    wifi_mgr.last_attempt_ms = now;

    Serial.print("[WiFiMgr] Attempting reconnect (backoff: ");
    Serial.print(wifi_mgr.backoff_delay_ms);
    Serial.println("ms)...");

    // Attempt connection (non-blocking - will return immediately)
    WiFi.begin(ssid, pass);

    // Increase backoff for next attempt (exponential backoff)
    wifi_mgr.backoff_delay_ms = wifi_mgr.backoff_delay_ms * 2;
    if (wifi_mgr.backoff_delay_ms > wifi_mgr.MAX_BACKOFF_MS) {
      wifi_mgr.backoff_delay_ms = wifi_mgr.MAX_BACKOFF_MS;
    }
  }

  return false;
}

// Get connection status
static bool isWiFiConnected() {
  return wifi_mgr.is_connected;
}

// Get signal strength (RSSI) if connected
static long getWiFiSignalStrength() {
  if (wifi_mgr.is_connected) {
    return WiFi.RSSI();
  }
  return 0;
}

#endif // WIFI_MANAGER_H
