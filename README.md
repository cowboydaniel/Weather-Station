# UNO R4 WiFi Weather Station

This sketch turns an Arduino UNO R4 WiFi into a self-hosted weather station with a Bosch BME680 environmental sensor. It serves a browser dashboard and several detail pages, plus JSON APIs for integrating the live readings in other tools.

## Hardware and library requirements

- Arduino UNO R4 WiFi (uses the `WiFiS3` driver).
- Bosch BME680 sensor wired to the I2C bus (the sketch probes `0x76` first and falls back to `0x77`).
- Arduino libraries: **WiFiS3**, **Adafruit BME680**, and **Adafruit Unified Sensor**.

## Configuration

1. Open `WeatherStation/WeatherStation.ino` in the Arduino IDE (or CLI).
2. Update your Wi-Fi network name, password, and local altitude in the **USER CONFIG** section before uploading.
3. Select the UNO R4 WiFi board definition and the correct serial/USB port.
4. Upload the sketch.

## Running the station

After boot, the sketch connects to Wi‑Fi, initializes the BME680, and starts an HTTP server on port 80. The assigned IP address is printed to the serial monitor for quick access.

## Web dashboard

Visit `http://<device-ip>/` to view the landing dashboard. Additional pages provide focused charts and tables:

- `/temp`, `/humidity`, `/pressure`, `/gas`: detail pages for each raw sensor channel.
- `/comfort`: humidity/temperature comfort view.
- `/derived`: dew point, heat index, sea-level pressure, and pressure tendency.
- `/settings`: sampling/config overview.
- `/stats`: system statistics and request counters.

## JSON API endpoints

Each page has a corresponding JSON feed for scripting or external dashboards:

- `/api` – summary snapshot of the latest readings.
- `/api/temp`, `/api/humidity`, `/api/pressure`, `/api/gas` – raw series and latest values.
- `/api/comfort`, `/api/derived` – derived/comfort metrics.
- `/api/dashboard` – combined panel data for the landing page.
- `/api/stats` – request counts, loop rate, and buffer sizes.

Example curl:

```bash
curl http://<device-ip>/api
```

## Notes

- If the sensor is not found at `0x76`, the sketch automatically retries `0x77` and halts with an error message if both fail.
- Sampling intervals (1 Hz for temp/humidity/pressure; ~0.33 Hz for gas resistance) and ring buffer sizes are configurable near the top of the sketch if you need longer history or faster updates.
