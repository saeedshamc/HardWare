# KeyLab ESP32 + RC522 backend

```bash
pio run -t upload
pio device monitor
```

Host:

```bash
mifare-keylab card info --reader rc522 --port COMx
```

Pins: SS=5 SCK=18 MOSI=23 MISO=19 RST=22 — **3.3V only**.
