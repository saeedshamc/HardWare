from __future__ import annotations

import json
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

from mifare_keylab.config import AppConfig
from mifare_keylab.readers.interface.base import CardInfo


def build_report(
    card: CardInfo,
    cfg: AppConfig,
    *,
    tested_sectors: list[int],
    authentication_tests: int,
    matches: int,
    extra: dict[str, Any] | None = None,
) -> dict[str, Any]:
    uid = "[REDACTED]" if cfg.redact_uid or cfg.privacy else card.uid_hex
    payload = {
        "card_type": card.card_type,
        "uid": uid,
        "reader": card.reader_name,
        "tested_sectors": tested_sectors,
        "authentication_tests": authentication_tests,
        "matches": matches,
        "timestamp": datetime.now(timezone.utc).isoformat(),
    }
    if extra:
        payload.update(extra)
    return payload


def write_report(path: str | Path, report: dict[str, Any]) -> None:
    Path(path).write_text(json.dumps(report, indent=2), encoding="utf-8")
