# Getting started

```bash
cd esp32/MifareKeyLab
python -m venv .venv
# Windows: .venv\Scripts\activate
pip install -r requirements.txt
pip install -e .

# Simulator (no hardware)
mifare-keylab info --reader simulator
mifare-keylab test --sector 1 --type A --keys data/keys.example.txt --reader simulator

# ESP32 + RC522
cd firmware/esp32 && pio run -t upload
mifare-keylab config reader rc522 --port COM3
mifare-keylab card info --reader rc522 --port COM3
```

Authorized test cards only. See `SECURITY.md`.
