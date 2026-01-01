#ifndef SD_LOGGING_H
#define SD_LOGGING_H

#include <SdFat.h>

// SD card configuration using SdFat with DEDICATED_SPI
// CS pin 7 (WiFi uses pin 10, so no conflict)
#define SD_CS_PIN 7
#define SPI_CLOCK 25  // 25 MHz - proven stable on UNO R4
#define SD_CONFIG SdSpiConfig(SD_CS_PIN, DEDICATED_SPI, SPI_CLOCK)

// SD card object
SdFat sd;
File logFile;

// SD card status
struct SDInfo {
  bool initialized = false;
  uint64_t total_bytes = 0;  // uint64_t to handle cards > 4GB
  uint64_t free_bytes = 0;   // uint64_t to handle cards > 4GB
  uint32_t logged_samples = 0;
  unsigned long file_size = 0;
  char error_msg[64] = "";
};

SDInfo sd_info;

// ============ SD INITIALIZATION ============
bool initSDCard() {
  Serial.println("Initializing SD card on pin 7 with SdFat...");

  if (!sd.begin(SD_CONFIG)) {
    strcpy(sd_info.error_msg, "SD init failed");
    Serial.println("SD card initialization failed!");
    sd_info.initialized = false;
    return false;
  }

  sd_info.initialized = true;

  // Get card info (use uint64_t to handle cards larger than 4GB)
  uint64_t cardSize = sd.card()->cardSize();
  sd_info.total_bytes = cardSize * 512;

  // Calculate free space
  uint32_t freeClusters = sd.vol()->freeClusterCount();
  uint32_t clusterSize = sd.vol()->sectorsPerCluster() * 512;
  sd_info.free_bytes = (uint64_t)freeClusters * clusterSize;

  Serial.print("SD card initialized. Total: ");
  Serial.print(sd_info.total_bytes / (1024*1024));
  Serial.print(" MB, Free: ");
  Serial.print(sd_info.free_bytes / (1024*1024));
  Serial.println(" MB");

  return true;
}

// Forward declaration for loadHistoryFromSD
// Implementation is in main sketch after RingF struct is defined
struct RingF;
bool loadHistoryFromSD(RingF &tempSeries, RingF &humSeries, RingF &pressSeries,
                       RingF &gasSeries, RingF &slpTrend);

// ============ LOG READING TO CSV ============
// Forward declaration for helper function (will be defined in main sketch)
// Note: Implementation must be non-static so this forward declaration works
void getCurrentDateString(char* dateStr, int maxLen);

// All possible sensor data for complete historical logging
// Logs to per-day files: YYYY-MM-DD.csv
bool logSensorReading(unsigned long timestamp_ms,
                      float temp_c, float hum_pct,
                      float press_station_hpa, float press_sealevel_hpa,
                      float dew_point_c, float heat_index_c,
                      float pressure_tendency_hpa_per_3h,
                      float storm_score, float gas_kohm) {
  if (!sd_info.initialized) {
    return false;
  }

  // Get current date string for filename (YYYY-MM-DD)
  char date_str[16];
  getCurrentDateString(date_str, sizeof(date_str));

  // Construct filename: YYYY-MM-DD.csv
  char filename[32];
  snprintf(filename, sizeof(filename), "%s.csv", date_str);

  // Open file in append mode
  if (!logFile.open(filename, O_WRONLY | O_CREAT | O_APPEND)) {
    strcpy(sd_info.error_msg, "Cannot open daily CSV");
    return false;
  }

  // Format: timestamp_ms,temp_c,humidity_pct,pressure_station_hpa,pressure_sealevel_hpa,
  //         dew_point_c,heat_index_c,pressure_tendency_hpa_per_3h,storm_score,gas_kohm
  char buffer[256];
  snprintf(buffer, sizeof(buffer),
           "%lu,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f\n",
           timestamp_ms, temp_c, hum_pct,
           press_station_hpa, press_sealevel_hpa,
           dew_point_c, heat_index_c,
           pressure_tendency_hpa_per_3h, storm_score, gas_kohm);

  logFile.print(buffer);
  logFile.sync();   // Flush to SD card immediately
  logFile.close();  // Close file

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
