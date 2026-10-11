#!/usr/bin/env python3
"""Toy LFSR demo to illustrate weak PRNG concepts (not Crypto1)."""

from __future__ import annotations


def lfsr_step(state: int) -> int:
    bit = ((state >> 0) ^ (state >> 2) ^ (state >> 3) ^ (state >> 5)) & 1
    return ((state >> 1) | (bit << 31)) & 0xFFFFFFFF


def main() -> None:
    print("PRNG / keystream concept demo")
    print("=============================\n")
    state = 0xA5C3F129
    stream = []
    for _ in range(16):
        state = lfsr_step(state)
        stream.append(f"{state & 0xFF:02X}")
    print("Toy keystream:", " ".join(stream))
    print()
    print("If a real PRNG is predictable across sessions,")
    print("later authentications can leak information about earlier state.")


if __name__ == "__main__":
    main()
