# SD Card Static Assets

These files must be copied to the root of your Arduino's SD card:

- `app.css` - Stylesheet for web dashboard
- `app.js` - JavaScript for interactive features and graphs
- `favicon.svg` - Website icon

## Instructions

1. Copy these three files to the root directory of your SD card (same level where CSV data files are stored)
2. Make sure your SD card is inserted into the Arduino
3. Upload the modified sketch to your Arduino
4. The sketch will now serve these files from the SD card instead of embedding them in flash memory

## Why This Changed

The original sketch had CSS and JavaScript embedded as large string constants in the code, which consumed ~60KB of the Arduino's limited 256KB flash memory. By moving these files to the SD card, we:

- Free up ~60KB of flash for actual code
- Reduce the binary size from overflowing to fitting comfortably
- Make it easy to update CSS/JS without recompiling the sketch

## Verification

After uploading, test that the dashboard loads correctly:
- Visit http://your-arduino-ip/
- Check that styles and interactive features work
- Verify graph hover tooltips and calendar work
