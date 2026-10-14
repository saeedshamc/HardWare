# How to collect useful logs for debugging

When you run MifareBreaker for the first time, collect these artifacts if you need help debugging:

1. Serial JSON log (full session) — run:
   python tools/monitor.py --port /dev/ttyUSB0 --baud 115200 > session.jsonl

2. Screenshot of web dashboard (optional)

3. Contents of SD card (keys.txt) used

4. Exact config.h you used (mask passwords if needed)

Attach these to an issue or paste here and I will analyze and tune timings and retries.
