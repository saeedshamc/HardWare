from __future__ import annotations

from abc import ABC, abstractmethod
from dataclasses import dataclass
from typing import Literal


KeyType = Literal["A", "B"]


@dataclass
class CardInfo:
    uid_hex: str
    atqa: str
    sak: str
    card_type: str
    sectors: int
    blocks: int
    reader_name: str


class ReaderInterface(ABC):
    @abstractmethod
    def open(self) -> None: ...

    @abstractmethod
    def close(self) -> None: ...

    @abstractmethod
    def detect_card(self) -> bool: ...

    @abstractmethod
    def get_card_info(self) -> CardInfo: ...

    @abstractmethod
    def authenticate(self, sector: int, key_type: KeyType, key_hex: str) -> bool: ...

    @abstractmethod
    def read_block(self, block: int) -> bytes | None: ...

    @abstractmethod
    def stop_crypto1(self) -> None: ...
