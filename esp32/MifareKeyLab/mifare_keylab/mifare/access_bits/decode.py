from __future__ import annotations


def validate(access: bytes) -> bool:
    if len(access) < 3:
        return False
    c1_inv = access[0] & 0x0F
    c2_inv = (access[0] >> 4) & 0x0F
    c3_inv = access[1] & 0x0F
    c1 = (access[1] >> 4) & 0x0F
    c2 = access[2] & 0x0F
    c3 = (access[2] >> 4) & 0x0F
    return (c1 ^ c1_inv) == 0x0F and (c2 ^ c2_inv) == 0x0F and (c3 ^ c3_inv) == 0x0F


def conditions(access: bytes, idx: int) -> tuple[int, int, int]:
    c1 = ((access[1] >> 4) >> idx) & 1
    c2 = ((access[2] & 0x0F) >> idx) & 1
    c3 = (((access[2] >> 4) & 0x0F) >> idx) & 1
    return c1, c2, c3


def format_decode(access: bytes) -> str:
    lines = [
        f"Access Bits: {access[0]:02X} {access[1]:02X} {access[2]:02X}",
        f"Valid: {validate(access)}",
    ]
    for i in range(4):
        c1, c2, c3 = conditions(access, i)
        label = "trailer" if i == 3 else f"data block {i}"
        lines.append(f"{label}: C1 C2 C3 = {c1} {c2} {c3}")
    return "\n".join(lines)
