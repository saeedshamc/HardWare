from __future__ import annotations

import time

import serial

from mifare_keylab.readers.interface.base import CardInfo, KeyType, ReaderInterface


class Rc522SerialReader(ReaderInterface):
    """Talks to KeyLab ESP32 firmware over USB serial (line protocol)."""

    def __init__(self, port: str, baud: int = 115200) -> None:
        self.port = port
        self.baud = baud
        self._ser: serial.Serial | None = None

    def open(self) -> None:
        self._ser = serial.Serial(self.port, self.baud, timeout=2)
        time.sleep(0.4)
        self._drain()

    def close(self) -> None:
        if self._ser and self._ser.is_open:
            self._ser.close()
        self._ser = None

    def _drain(self) -> None:
        assert self._ser
        time.sleep(0.05)
        self._ser.reset_input_buffer()

    def _cmd(self, line: str) -> list[str]:
        assert self._ser
        self._drain()
        self._ser.write((line.strip() + "\n").encode("utf-8"))
        self._ser.flush()
        lines: list[str] = []
        deadline = time.time() + 3.0
        while time.time() < deadline:
            raw = self._ser.readline().decode("utf-8", errors="ignore").strip()
            if not raw:
                continue
            lines.append(raw)
            if raw.startswith("OK") or raw.startswith("ERR") or raw.startswith("FAIL"):
                break
        return lines

    def detect_card(self) -> bool:
        lines = self._cmd("DETECT")
        return any(l.startswith("OK") for l in lines)

    def get_card_info(self) -> CardInfo:
        lines = self._cmd("INFO")
        data = {"UID": "", "ATQA": "", "SAK": "", "TYPE": "MIFARE Classic 1K"}
        for l in lines:
            if ":" in l:
                k, v = l.split(":", 1)
                data[k.strip().upper()] = v.strip()
        return CardInfo(
            uid_hex=data.get("UID", ""),
            atqa=data.get("ATQA", ""),
            sak=data.get("SAK", ""),
            card_type=data.get("TYPE", "MIFARE Classic 1K"),
            sectors=16,
            blocks=64,
            reader_name="ESP32+RC522",
        )

    def authenticate(self, sector: int, key_type: KeyType, key_hex: str) -> bool:
        key = key_hex.strip().upper().replace(" ", "")
        lines = self._cmd(f"AUTH {sector} {key_type} {key}")
        return any(l.startswith("OK") for l in lines)

    def read_block(self, block: int) -> bytes | None:
        lines = self._cmd(f"READ {block}")
        for l in lines:
            if l.startswith("DATA:"):
                hexpart = l.split(":", 1)[1].strip().replace(" ", "")
                return bytes.fromhex(hexpart)
        return None

    def stop_crypto1(self) -> None:
        self._cmd("HALT")
