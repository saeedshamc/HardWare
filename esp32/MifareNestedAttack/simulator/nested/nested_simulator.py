#!/usr/bin/env python3
"""
Nested Attack *concept* simulator for classrooms.

Does not implement real Crypto1 nested key recovery.
"""

from __future__ import annotations

import secrets


def pause(msg: str = "Enter...") -> None:
    try:
        input(msg)
    except EOFError:
        pass


def main() -> None:
    print("=" * 40)
    print("       NESTED ATTACK SIMULATOR")
    print("=" * 40)
    print("Educational model — no real key recovery\n")

    steps = {
        "1": "Load known authentication",
        "2": "Generate card nonce",
        "3": "Establish Crypto1 state",
        "4": "Start nested authentication",
        "5": "Analyze leaked information",
        "6": "Visualize attack concept",
        "0": "Exit",
    }

    auth = False
    state = False
    nonce = None

    while True:
        for k, v in steps.items():
            print(f"[{k}] {v}")
        choice = input("\nSelect: ").strip()

        if choice == "0":
            break
        if choice == "1":
            print("Known Key -> Authentication #1 -> Crypto session")
            auth = True
            state = False
            print("[+] Known authentication context loaded (simulated)")
        elif choice == "2":
            nonce = secrets.token_hex(4).upper()
            print(f"Card nonce nT (demo): {nonce}")
        elif choice == "3":
            if not auth:
                print("[-] Load known authentication first")
            else:
                print("Auth #1 mixes key + nonces into internal cipher state.")
                state = True
                print("[+] Crypto1 state established (conceptual)")
        elif choice == "4":
            if not state:
                print("[-] Establish Crypto1 state first")
            else:
                n2 = secrets.token_hex(4).upper()
                print("While in session #1, start Authentication #2 (nested).")
                print(f"Nested nonce (demo): {n2}")
                print("Conceptually, session #2 is no longer independent.")
        elif choice == "5":
            print(
                """
Historical weakness (simplified):

  Auth #1  -->  Crypto State
                    |
                    | relationship / PRNG weakness
                    v
  Auth #2  -->  Information Leakage
                    |
                    v
            Candidate key space shrinks

This lab stops at the concept. It does not crack keys.
"""
            )
        elif choice == "6":
            print(
                """
Known Authentication
        |
        v
Authenticated Session
        |
        v
Crypto1 State Relationship
        |
        v
Second Authentication
        |
        v
Information Leakage

Lesson: "We use encryption" != "the system is secure".
"""
            )
        else:
            print("Unknown option")
        print()


if __name__ == "__main__":
    main()
