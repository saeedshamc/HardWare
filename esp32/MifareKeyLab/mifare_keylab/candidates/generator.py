from __future__ import annotations

from mifare_keylab.candidates.validator import normalize_key


def structured_range(start_hex: str, end_hex: str, limit: int) -> list[str]:
    """Bounded inclusive range of 6-byte keys. Refuses if span > limit."""
    start = normalize_key(start_hex)
    end = normalize_key(end_hex)
    if not start or not end:
        raise ValueError("start/end must be 12 hex chars")
    a = int(start, 16)
    b = int(end, 16)
    if b < a:
        a, b = b, a
    span = b - a + 1
    if span > limit:
        raise ValueError(
            f"range has {span} candidates; max allowed is {limit}. "
            "Refuse unbounded brute force."
        )
    return [f"{i:012X}" for i in range(a, b + 1)]


def known_prefix(prefix_hex: str, limit: int) -> list[str]:
    """
    prefix_hex like A0A1A2 or A0A1A2?????? .
    Remaining trailing nibbles must keep candidate count <= limit.
    """
    cleaned = prefix_hex.strip().upper().replace(" ", "")
    # allow trailing ?
    while cleaned.endswith("?"):
        cleaned = cleaned[:-1]
    raw = cleaned.replace("?", "")
    if len(raw) > 12 or len(raw) % 2 != 0:
        raise ValueError("prefix must be even-length hex (0..12 chars)")
    if raw and any(c not in "0123456789ABCDEF" for c in raw):
        raise ValueError("prefix contains invalid hex")
    unknown_nibbles = 12 - len(raw)
    count = 16**unknown_nibbles
    if count > limit:
        raise ValueError(
            f"prefix leaves {count} candidates; max allowed is {limit}"
        )
    prefix_val = int(raw, 16) if raw else 0
    shift = unknown_nibbles * 4
    return [f"{((prefix_val << shift) | i):012X}" for i in range(count)]


def visualize_space(prefix_hex: str = "") -> str:
    full = 2**48
    lines = [
        "Full key space",
        "",
        "2^48",
        "│",
        f"│  {full:,}",
        "└───────────────────────────────────────────",
        "",
    ]
    cleaned = prefix_hex.strip().upper().replace(" ", "")
    while cleaned.endswith("?"):
        cleaned = cleaned[:-1]
    raw = cleaned.replace("?", "")
    if raw:
        unknown = 12 - len(raw)
        rem_bits = unknown * 4
        rem = 16**unknown
        lines += [
            f"Known prefix: {raw}{'?' * unknown}",
            f"Remaining: {rem_bits} bits",
            "",
            f"2^{rem_bits}",
            "│",
            f"│ {rem:,} candidates",
            "└──────────────────────",
        ]
    return "\n".join(lines)
