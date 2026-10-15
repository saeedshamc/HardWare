# MifareBreaker — ESP32 Firmware

Near-complete ESP32 + RC522 lab firmware for **authorized** testing of your own MIFARE Classic cards.

## Status

| Area | State |
|------|--------|
| HAL (RC522 / card_comm) | Implemented |
| UID / key store | Implemented |
| Dictionary (builtin + SD `/keys.txt`) | Implemented |
| Nested / Darkside | Heuristic / best-effort only (not Proxmark-grade Crypto1) |
| Crypto1 | Lightweight LFSR for development — not production crapto1 |
| JSON serial + web dashboard + monitor.py | Implemented |
| Dump to SD | Simple `.mfd`-like binary |

## Wiring

| RC522 | ESP32 |
|-------|-------|
| SDA/SS | GPIO 5 (`PIN_SS`) |
| SCK | GPIO 18 |
| MOSI | GPIO 23 |
| MISO | GPIO 19 |
| RST | GPIO 22 |
| 3.3V / GND | 3.3V / GND (**not 5V**) |

| SD (optional) | ESP32 |
|---------------|-------|
| CS | GPIO 4 (`SD_CS_PIN`) |
| MOSI/MISO/SCK | shared VSPI with RC522 |

Put dictionary keys on the SD card root as **`/keys.txt`** (12 hex chars per line). Sample: `data/keys_sample.txt`.

## Config

Edit [`config.h`](./config.h):

- `WIFI_SSID` / `WIFI_PASS` — your LAN (placeholders by default)
- `PIN_*` / `SD_CS_PIN` — match your wiring

Do not commit real WiFi passwords.

## Build & flash (PlatformIO)

```bash
cd esp32/MifareBreaker
pio run -t upload
pio run -t uploadfs          # data/index.html → SPIFFS
pio device monitor
```

## Python monitor

```bash
pip install pyserial rich requests
python tools/monitor.py --port COM3 --baud 115200
# optional dump pull:
python tools/monitor.py --port COM3 --ip 192.168.1.x
```

## Legal

Authorized research on hardware/cards you own only. Unauthorized use is illegal.

See `LICENSE_NOTE.md` / `THIRD_PARTY.md` before importing GPL Proxmark3 crypto.
