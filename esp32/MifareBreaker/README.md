# ⚡ MifareBreaker

Security research tool for auditing MIFARE Classic RFID cards.
**For authorized testing of your own hardware only.**

---

## Wiring — RC522 → ESP32 (VSPI)

| RC522 Pin | ESP32 Pin | Notes           |
|-----------|-----------|-----------------|
| SDA (SS)  | GPIO 5    | Chip Select     |
| SCK       | GPIO 18   | SPI Clock       |
| MOSI      | GPIO 23   | SPI Data Out    |
| MISO      | GPIO 19   | SPI Data In     |
| RST       | GPIO 22   | Reset           |
| GND       | GND       |                 |
| 3.3V      | 3.3V      | NOT 5V!         |

## Dependencies (Arduino / PlatformIO)

```
ESP32 Arduino Core >= 2.0
ESPAsyncWebServer   (https://github.com/me-no-dev/ESPAsyncWebServer)
AsyncTCP            (https://github.com/me-no-dev/AsyncTCP)
SD                  (built-in)
SPIFFS              (built-in)
```

## SPIFFS Upload

Upload `/data/index.html` to ESP32 SPIFFS:
1. Install "ESP32 Sketch Data Upload" plugin in Arduino IDE
2. Place `data/index.html` inside your sketch folder
3. Tools → ESP32 Sketch Data Upload

## Config

Edit `config.h`:
- `WIFI_SSID` / `WIFI_PASS` — your network credentials
- `PIN_SS`, `PIN_RST`, etc. — GPIO pin assignments

## Python Monitor

```bash
pip install pyserial rich requests
python tools/monitor.py --port /dev/ttyUSB0 --baud 115200
# With web dashboard download:
python tools/monitor.py --port /dev/ttyUSB0 --ip 192.168.1.x
```

## Adding Keys to Dictionary

Place a `keys.txt` file on your SD card:
```
FFFFFFFFFFFF
A0A1A2A3A4A5
D3F7D3F7D3F7
```
One key per line, 12 hex characters, no spaces.

## Attack Pipeline

1. **Dictionary** — tries ~25 built-in keys + SD card list
2. **Nested** — exploits weak PRNG after any one known key
3. **Darkside** — zero-knowledge attack (slow, ~5–30 min/sector)

> **Note:** Nested and Darkside key-recovery math are marked TODO.
> Integrate `crapto1.c` from Proxmark3 firmware for full functionality:
> https://github.com/RfidResearchGroup/proxmark3

## Output Formats

- **Serial:** newline-delimited JSON (`{"event":"...","data":{...}}`)
- **Web Dashboard:** `http://<esp32-ip>/` (live WebSocket updates)
- **Dump:** Proxmark3-compatible `.mfd` binary on SD + SPIFFS

## Project Structure

```
MifareBreaker/
├── config.h                  Global configuration
├── main.cpp                  Entry point & attack pipeline
├── hal/
│   ├── rc522_driver.h/.cpp   Low-level SPI + ISO14443-A framing
│   └── card_comm.h/.cpp      Anticollision, auth, read
├── core/
│   ├── key_store.h/.cpp      CardDump struct + key management
│   └── uid_reader.h/.cpp     Card detection & type identification
├── crypto/
│   └── crypto1.h/.cpp        Crypto1 48-bit LFSR cipher
├── attacks/
│   ├── dictionary_attack.*   Built-in + SD key dictionary
│   ├── nested_attack.*       Nested auth attack (PRNG exploitation)
│   └── darkside_attack.*     Courtois darkside attack
├── output/
│   ├── serial_reporter.*     JSON event bus + Serial output
│   └── web_server.*          AsyncWebServer + WebSocket dashboard
├── storage/
│   └── dump_manager.*        .mfd file save/load/print
├── data/
│   └── index.html            Self-contained web dashboard
└── tools/
    └── monitor.py            Python terminal monitor (rich UI)
```

## Legal Notice

This tool is for authorized security research and testing of your own
hardware only. Unauthorized use against third-party systems is illegal.
