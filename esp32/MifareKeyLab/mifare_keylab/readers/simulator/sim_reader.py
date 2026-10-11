from __future__ import annotations

from mifare_keylab.readers.interface.base import CardInfo, KeyType, ReaderInterface


class SimulatorReader(ReaderInterface):
    """
    Hardware-free lab backend.
    Default sector keys (educational defaults):
      sector 0..15 KeyA/KeyB = FFFFFFFFFFFF
      sector 1 KeyA also accepts 112233445566 (demo match from README)
    """

    def __init__(self) -> None:
        self._present = False
        self._authed = False
        self._keys: dict[tuple[int, str], set[str]] = {}
        for s in range(16):
            self._keys[(s, "A")] = {"FFFFFFFFFFFF"}
            self._keys[(s, "B")] = {"FFFFFFFFFFFF"}
        # Sector 1 KeyA matches README demo walk-through (FFF fails, 1122.. succeeds)
        self._keys[(1, "A")] = {"112233445566"}


    def open(self) -> None:
        return

    def close(self) -> None:
        self._present = False
        self._authed = False

    def detect_card(self) -> bool:
        self._present = True
        self._authed = False
        return True

    def get_card_info(self) -> CardInfo:
        if not self._present:
            self.detect_card()
        return CardInfo(
            uid_hex="04 DE AD BE EF",
            atqa="00 04",
            sak="08",
            card_type="MIFARE Classic 1K",
            sectors=16,
            blocks=64,
            reader_name="Simulator",
        )

    def authenticate(self, sector: int, key_type: KeyType, key_hex: str) -> bool:
        if not self._present:
            return False
        key = key_hex.strip().upper().replace(" ", "")
        ok = key in self._keys.get((sector, key_type), set())
        self._authed = ok
        return ok

    def read_block(self, block: int) -> bytes | None:
        if not self._authed:
            return None
        return bytes([block & 0xFF]) * 16

    def stop_crypto1(self) -> None:
        self._authed = False
