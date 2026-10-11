from __future__ import annotations

import re

_KEY_RE = re.compile(r"^[0-9A-Fa-f]{12}$")


def normalize_key(raw: str) -> str | None:
    s = raw.strip().replace(" ", "").replace(":", "").upper()
    if not s or s.startswith("#"):
        return None
    if not _KEY_RE.match(s):
        return None
    return s


def load_dictionary(path: str) -> list[str]:
    keys: list[str] = []
    with open(path, encoding="utf-8") as f:
        for line in f:
            k = normalize_key(line)
            if k:
                keys.append(k)
    return keys
