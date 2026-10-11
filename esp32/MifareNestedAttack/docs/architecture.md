# Architecture

```text
ESP32
  ├── CLI (serial menu)
  ├── RFID reader (MFRC522 / SPI)
  ├── MIFARE helpers (sectors/blocks)
  ├── Authentication demo (known lab keys)
  ├── Access-bits decoder
  ├── Crypto1 *concept* demo
  ├── Nested Attack *simulator* (no key recovery)
  └── Defensive analysis module

Host (optional)
  ├── simulator/crypto1
  └── simulator/nested
```

Hardware path: RF 13.56 MHz → RC522 → SPI → ESP32 → Serial UI.
