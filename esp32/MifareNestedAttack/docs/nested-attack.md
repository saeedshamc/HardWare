# Nested Attack — educational concept

Nested Attacks historically abused relationships between an already-authenticated Crypto1 session and a second authentication.

This repository provides a **simulator** that walks students through the idea:

```text
Known Auth → Session → State → Nested Auth → Information Leakage (concept)
```

It intentionally stops before key recovery. RC522 + ESP32 is not a Proxmark3.
