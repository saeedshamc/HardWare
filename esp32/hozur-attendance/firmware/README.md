# Hozur Firmware (ESP32)

PlatformIO + Arduino framework.

```bash
cp include/secrets.example.h include/secrets.h
# edit secrets.h
pio run -t upload
pio device monitor
```

See [`../docs/wiring.md`](../docs/wiring.md) and [`../docs/setup.md`](../docs/setup.md).

## Modules

| Path | Role |
|------|------|
| `src/rfid/` | RC522 read / enroll / halt |
| `src/security/` | HMAC key derivation, CRC16, Access Bits |
| `src/display/` | TFT_eSPI wrapper + bitmap blit |
| `src/ui/` | Idle / success / denied / duplicate |
| `src/net/` | WiFi + signed HTTP |
| `src/storage/` | NVS secrets + LittleFS offline queue |
