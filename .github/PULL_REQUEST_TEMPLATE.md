# Pull Request: Release MifareBreaker (mifarebreaker-release → main)

## What this PR does
- Adds a complete, modular ESP32 firmware for MIFARE Classic auditing (MifareBreaker):
  - HAL: RC522 SPI driver and ISO14443-A framing (hal/)
  - Core: UID detection, CardDump and key-store (core/)
  - Crypto: development-only Crypto1-like LFSR (crypto/) for testing attacks
  - Attacks: dictionary (builtin + SD), nested (heuristic), darkside (best-effort)
  - Output: serial JSON reporter (output/serial_reporter) and web dashboard with WebSocket and REST (output/web_server)
  - Storage: dump saving to SD (.mfd placeholder) and /api/dump download (storage/)
  - UI: data/index.html single-file offline dashboard
  - Tools: tools/monitor.py (Python monitor using rich)
  - Docs: README, FINAL_CHECKLIST, ISSUE_GUIDE, RELEASE_NOTES, platformio.ini, .gitignore

## Testing performed
- Static code review and API consistency checks
- Basic compile-oriented adjustments (PlatformIO config included)
- No hardware-in-the-loop tests performed in this PR (ESP32 + RC522 + SD hardware testing required)

## Remaining work / TODO
1. Integrate a production-grade Crypto1 (crapto1 from Proxmark3) for higher success in nested/darkside attacks — note: this code is GPL and has license implications if merged. Owner decision required.
2. Hardware testing on ESP32 DevKit with RC522 + SD to tune timeouts, retries and fix timing-related issues.
3. Extensive fuzzing and memory profiling for long-running attacks.
4. Optional: add unit tests and CI checks (build, format) before merge.

## How to test locally
- See `esp32/MifareBreaker/FINAL_CHECKLIST.md` for wiring, build and SPIFFS upload steps.

## License / legal note
- Current work in this branch is authored in this repository. It includes a development-only Crypto1 LFSR for testing and does NOT vendor Proxmark3 crapto1.
- Integrating Proxmark3's Crypto1 implementation would require accepting GPL terms. Do not integrate GPL code without explicit approval from the repository owner.

Please review and, if acceptable, merge `mifarebreaker-release` into `main`. If you want me to merge this PR after review, reply with `merge` and I will proceed.