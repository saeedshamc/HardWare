#!/usr/bin/env python3
"""Educational three-pass authentication storyboard (not real Crypto1)."""

from __future__ import annotations

import secrets


def main() -> None:
    print("Crypto1 Authentication Demonstration")
    print("====================================")
    print("(conceptual — classroom use)\n")

    nt = secrets.token_hex(4).upper()
    nr = secrets.token_hex(4).upper()
    print("Reader -> Card : AUTH(sector, KEY_A)")
    print(f"Card   -> Reader: nonce nT = {nt}")
    print(f"Reader -> Card : {nr} || response(nT)")
    print("Card   -> Reader: encrypted answer")
    print()
    print("Result: authenticated session (conceptual)")
    print("Internal cipher state now depends on key + nonces.")
    print()
    print("Lesson: secrecy relies on unpredictable nonces and a strong cipher.")
    print("MIFARE Classic Crypto1 fails that bar for modern access control.")


if __name__ == "__main__":
    main()
