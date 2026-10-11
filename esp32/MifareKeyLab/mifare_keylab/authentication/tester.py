from __future__ import annotations

import time
from dataclasses import dataclass, field

from rich.console import Console
from rich.progress import BarColumn, Progress, TextColumn, TimeElapsedColumn

from mifare_keylab.config import AppConfig
from mifare_keylab.readers.interface.base import KeyType, ReaderInterface


@dataclass
class TestStats:
    tested: int = 0
    successes: int = 0
    failures: int = 0
    matched_key: str | None = None
    elapsed_sec: float = 0.0
    errors: int = 0


@dataclass
class AuthResult:
    ok: bool
    sector: int
    key_type: KeyType
    key_hex: str


def mask_key(key: str, privacy: bool) -> str:
    return "[REDACTED]" if privacy else key


def run_single(
    reader: ReaderInterface,
    sector: int,
    key_type: KeyType,
    key_hex: str,
) -> AuthResult:
    ok = reader.authenticate(sector, key_type, key_hex)
    reader.stop_crypto1()
    return AuthResult(ok=ok, sector=sector, key_type=key_type, key_hex=key_hex)


def run_candidates(
    reader: ReaderInterface,
    sector: int,
    key_type: KeyType,
    keys: list[str],
    cfg: AppConfig,
    console: Console | None = None,
    stop_on_success: bool = True,
) -> TestStats:
    console = console or Console()
    stats = TestStats()
    t0 = time.perf_counter()
    total = len(keys)

    with Progress(
        TextColumn("[progress.description]{task.description}"),
        BarColumn(),
        TextColumn("{task.completed}/{task.total}"),
        TimeElapsedColumn(),
        console=console,
        transient=False,
    ) as progress:
        task = progress.add_task("Testing candidates...", total=total)
        for idx, key in enumerate(keys, start=1):
            try:
                ok = reader.authenticate(sector, key_type, key)
                reader.stop_crypto1()
            except Exception:
                stats.errors += 1
                stats.tested += 1
                progress.advance(task)
                continue

            stats.tested += 1
            shown = mask_key(key, cfg.privacy or cfg.redact_keys)
            if ok:
                stats.successes += 1
                stats.matched_key = key
                console.print(f"[{idx:02d}/{total:02d}] {shown}   SUCCESS")
                progress.advance(task)
                if stop_on_success:
                    break
            else:
                stats.failures += 1
                console.print(f"[{idx:02d}/{total:02d}] {shown}   FAIL")
                progress.advance(task)

    stats.elapsed_sec = time.perf_counter() - t0
    return stats
