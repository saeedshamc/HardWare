# MIFARE Classic 1K (lab notes)

- 16 sectors × 4 blocks × 16 bytes = 1024 bytes
- Last block of each sector = sector trailer (KeyA + Access Bits + KeyB)
- Sector 0 block 0 contains manufacturer data / UID (treat as read-only in labs)

See the project README for the full classroom narrative.
