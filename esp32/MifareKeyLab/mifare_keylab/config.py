from __future__ import annotations

import json
from pathlib import Path

from pydantic import BaseModel, Field

CONFIG_PATH = Path.home() / ".mifare-keylab.json"


class AppConfig(BaseModel):
    privacy: bool = True
    reader: str = "simulator"  # simulator | rc522
    serial_port: str = ""
    baud: int = 115200
    redact_uid: bool = True
    redact_keys: bool = True
    max_structured_candidates: int = Field(default=1_000_000, ge=1)


def load_config() -> AppConfig:
    if CONFIG_PATH.is_file():
        try:
            return AppConfig.model_validate_json(CONFIG_PATH.read_text(encoding="utf-8"))
        except Exception:
            pass
    return AppConfig()


def save_config(cfg: AppConfig) -> None:
    CONFIG_PATH.write_text(cfg.model_dump_json(indent=2), encoding="utf-8")
