Project Name: MifareBreaker

I want to build a security research tool for auditing MIFARE Classic RFID/NFC cards using an ESP32 microcontroller and an RC522 reader module. This is for authorized security testing of my own access-control hardware (e.g., auditing an attendance system I built). Implement it in C++ using the Arduino/ESP-IDF framework for ESP32.

## Project Structure
Create a modular firmware project with this folder layout:

- /hal
  - rc522_driver.cpp/h — Low-level SPI communication with RC522, raw ISO14443-A frame send/receive
  - card_comm.cpp/h — Anticollision, SELECT, and request/wakeup sequences per ISO14443-3

- /core
  - uid_reader.cpp/h — UID detection and card type identification (MIFARE Classic 1K vs 4K)
  - sector_manager.cpp/h — Sector/block addressing helpers
  - key_store.cpp/h — In-memory CardDump struct holding recovered keys and sector data

- /crypto
  - crypto1.cpp/h — Crypto1 stream cipher (port crapto1 reference implementation to ESP32)

- /attacks
  - dictionary_attack.cpp/h — Common/default key dictionary attack (~500-600 keys)
  - nested_attack.cpp/h — Nested authentication attack using weak PRNG exploitation
  - darkside_attack.cpp/h — Darkside attack for zero-knowledge scenarios

- /storage
  - dump_manager.cpp/h — Save card dumps to SPIFFS/SD in Proxmark3-compatible .mfd format

- /output
  - serial_reporter.cpp/h — Structured JSON output over Serial (115200 baud)
  - web_server.cpp/h — ESP32 WiFi + WebSocket-based live dashboard

- /ui
  - display_menu.cpp/h — Optional SSD1306/TFT menu and progress display

- main.cpp — Entry point: initialize hardware → detect card → dictionary → nested → darkside → save dump

---

## Serial Output (JSON Format)
All events must be emitted as JSON lines over Serial so any companion app can parse them easily.

Every message follows this envelope:
{"event": "<type>", "data": { ... }}

Required event types:

{"event": "card_detected",  "data": {"uid": "A1B2C3D4", "type": "MIFARE_1K", "sectors": 16}}
{"event": "attack_start",   "data": {"attack": "dictionary", "sector": 3, "key_type": "A"}}
{"event": "key_found",      "data": {"sector": 3, "key_type": "A", "key": "FFFFFFFFFFFF", "method": "dictionary"}}
{"event": "key_failed",     "data": {"sector": 3, "key_type": "A"}}
{"event": "data_read",      "data": {"sector": 3, "block": 0, "hex": "0102030405060708090A0B0C0D0E0F10"}}
{"event": "attack_summary", "data": {"total_sectors": 16, "cracked": 14, "failed": 2, "elapsed_ms": 38200}}
{"event": "dump_saved",     "data": {"path": "/sdcard/dump_A1B2C3D4.mfd"}}
{"event": "error",          "data": {"code": "CARD_REMOVED", "msg": "Card removed mid-attack"}}

All Serial output must be valid JSON (no plain-text mixed in). Prefix debug-only messages with a comment char if needed but keep them disabled in release mode.

---

## ESP32 Web Dashboard (Built-in Web Server)
Implement a self-hosted live dashboard served directly from the ESP32 over WiFi.

### Server Side (web_server.cpp)
- Use AsyncWebServer + AsyncWebSocket libraries
- On boot, connect to WiFi using credentials from config.h (WIFI_SSID, WIFI_PASS)
- Serve a single-page dashboard at http://<esp32-ip>/ from SPIFFS (/data/index.html)
- Open a WebSocket endpoint at ws://<esp32-ip>/ws
- Every time serial_reporter emits a JSON event, also broadcast it to all connected WebSocket clients
- REST endpoints:
  - GET  /api/status   → current CardDump state as JSON (keys found, sectors cracked, uid)
  - GET  /api/dump     → download the raw .mfd dump file
  - POST /api/attack   → body: {"attack": "dictionary"|"nested"|"darkside"} — start an attack remotely
  - POST /api/wifi     → update WiFi credentials at runtime

### Client Side (SPIFFS /data/index.html)
Build a clean, single-file HTML+CSS+JS dashboard (no external CDN dependencies — everything must be inline since the ESP32 has no internet):

Layout:
┌─────────────────────────────────────────────┐
│  MifareBreaker         ● Connected  [WiFi]  │
├────────────┬────────────────────────────────┤
│            │  UID: A1B2C3D4                  │
│  SECTOR    │  Type: MIFARE Classic 1K        │
│  GRID      │  Sectors: 16  Cracked: 14/16   │
│            │──────────────────────────────  │
│  S00 ████  │  LIVE LOG                       │
│  S01 ████  │  [key_found] S03 KeyA: FFFF..  │
│  S02 ████  │  [key_found] S04 KeyA: A0A1..  │
│  S03 ░░░░  │  [attack_start] Nested S12..   │
│  ...       │  [data_read] S05 B0: 01 02..   │
│            │──────────────────────────────  │
│            │  [▶ Dictionary] [▶ Nested]      │
│            │  [▶ Darkside]   [⬇ Dump]       │
└────────────┴────────────────────────────────┘

Requirements:
- Sector grid: each sector shown as a colored tile (green = fully cracked, yellow = partial, red = failed, gray = untouched). Clicking a tile shows its keys + raw data blocks.
- Live log panel: scrolling feed of incoming WebSocket events, color-coded by event type.
- Attack buttons: clicking triggers POST /api/attack. Disable buttons while an attack is running.
- Download Dump button: triggers GET /api/dump to download the .mfd file.
- Auto-reconnect WebSocket if connection drops.
- Dark theme, monospace font for hex data, responsive layout.
- Everything must work offline — zero external JS/CSS libraries, pure vanilla HTML+CSS+JS.

---

## Companion Python Script (tools/monitor.py)
Also provide a standalone Python 3 script as an alternative to the web dashboard for users without WiFi:

- Connect to ESP32 via Serial port (configurable via CLI arg: --port COM3 --baud 115200)
- Parse incoming JSON events line by line
- Display a live terminal UI using the `rich` library:
  - Top panel: card info (UID, type, sector count)
  - Middle panel: sector table (sector number | Key A | Key B | status)
  - Bottom panel: scrolling event log
- On receiving dump_saved event, optionally pull the dump via /api/dump if --ip <esp32-ip> is also provided
- Save a local copy of all events to a timestamped .jsonl log file
- CLI usage:
  python monitor.py --port /dev/ttyUSB0 --baud 115200 [--ip 192.168.1.x]

---

## config.h (Top-level configuration)
#define WIFI_SSID        "your_ssid"
#define WIFI_PASS        "your_password"
#define SERIAL_BAUD      115200
#define SPI_SS_PIN       5
#define SPI_RST_PIN      22
#define SPI_MOSI_PIN     23
#define SPI_MISO_PIN     19
#define SPI_SCK_PIN      18
#define DICT_PATH        "/sdcard/keys.txt"
#define DUMP_PATH_PREFIX "/sdcard/dump_"
#define ATTACK_TIMEOUT_MS 30000
#define USE_DISPLAY      false   // set true if SSD1306 is wired

---

## General Requirements
1. Raw SPI register access for RC522 timing-critical operations (nested/darkside)
2. Clean modular C++, no monolithic files, well-commented
3. All Serial output strictly JSON (serial_reporter handles this centrally — no raw Serial.print elsewhere)
4. web_server and serial_reporter share the same internal event bus (callback/observer pattern) so adding a new output channel later is easy
5. Basic error handling: SPI failures, card removed mid-attack, WiFi disconnect, SPIFFS mount fail
6. Target: ESP32 DevKit WROOM-32

## Deliverables
1. Full ESP32 firmware (all .cpp/.h files with complete implementations or clearly documented stubs)
2. SPIFFS /data/index.html — complete single-file dashboard
3. tools/monitor.py — complete Python companion script
4. README.md — wiring diagram, SPIFFS upload steps, Python setup, WiFi config instructions