#!/usr/bin/env python3
"""
MifareBreaker — Python Serial Monitor
Requires: pip install pyserial rich requests
Usage:    python monitor.py --port /dev/ttyUSB0 [--baud 115200] [--ip 192.168.1.x]
"""

import argparse, json, sys, threading, time, os
from datetime import datetime
from pathlib import Path

try:
    import serial
    from rich.console import Console
    from rich.table import Table
    from rich.panel import Panel
    from rich.layout import Layout
    from rich.live import Live
    from rich.text import Text
    from rich.columns import Columns
    from rich import box
except ImportError:
    print("Missing deps. Run:  pip install pyserial rich requests")
    sys.exit(1)

# ── Config ──────────────────────────────────────────────────────────────────
MAX_LOG   = 80
KEY_EMPTY = "????????????"

# ── State ───────────────────────────────────────────────────────────────────
class State:
    def __init__(self):
        self.uid       = "—"
        self.card_type = "—"
        self.sectors   = {}       # {idx: {keyA,keyB,statusA,statusB,blocks}}
        self.total     = 0
        self.cracked   = 0
        self.log       = []
        self.active_attack = None
        self.active_sector = None
        self.lock      = threading.Lock()

st = State()

# ── Logging ─────────────────────────────────────────────────────────────────
def log_path():
    ts = datetime.now().strftime("%Y%m%d_%H%M%S")
    return Path(f"mifare_session_{ts}.jsonl")

LOG_FILE = log_path()

def append_log(raw: str):
    with open(LOG_FILE, "a") as f:
        f.write(raw + "\n")

# ── Event handler ────────────────────────────────────────────────────────────
def handle_event(ev: dict):
    d = ev.get("data", {})
    with st.lock:
        ev_type = ev.get("event", "")
        ts = datetime.now().strftime("%H:%M:%S")

        if ev_type == "card_detected":
            st.uid       = d.get("uid", "—")
            st.card_type = d.get("type", "—")
            st.total     = d.get("sectors", 0)
            st.cracked   = 0
            st.sectors   = {}
            for i in range(st.total):
                st.sectors[i] = {"keyA": KEY_EMPTY, "keyB": KEY_EMPTY,
                                 "statusA": "unknown", "statusB": "unknown", "blocks": {}}

        elif ev_type == "key_found":
            s = d.get("sector", -1)
            if s in st.sectors:
                if d.get("key_type") == "A":
                    st.sectors[s]["keyA"]   = d.get("key", KEY_EMPTY)
                    st.sectors[s]["statusA"] = "found"
                else:
                    st.sectors[s]["keyB"]   = d.get("key", KEY_EMPTY)
                    st.sectors[s]["statusB"] = "found"
            st.cracked = sum(1 for s in st.sectors.values()
                             if s["statusA"] == "found" or s["statusB"] == "found")

        elif ev_type == "key_failed":
            s = d.get("sector", -1)
            if s in st.sectors:
                if d.get("key_type") == "A":
                    st.sectors[s]["statusA"] = "failed"
                else:
                    st.sectors[s]["statusB"] = "failed"

        elif ev_type == "data_read":
            s, b = d.get("sector", -1), d.get("block", -1)
            if s in st.sectors:
                st.sectors[s]["blocks"][b] = d.get("hex", "")

        elif ev_type == "attack_start":
            st.active_attack  = d.get("attack", "")
            st.active_sector  = d.get("sector", None)

        elif ev_type == "attack_summary":
            st.active_attack = None
            st.active_sector = None

        # Log entry
        msg = _format_msg(ev_type, d)
        st.log.append((ts, ev_type, msg))
        if len(st.log) > MAX_LOG:
            st.log.pop(0)

def _format_msg(ev_type, d):
    if ev_type == "key_found":
        return f"S{d.get('sector','?')} Key{d.get('key_type','?')}: {d.get('key','?')} ({d.get('method','?')})"
    if ev_type == "key_failed":
        return f"S{d.get('sector','?')} Key{d.get('key_type','?')} not found"
    if ev_type == "card_detected":
        return f"{d.get('uid','?')} {d.get('type','?')} ({d.get('sectors','?')} sectors)"
    if ev_type == "attack_start":
        return f"{d.get('attack','?')} → S{d.get('sector','?')} Key{d.get('key_type','?')}"
    if ev_type == "attack_summary":
        return f"Done: {d.get('cracked','?')}/{d.get('total_sectors','?')} cracked in {d.get('elapsed_ms',0)/1000:.1f}s"
    if ev_type == "data_read":
        return f"S{d.get('sector','?')} B{d.get('block','?')}: {d.get('hex','?')}"
    if ev_type == "error":
        return f"[{d.get('code','?')}] {d.get('msg','?')}"
    if ev_type == "dump_saved":
        return f"Saved → {d.get('path','?')}"
    return str(d)[:80]

# ── Rich UI builder ──────────────────────────────────────────────────────────
def build_ui():
    # Header
    attack_status = (f"[bold yellow]● {st.active_attack.upper()} → S{st.active_sector}[/]"
                     if st.active_attack else "[dim]● idle[/]")
    header = Panel(
        f"  [bold blue]⚡ MifareBreaker[/]   {attack_status}   "
        f"[dim]{datetime.now().strftime('%H:%M:%S')}[/]",
        box=box.SIMPLE
    )

    # Card info
    pct = f"{st.cracked}/{st.total}"
    bar_len = 30
    filled  = int(bar_len * st.cracked / st.total) if st.total else 0
    bar     = "[green]" + "█" * filled + "[/][dim]" + "░" * (bar_len - filled) + "[/]"
    card_panel = Panel(
        f"  UID: [bold]{st.uid}[/]   Type: [bold]{st.card_type}[/]   "
        f"Cracked: [bold green]{pct}[/]\n  {bar}",
        title="[dim]CARD[/]", box=box.ROUNDED, height=5
    )

    # Sector table
    tbl = Table(box=box.SIMPLE_HEAVY, show_header=True, header_style="dim",
                expand=True, min_width=70)
    tbl.add_column("SEC",    style="dim",  width=4)
    tbl.add_column("KEY A",  style="cyan", width=14)
    tbl.add_column("KEY B",  style="cyan", width=14)
    tbl.add_column("STATUS", width=14)
    tbl.add_column("DATA",   style="dim",  width=16)

    for idx in range(st.total):
        s = st.sectors.get(idx, {})
        kA, kB  = s.get("keyA", KEY_EMPTY), s.get("keyB", KEY_EMPTY)
        stA, stB = s.get("statusA","unknown"), s.get("statusB","unknown")
        is_active = (st.active_sector == idx)

        if stA == "found" and stB == "found":
            status = "[bold green]✓ CRACKED[/]"
        elif stA == "found" or stB == "found":
            status = "[bold yellow]~ PARTIAL[/]"
        elif stA == "failed" and stB == "failed":
            status = "[bold red]✗ FAILED[/]"
        elif is_active:
            status = "[bold yellow]⟳ ATTACK[/]"
        else:
            status = "[dim]? UNKNOWN[/]"

        kA_disp  = f"[green]{kA}[/]"  if stA=="found" else f"[dim]{kA}[/]"
        kB_disp  = f"[green]{kB}[/]"  if stB=="found" else f"[dim]{kB}[/]"
        blk_count = len(s.get("blocks", {}))
        data_disp = f"[dim]{blk_count}/4 blocks[/]" if blk_count else "[dim]—[/]"
        sec_disp  = f"[bold yellow]▶S{idx:02d}[/]" if is_active else f"S{idx:02d}"
        tbl.add_row(sec_disp, kA_disp, kB_disp, status, data_disp)

    sector_panel = Panel(tbl, title="[dim]SECTORS[/]", box=box.ROUNDED)

    # Event log
    log_lines = Text()
    EV_STYLE = {
        "key_found":"bold green", "card_detected":"bold blue",
        "attack_start":"yellow", "key_failed":"red",
        "data_read":"magenta", "attack_summary":"bold cyan",
        "error":"bold red", "dump_saved":"green"
    }
    for ts, ev_type, msg in st.log[-18:]:
        style = EV_STYLE.get(ev_type, "dim")
        log_lines.append(f"{ts} ", style="dim")
        log_lines.append(f"[{ev_type}]", style=style)
        log_lines.append(f" {msg}\n", style="white")
    log_panel = Panel(log_lines, title="[dim]LIVE LOG[/]", box=box.ROUNDED, height=22)

    from rich.console import Group
    return Group(header, Columns([card_panel], expand=True),
                 sector_panel, log_panel)

# ── Serial reader thread ─────────────────────────────────────────────────────
def serial_reader(port, baud):
    while True:
        try:
            with serial.Serial(port, baud, timeout=1) as ser:
                print(f"[*] Connected to {port} @ {baud}")
                while True:
                    raw = ser.readline().decode("utf-8", errors="replace").strip()
                    if not raw: continue
                    append_log(raw)
                    try:
                        ev = json.loads(raw)
                        handle_event(ev)
                    except json.JSONDecodeError:
                        pass  # skip non-JSON debug lines
        except serial.SerialException as e:
            print(f"[!] Serial error: {e} — retrying in 3s")
            time.sleep(3)

# ── Optional dump download ────────────────────────────────────────────────────
def download_dump(ip):
    try:
        import requests
        r = requests.get(f"http://{ip}/api/dump", timeout=10)
        if r.status_code == 200:
            fname = f"dump_{datetime.now().strftime('%Y%m%d_%H%M%S')}.mfd"
            with open(fname, "wb") as f:
                f.write(r.content)
            print(f"[+] Dump saved to {fname}")
        else:
            print(f"[!] Dump download failed: HTTP {r.status_code}")
    except Exception as e:
        print(f"[!] Dump download error: {e}")

# ── Entry point ──────────────────────────────────────────────────────────────
def main():
    ap = argparse.ArgumentParser(description="MifareBreaker Serial Monitor")
    ap.add_argument("--port",  required=True,  help="Serial port, e.g. /dev/ttyUSB0 or COM3")
    ap.add_argument("--baud",  default=115200, type=int, help="Baud rate (default 115200)")
    ap.add_argument("--ip",    default=None,   help="ESP32 IP for dump download via HTTP")
    args = ap.parse_args()

    console = Console()
    console.print(f"[bold blue]⚡ MifareBreaker Monitor[/] — [dim]Session log: {LOG_FILE}[/]")

    if args.ip:
        console.print(f"[dim]Dashboard: http://{args.ip}/[/]")

    # Start serial reader thread
    t = threading.Thread(target=serial_reader, args=(args.port, args.baud), daemon=True)
    t.start()

    # Live UI
    try:
        with Live(build_ui(), refresh_per_second=4, screen=False) as live:
            while True:
                with st.lock:
                    ui = build_ui()
                live.update(ui)
                time.sleep(0.25)

                # Auto-download dump when saved
                if args.ip:
                    for _, ev_type, msg in st.log:
                        if ev_type == "dump_saved" and "downloaded" not in msg:
                            download_dump(args.ip)
    except KeyboardInterrupt:
        console.print("\n[dim]Exiting. Session saved to[/] " + str(LOG_FILE))

if __name__ == "__main__":
    main()
