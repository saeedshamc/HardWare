#!/usr/bin/env bash
# Helper script to upload SPIFFS using PlatformIO
# Usage: ./upload_spiffs.sh

set -e
PROJECT_DIR="$(cd "$(dirname "$0")/.." && pwd)/esp32/MifareBreaker"
cd "$PROJECT_DIR"
if ! command -v pio >/dev/null 2>&1; then
  echo "PlatformIO (pio) not found in PATH. Install PlatformIO or use Arduino IDE."
  exit 1
fi
pio run --target uploadfs
