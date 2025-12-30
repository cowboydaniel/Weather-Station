#ifndef SD_LOGGING_H
#define SD_LOGGING_H

#include <SdFat.h>
#include <SPI.h>

// SD card CS pin (different from WiFi's pin 10, shares hardware SPI bus)
// Hardware SPI pins (shared): 11 (MOSI), 12 (MISO), 13 (SCK)
// CS pins (separate): WiFi uses 10, SD card uses 7
const uint8_t SD_CS_PIN = 7;

// SD card object using shared hardware SPI
SdFat32 sd;
File32 logFile;

// SD card status
struct SDInfo {
  bool initialized = false;
  uint32_t total_bytes = 0;
  uint32_t free_bytes = 0;
  uint32_t logged_samples = 0;
  unsigned long file_size = 0;
  char error_msg[64] = "";
};

SDInfo sd_info;

// ============ SD INITIALIZATION ============
bool initSDCard() {
  Serial.println("Initializing SD card on shared SPI bus...");

  // Use hardware SPI with pin 7 as CS (WiFi uses pin 10)
  // Both devices share the SPI bus but with separate CS pins
  SdSpiConfig spiConfig(SD_CS_PIN, SHARED_SPI, SD_SCK_MHZ(10));

  if (!sd.begin(spiConfig)) {
    strcpy(sd_info.error_msg, "SD init failed");
    Serial.println("SD card initialization failed!");
    sd_info.initialized = false;
    return false;
  }

  // Get card info
  sd_info.total_bytes = (uint32_t)sd.card()->cardSize() * 512;

  // Calculate free space
  uint32_t freeClusters = sd.vol()->freeClusterCount();
  uint32_t clusterSize = sd.vol()->sectorsPerCluster() * 512;
  sd_info.free_bytes = freeClusters * clusterSize;
  sd_info.initialized = true;

  Serial.print("SD card initialized. Total: ");
  Serial.print(sd_info.total_bytes / (1024*1024));
  Serial.print(" MB, Free: ");
  Serial.print(sd_info.free_bytes / (1024*1024));
  Serial.println(" MB");

  return true;
}

// Forward declaration - RingF is defined in main sketch
struct RingF;

// ============ LOAD HISTORY FROM CSV ============
bool loadHistoryFromSD(RingF &tempSeries, RingF &humSeries, RingF &pressSeries,
                       RingF &gasSeries, RingF &slpTrend) {
  if (!sd_info.initialized) {
    Serial.println("SD card not initialized");
    return false;
  }

  if (!sd.exists("data.csv")) {
    Serial.println("No existing data.csv found, starting fresh");
    sd_info.logged_samples = 0;
    return true;
  }

  // Open existing file for reading
  if (!logFile.open("data.csv", O_RDONLY)) {
    strcpy(sd_info.error_msg, "Cannot open data.csv");
    Serial.println("Failed to open data.csv");
    return false;
  }

  Serial.println("Loading historical data from CSV...");

  uint32_t sample_count = 0;
  char line[128];
  int line_len = 0;

  // Read and parse each line
  while (logFile.available()) {
    int c = logFile.read();

    if (c == '\n' || c == -1) {
      if (line_len > 0) {
        line[line_len] = '\0';

        // Parse CSV: timestamp_ms,temp_c,humidity_pct,pressure_hpa,gas_kohm
        unsigned long ts = 0;
        float temp = NAN, hum = NAN, press = NAN, gas = NAN;

        int parsed = sscanf(line, "%lu,%f,%f,%f,%f", &ts, &temp, &hum, &press, &gas);

        if (parsed == 5 && isfinite(temp) && isfinite(hum) && isfinite(press)) {
          // Push to ring buffers (they auto-wrap when full)
          tempSeries.push(temp);
          humSeries.push(hum);
          pressSeries.push(press);

          if (isfinite(gas)) {
            gasSeries.push(gas);
          }

          sample_count++;
        }
      }

      line_len = 0;
      if (c == -1) break;
    } else if (c >= 32 && c < 127) {  // Printable ASCII
      if (line_len < sizeof(line) - 1) {
        line[line_len++] = c;
      }
    }
  }

  logFile.close();

  sd_info.logged_samples = sample_count;
  sd_info.file_size = logFile.fileSize();

  Serial.print("Loaded ");
  Serial.print(sample_count);
  Serial.println(" samples from SD card");

  return true;
}

// ============ LOG READING TO CSV ============
bool logSensorReading(unsigned long timestamp_ms, float temp_c, float hum_pct,
                      float press_hpa, float gas_kohm) {
  if (!sd_info.initialized) {
    return false;
  }

  // Open file in append mode
  if (!logFile.open("data.csv", O_WRONLY | O_APPEND | O_CREAT)) {
    strcpy(sd_info.error_msg, "Cannot open data.csv");
    return false;
  }

  // Format: timestamp_ms,temp_c,humidity_pct,pressure_hpa,gas_kohm
  char buffer[128];
  snprintf(buffer, sizeof(buffer), "%lu,%.2f,%.2f,%.2f,%.2f\n",
           timestamp_ms, temp_c, hum_pct, press_hpa, gas_kohm);

  logFile.print(buffer);
  logFile.close();

  sd_info.logged_samples++;

  return true;
}

// ============ GET SAMPLE COUNT ============
uint32_t getSDSampleCount() {
  return sd_info.logged_samples;
}

// ============ GET FREE SPACE (MB) ============
uint32_t getSDFreeSpaceMB() {
  return sd_info.free_bytes / (1024 * 1024);
}

// ============ GET TOTAL SPACE (MB) ============
uint32_t getSDTotalSpaceMB() {
  return sd_info.total_bytes / (1024 * 1024);
}

#endif
