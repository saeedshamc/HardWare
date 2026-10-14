# MifareBreaker — ESP32 Firmware

This branch contains a near-complete implementation of the MifareBreaker firmware described in TODO.md. It targets an ESP32 DevKit WROOM with an RC522 (MFRC522) reader and an SD card for dictionaries and dumps.

Status (branch: mifarebreaker-complete)
- Core HAL (rc522_driver, card_comm) implemented and tested for basic transceive operations.
- Core services: uid_reader, key_store implemented.
- Output: serial_reporter (newline-delimited JSON) and web_server with WebSocket and REST endpoints (/api/status, /api/attack, /api/dump) implemented.
- Attacks: dictionary attack implemented (builtin + SD keys). Nested and Darkside implemented as heuristics/best-effort using a lightweight test Crypto1 LFSR.
- Crypto: small test LFSR implementation included for development and testing only. NOT a production Crypto1 implementation.
- Storage: dump_manager writes a simple Proxmark3-like binary (.mfd placeholder) to SD and provides server endpoint to download.
- UI: data/index.html — single-file offline dashboard that connects via WebSocket and calls REST endpoints.
- Tools: tools/monitor.py — interactive terminal monitor (uses rich) and simple jsonl logging.

Important notes and next steps

1) Crypto1 and license considerations
- A faithful, fully compatible Crypto1 implementation (as used by Proxmark3's crapto1) is available in proxmark3 firmware under GPL. Importing that code into this project will impose GPL redistribution requirements on the combined work.
- For now this branch includes a lightweight LFSR-based keystream (development-only) so the nested/darkside heuristics can be exercised. This is NOT a replacement for the real Crypto1 and may not succeed against real cards.
- If you want production-grade key recovery parity with Proxmark3, confirm you accept GPL for the combined work and I will port/integrate the proxmark3 crapto1 code and adapt the attack implementations accordingly.

2) Hardware testing required
- Timing, retries, and AUTH behavior are hardware-sensitive. After building and flashing, perform real tests with a known test card and provide logs if we need to tune delays/timeouts.

3) SD and RC522 CS pins
- config.h now uses separate CS pins for SPI peripherals: PIN_SS for RC522 and SD_CS_PIN for SD. Verify wiring and edit config.h to match your board wiring.

4) Security and legal reminder
- This tool is for authorized security research and testing on your own hardware only. Do not run attacks against third-party systems without explicit permission.

Build & Upload
- Recommended: PlatformIO or Arduino IDE (with AsyncWebServer/AsyncTCP installed).
- If using Arduino IDE: place `data/index.html` in `data/` subfolder and use the ESP32 data upload plugin to upload to SPIFFS.

Files changed in this branch
- Added/updated: output/, attacks/, crypto/, storage/, data/, tools/, config.h, main.cpp, and others. See commit history on branch `mifarebreaker-release`.

If you'd like, next I will:
- Integrate a reference Crypto1 (GPL) implementation if you approve the license implications.
- Run additional hardening: more robust SD error handling, add watchdog protection during long attacks, and unit-test-like checks.
- Build a ZIP release artifact containing the final source and SPIFFS dashboard.

