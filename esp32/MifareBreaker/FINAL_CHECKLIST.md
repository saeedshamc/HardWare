# Final checklist for testing MifareBreaker on ESP32

Follow these steps to test the firmware on an ESP32 DevKit WROOM with RC522 and SD:

1. Wiring
   - RC522: SDA(PIN_SS)=GPIO5, SCK=GPIO18, MOSI=GPIO23, MISO=GPIO19, RST=GPIO22
   - SD: CS=GPIO4 (SD_CS_PIN), MOSI/MISO/SCK shared with VSPI
   - 3.3V and GND common

2. Configure
   - Edit esp32/MifareBreaker/config.h: set WIFI_SSID, WIFI_PASS and pin defines to match your wiring.

3. Build & Flash
   - Using PlatformIO (recommended):
     - Open project folder esp32/MifareBreaker
     - pio run --target upload
   - Or Arduino IDE: open folder as sketch, select ESP32 Dev Module, and upload.

4. Upload SPIFFS
   - Ensure data/index.html is present in data/
   - PlatformIO: pio run --target uploadfs
   - Arduino IDE: use "ESP32 Sketch Data Upload" plugin

5. Prepare SD
   - Place keys on SD root as `/keys.txt` (copy from `data/keys_sample.txt`)

6. Run & Monitor
   - Open serial monitor at 115200
   - Expect initial JSON events; bring a MIFARE Classic card near the reader
   - Use web dashboard at http://<esp32-ip>/ and monitor ws://<esp32-ip>/ws

7. Collect Logs
   - Use tools/monitor.py to capture JSONL logs and view progress locally

8. Debugging
   - If RC522 fails to initialize: check wiring, 3.3V, swap CS pins
   - If SD not found: ensure SD_CS_PIN is correct and SD formatted FAT32

9. Next steps after smoke-test
   - Share serial logs (jsonl) if timing/auth errors occur; I will tune timeouts and retries.

