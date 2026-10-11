# ESP32 Firmware — MIFARE Security Lab

```bash
cd firmware/esp32
pio run -t upload
pio device monitor
```

Pins: SS=5, SCK=18, MOSI=23, MISO=19, RST=22 — **RC522 @ 3.3V only**.

Optional compile flag: `-DLAB_SHOW_KEYS=1` to show trailer keys in sector analyzer (instructor mode).
