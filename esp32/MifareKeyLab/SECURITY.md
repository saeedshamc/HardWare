# Security & Authorized Use

MIFARE KeyLab is for:

- cards you own
- dedicated laboratory / classroom test cards
- authorized security research

Do **not** use against workplace, university, transit, hotel, or payment credentials without explicit authorization.

## Built-in safety defaults

1. No automatic targeting of arbitrary cards.
2. Laboratory / explicit CLI actions required.
3. Candidate files must be supplied by the user.
4. Unbounded 48-bit brute force is disabled.
5. Keys and UIDs are redacted by default in reports / privacy mode.
6. Nested module is an educational simulator (no real key recovery).
7. Simulation backend works without hardware.

## Reader note

ESP32 + RC522 is for learning and controlled candidate testing.
Chameleon Ultra / Proxmark3 backends are stubs for future work.
