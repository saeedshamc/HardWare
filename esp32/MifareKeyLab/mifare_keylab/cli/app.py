from __future__ import annotations

from pathlib import Path
from typing import Optional

import typer
from rich.console import Console

from mifare_keylab import __version__
from mifare_keylab.authentication.tester import mask_key, run_candidates, run_single
from mifare_keylab.candidates.generator import (
    known_prefix,
    structured_range,
    visualize_space,
)
from mifare_keylab.candidates.validator import load_dictionary, normalize_key
from mifare_keylab.config import load_config, save_config
from mifare_keylab.crypto.demo import crypto1_demo_text
from mifare_keylab.mifare.access_bits.decode import format_decode
from mifare_keylab.nested.simulator import nested_concept_text
from mifare_keylab.readers import create_reader
from mifare_keylab.reports.export import build_report, write_report

app = typer.Typer(
    name="mifare-keylab",
    help="Educational MIFARE Classic security laboratory (authorized test cards only).",
    no_args_is_help=True,
)
card_app = typer.Typer(help="Card commands")
config_app = typer.Typer(help="Configuration")
crypto_app = typer.Typer(help="Crypto1 educational demos")
app.add_typer(card_app, name="card")
app.add_typer(config_app, name="config")
app.add_typer(crypto_app, name="crypto")

console = Console()


def _reader_from_opts(reader: Optional[str], port: Optional[str]):
    cfg = load_config()
    if reader:
        cfg.reader = reader
    if port:
        cfg.serial_port = port
        cfg.reader = cfg.reader if cfg.reader != "simulator" else "rc522"
    r = create_reader(cfg)
    r.open()
    return cfg, r


@app.callback()
def main_callback() -> None:
    """MIFARE KeyLab CLI."""


@app.command("info")
def info_cmd(
    reader: Optional[str] = typer.Option(None, "--reader"),
    port: Optional[str] = typer.Option(None, "--port"),
) -> None:
    """Show tool + current card/reader summary."""
    cfg, r = _reader_from_opts(reader, port)
    try:
        r.detect_card()
        card = r.get_card_info()
        uid = "[REDACTED]" if cfg.privacy else card.uid_hex
        console.print("MIFARE KeyLab")
        console.print("=============\n")
        console.print(f"Reader       : {card.reader_name}")
        console.print(f"Card         : {card.card_type}")
        console.print(f"UID          : {uid}")
        console.print(f"ATQA         : {card.atqa}")
        console.print(f"SAK          : {card.sak}")
        console.print(f"Version      : {__version__}")
    finally:
        r.close()


@card_app.command("info")
def card_info(
    reader: Optional[str] = typer.Option(None, "--reader"),
    port: Optional[str] = typer.Option(None, "--port"),
) -> None:
    cfg, r = _reader_from_opts(reader, port)
    try:
        r.detect_card()
        card = r.get_card_info()
        uid = "[REDACTED]" if cfg.privacy else card.uid_hex
        console.print("Card Information")
        console.print("----------------\n")
        console.print(f"UID       : {uid}")
        console.print(f"ATQA      : {card.atqa}")
        console.print(f"SAK       : {card.sak}")
        console.print(f"\nCard Type : {card.card_type}")
        console.print(f"Sectors   : {card.sectors}")
        console.print(f"Blocks    : {card.blocks}")
    finally:
        r.close()


@app.command("auth")
def auth_cmd(
    sector: int = typer.Option(..., "--sector"),
    key_type: str = typer.Option("A", "--type", help="A or B"),
    key: str = typer.Option(..., "--key"),
    reader: Optional[str] = typer.Option(None, "--reader"),
    port: Optional[str] = typer.Option(None, "--port"),
) -> None:
    """Test one laboratory candidate key."""
    kt = key_type.upper()
    if kt not in {"A", "B"}:
        raise typer.BadParameter("type must be A or B")
    key_n = normalize_key(key)
    if not key_n:
        raise typer.BadParameter("key must be 12 hex characters")

    cfg, r = _reader_from_opts(reader, port)
    try:
        if not r.detect_card():
            console.print("[red]No card detected[/red]")
            raise typer.Exit(1)
        console.print(f"[*] Sector : {sector}")
        console.print(f"[*] Key    : {kt}")
        console.print("[*] Testing candidate...\n")
        res = run_single(r, sector, kt, key_n)  # type: ignore[arg-type]
        if res.ok:
            console.print("[green][+] Authentication successful[/green]\n")
            console.print("Sector:\n    ", sector)
            console.print("\nKey type:\n    ", kt)
            console.print("\nStatus:\n    VALID")
            if not cfg.privacy:
                console.print("\nKey:\n    ", key_n)
            else:
                console.print("\nKey:\n    [REDACTED]")
        else:
            console.print("[red][-] Authentication FAILURE[/red]")
            raise typer.Exit(2)
    finally:
        r.close()


@app.command("test")
def test_cmd(
    sector: int = typer.Option(1, "--sector"),
    key_type: str = typer.Option("A", "--type"),
    keys: Optional[Path] = typer.Option(None, "--keys", exists=True, dir_okay=False),
    start: Optional[str] = typer.Option(None, "--start", help="Structured range start"),
    end: Optional[str] = typer.Option(None, "--end", help="Structured range end"),
    prefix: Optional[str] = typer.Option(None, "--prefix", help="Known prefix e.g. A0A1A2"),
    reader: Optional[str] = typer.Option(None, "--reader"),
    port: Optional[str] = typer.Option(None, "--port"),
) -> None:
    """Candidate-key testing (dictionary / bounded structured / known prefix)."""
    kt = key_type.upper()
    if kt not in {"A", "B"}:
        raise typer.BadParameter("type must be A or B")

    cfg = load_config()
    candidates: list[str] = []
    if keys:
        candidates = load_dictionary(str(keys))
    elif start and end:
        candidates = structured_range(start, end, cfg.max_structured_candidates)
    elif prefix:
        candidates = known_prefix(prefix, cfg.max_structured_candidates)
    else:
        raise typer.BadParameter("Provide --keys FILE, or --start/--end, or --prefix")

    if not candidates:
        console.print("No candidates loaded.")
        raise typer.Exit(1)

    cfg, r = _reader_from_opts(reader, port)
    try:
        if not r.detect_card():
            console.print("[red]No card detected[/red]")
            raise typer.Exit(1)
        card = r.get_card_info()
        console.print("=" * 40)
        console.print(" MIFARE KEYLAB")
        console.print(" Candidate Key Tester")
        console.print("=" * 40)
        console.print(f"\nCard:\n    {card.card_type}")
        console.print(f"\nSector:\n    {sector}")
        console.print(f"\nKey:\n    {kt}")
        console.print(f"\nCandidates:\n    {len(candidates)}\n")
        console.print("-" * 40)
        stats = run_candidates(r, sector, kt, candidates, cfg, console)  # type: ignore[arg-type]
        console.print("-" * 40)
        if stats.matched_key:
            console.print("\n[green][+] Candidate matched[/green]\n")
            console.print("Key:\n    ", mask_key(stats.matched_key, cfg.privacy or cfg.redact_keys))
        else:
            console.print("\n[-] No candidate matched")
        rate = stats.tested / stats.elapsed_sec if stats.elapsed_sec > 0 else 0
        console.print("\nPerformance")
        console.print("-----------")
        console.print(f"Candidates tested : {stats.tested:,}")
        console.print(f"Successes          : {stats.successes}")
        console.print(f"Failures           : {stats.failures}")
        console.print(f"Elapsed             : {stats.elapsed_sec:.2f}s")
        console.print(f"Average rate        : {rate:.1f} candidates/sec")
    finally:
        r.close()


@app.command("space")
def space_cmd(prefix: str = typer.Option("", "--prefix")) -> None:
    """Visualize key-space / remaining entropy for a known prefix."""
    console.print(visualize_space(prefix))


@crypto_app.command("demo")
def crypto_demo() -> None:
    console.print(crypto1_demo_text())


@app.command("nested")
def nested_cmd() -> None:
    """Nested Attack educational simulator (no key recovery)."""
    console.print(nested_concept_text())


@app.command("access")
def access_cmd(
    bits: str = typer.Argument(..., help="three hex bytes e.g. 'FF 0F 00'"),
) -> None:
    parts = bits.replace(",", " ").split()
    if len(parts) != 3:
        raise typer.BadParameter("need exactly 3 hex bytes")
    access = bytes(int(p, 16) for p in parts)
    console.print(format_decode(access))


@app.command("report")
def report_cmd(
    output: Path = typer.Option(Path("report.json"), "--output"),
    reader: Optional[str] = typer.Option(None, "--reader"),
    port: Optional[str] = typer.Option(None, "--port"),
    tests: int = typer.Option(0, "--tests"),
    matches: int = typer.Option(0, "--matches"),
    sectors: str = typer.Option("1", "--sectors", help="comma-separated"),
) -> None:
    cfg, r = _reader_from_opts(reader, port)
    try:
        r.detect_card()
        card = r.get_card_info()
        secs = [int(x) for x in sectors.split(",") if x.strip()]
        payload = build_report(
            card,
            cfg,
            tested_sectors=secs,
            authentication_tests=tests,
            matches=matches,
        )
        write_report(output, payload)
        console.print(f"[+] Wrote {output}")
    finally:
        r.close()


@config_app.command("privacy")
def privacy_cmd(
    enable: bool = typer.Option(False, "--enable"),
    disable: bool = typer.Option(False, "--disable"),
) -> None:
    cfg = load_config()
    if enable:
        cfg.privacy = True
        cfg.redact_uid = True
        cfg.redact_keys = True
    elif disable:
        cfg.privacy = False
    else:
        console.print(f"privacy={cfg.privacy}")
        return
    save_config(cfg)
    console.print(f"[+] privacy={cfg.privacy}")


@config_app.command("reader")
def reader_cfg(
    name: str = typer.Argument(..., help="simulator|rc522"),
    port: Optional[str] = typer.Option(None, "--port"),
) -> None:
    cfg = load_config()
    cfg.reader = name
    if port:
        cfg.serial_port = port
    save_config(cfg)
    console.print(f"[+] reader={cfg.reader} port={cfg.serial_port or '-'}")


@app.command("lab")
def lab_cmd(
    reader: Optional[str] = typer.Option(None, "--reader"),
    port: Optional[str] = typer.Option(None, "--port"),
) -> None:
    """Interactive laboratory menu."""
    cfg = load_config()
    if reader:
        cfg.reader = reader
    if port:
        cfg.serial_port = port

    while True:
        console.print(
            """
=========================================
          MIFARE SECURITY LAB
=========================================

Card
----
[1] Detect
[2] Information
[3] Sector Map

Authentication
--------------
[4] Test Key
[5] Candidate List

Analysis
--------
[6] Access Conditions
[7] Crypto1 Demo
[8] Nested Research

Utilities
---------
[9] Export Report
[0] Exit
"""
        )
        choice = console.input("Select: ").strip()
        if choice == "0":
            break
        if choice == "1":
            info_cmd(reader=cfg.reader, port=cfg.serial_port or None)
        elif choice == "2":
            card_info(reader=cfg.reader, port=cfg.serial_port or None)
        elif choice == "3":
            console.print("Sector map (1K): sectors 0..15, trailer = sector*4+3")
            for s in range(16):
                console.print(f"  Sector {s:02d}: blocks {s*4}..{s*4+3}")
        elif choice == "4":
            sector = int(console.input("Sector: ") or "1")
            kt = (console.input("Type A/B [A]: ") or "A").upper()
            key = console.input("Key (12 hex): ")
            auth_cmd(sector=sector, key_type=kt, key=key, reader=cfg.reader, port=cfg.serial_port or None)
        elif choice == "5":
            path = console.input("keys file path: ").strip()
            sector = int(console.input("Sector [1]: ") or "1")
            test_cmd(
                sector=sector,
                key_type="A",
                keys=Path(path),
                start=None,
                end=None,
                prefix=None,
                reader=cfg.reader,
                port=cfg.serial_port or None,
            )
        elif choice == "6":
            bits = console.input("Access bits (e.g. FF 0F 00): ")
            access_cmd(bits=bits)
        elif choice == "7":
            crypto_demo()
        elif choice == "8":
            nested_cmd()
        elif choice == "9":
            out = console.input("Output path [report.json]: ") or "report.json"
            report_cmd(
                output=Path(out),
                reader=cfg.reader,
                port=cfg.serial_port or None,
                tests=0,
                matches=0,
                sectors="1",
            )
        else:
            console.print("Unknown option")


if __name__ == "__main__":
    app()
