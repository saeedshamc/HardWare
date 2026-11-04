#!/usr/bin/env python3
"""
MifareBreaker — Serial JSON Monitor
Usage: python monitor.py --port /dev/ttyUSB0 [--baud 115200] [--ip 192.168.x.x]
Deps:  pip install pyserial rich requests
"""
import argparse, serial, json, time, threading
from pathlib import Path
from datetime import datetime

try:
    from rich.console import Console
    from rich.table import Table
    from rich.panel import Panel
    from rich.live import Live
    from rich.text import Text
except ImportError:
    print("Run: pip install pyserial rich")
    raise SystemExit(1)

# ─── Args ─────────────────────────────────────────────────────────────────────
ap = argparse.ArgumentParser(description="MifareBreaker Monitor")
ap.add_argument("--port", required=True, help="Serial port (e.g. /dev/ttyUSB0 or COM3)")
ap.add_argument("--baud", type=int, default=115200)
ap.add_argument("--ip",   default=None, help="ESP32 IP for HTTP dump download")
args = ap.parse_args()

# ─── Shared state ─────────────────────────────────────────────────────────────
state = {
    "uid": "—", "type": "—", "sectors": 0,
    "cracked": 0, "sectors_data": {},
    "log": [], "log_lock": threading.Lock()
}
LOG_FILE = Path(f"mifare_{int(time.time())}.jsonl")

EV_STYLE = {
    "key_found": "bold green", "card_detected": "bold cyan",
    "attack_start": "yellow", "key_failed": "red",
    "data_read": "magenta", "attack_summary": "bold white",
    "error": "red", "dump_saved": "bold green",
}

def _hex(key6_list):
    return "".join(f"{b:02X}" for b in key6_list) if key6_list else "????????????"

def handle(ev):
    t   = ev.get("event", "")
    d   = ev.get("data", {})
    ts  = datetime.now().strftime("%H:%M:%S")

    if t == "card_detected":
        # ── Reset ALL state for new card ──
        state["uid"]          = d.get("uid", "—")
        state["type"]         = d.get("type", "—")
        state["sectors"]      = d.get("sectors", 0)
        state["cracked"]      = 0           # ← Fix 5: reset on new card
        state["sectors_data"] = {}
        for i in range(state["sectors"]):
            state["sectors_data"][i] = {"keyA": None, "keyB": None, "blocks": {}}

    elif t == "key_found":
        s = d.get("sector", -1)
        if s not in state["sectors_data"]:
            state["sectors_data"][s] = {"keyA": None, "keyB": None, "blocks": {}}
        entry = state["sectors_data"][s]
        if d.get("key_type") == "A":
            entry["keyA"] = d.get("key", "????????????")
        else:
            entry["keyB"] = d.get("key", "????????????")
        # Recount cracked sectors
        state["cracked"] = sum(
            1 for s in state["sectors_data"].values()
            if s.get("keyA") or s.get("keyB")
        )

    elif t == "key_failed":
        pass  # tile stays grey — no state change needed

    elif t == "data_read":
        s = d.get("sector", -1)
        b = d.get("block", -1)
        if s not in state["sectors_data"]:
            state["sectors_data"][s] = {"keyA": None, "keyB": None, "blocks": {}}
        state["sectors_data"][s]["blocks"][b] = d.get("hex", "")

    # ── Log entry ──
    msg = _fmt_msg(t, d)
    with state["log_lock"]:
        state["log"].append((ts, t, msg))
        if len(state["log"]) > 200:
            state["log"].pop(0)

def _fmt_msg(t, d):
    if t == "key_found":     return f"S{d.get('sector','?')} Key{d.get('key_type','?')}: {d.get('key','?')} ({d.get('method','?')})"
    if t == "key_failed":    return f"S{d.get('sector','?')} Key{d.get('key_type','?')} not found"
    if t == "card_detected": return f"UID={d.get('uid','?')} {d.get('type','?')} ({d.get('sectors','?')} sectors)"
    if t == "attack_start":  return f"{d.get('attack','?')} → S{d.get('sector','?')} Key{d.get('key_type','?')}"
    if t == "attack_summary":return f"Done: {d.get('cracked','?')}/{d.get('total_sectors','?')} in {d.get('elapsed_ms',0)/1000:.1f}s"
    if t == "data_read":     return f"S{d.get('sector','?')} B{d.get('block','?')}: {d.get('hex','?')}"
    if t == "error":         return f"[{d.get('code','?')}] {d.get('msg','?')}"
    if t == "dump_saved":    return f"Saved → {d.get('path','?')}"
    return str(d)[:80]

# ─── UI builder ───────────────────────────────────────────────────────────────
def build_ui():
    console = Console()
    pct     = int(state["cracked"] / max(state["sectors"], 1) * 30)
    bar     = "[green]" + "█" * pct + "[/][dim]" + "░" * (30 - pct) + "[/]"

    # Card panel
    card = Panel(
        f"  UID : [bold cyan]{state['uid']}[/]\n"
        f"  Type: [bold]{state['type']}[/]\n"
        f"  Cracked: [bold green]{state['cracked']}/{state['sectors']}[/]\n"
        f"  {bar}",
        title="[dim]CARD[/]", expand=True
    )

    # Sector table
    tbl = Table(show_header=True, header_style="dim", expand=True, min_width=72)
    tbl.add_column("SEC", width=4, style="dim")
    tbl.add_column("KEY A",  width=14)
    tbl.add_column("KEY B",  width=14)
    tbl.add_column("BLOCKS", width=8, style="dim")
    tbl.add_column("STATUS", width=12)

    for idx in range(state["sectors"]):
        s   = state["sectors_data"].get(idx, {})
        kA  = s.get("keyA")
        kB  = s.get("keyB")
        blk = len(s.get("blocks", {}))
        kAs = f"[green]{kA}[/]" if kA else "[dim]????????????[/]"
        kBs = f"[green]{kB}[/]" if kB else "[dim]????????????[/]"
        if kA and kB:
            st = "[bold green]CRACKED[/]"
        elif kA or kB:
            st = "[bold yellow]PARTIAL[/]"
        else:
            st = "[dim]UNKNOWN[/]"
        tbl.add_row(f"S{idx:02d}", kAs, kBs, f"{blk}/4", st)

    sec_panel = Panel(tbl, title="[dim]SECTORS[/]")

    # Log panel (last 14 lines)
    log_txt = Text()
    with state["log_lock"]:
        entries = state["log"][-14:]
    for ts, ev_t, msg in entries:
        style = EV_STYLE.get(ev_t, "dim")
        log_txt.append(f"{ts} ", style="dim")
        log_txt.append(f"[{ev_t}]", style=style)
        log_txt.append(f" {msg}\n", style="white")

    log_panel = Panel(log_txt, title="[dim]LIVE LOG[/]")

    from rich.console import Group
    return Group(card, sec_panel, log_panel)

# ─── Serial reader thread ──────────────────────────────────────────────────────
def reader():
    while True:
        try:
            with serial.Serial(args.port, args.baud, timeout=1) as ser:
                print(f"[*] Connected → {args.port} @ {args.baud}")
                with LOG_FILE.open("a") as lf:
                    while True:
                        raw = ser.readline().decode("utf-8", errors="replace").strip()
                        if not raw: continue
                        lf.write(raw + "\n"); lf.flush()
                        try:
                            handle(json.loads(raw))
                        except json.JSONDecodeError:
                            pass   # skip non-JSON debug lines
        except serial.SerialException as e:
            print(f"[!] {e} — retry in 3s")
            time.sleep(3)

# ─── Optional HTTP dump download ──────────────────────────────────────────────
def _auto_download():
    if not args.ip: return
    # Watch for dump_saved event
    last_len = 0
    while True:
        time.sleep(1)
        with state["log_lock"]:
            entries = state["log"][last_len:]
            last_len = len(state["log"])
        for _, ev_t, _ in entries:
            if ev_t == "dump_saved":
                try:
                    import requests
                    r = requests.get(f"http://{args.ip}/api/dump", timeout=10)
                    if r.status_code == 200:
                        fname = f"dump_{int(time.time())}.mfd"
                        Path(fname).write_bytes(r.content)
                        print(f"[+] Dump downloaded → {fname}")
                except Exception as e:
                    print(f"[!] Dump download failed: {e}")

# ─── Entry point ──────────────────────────────────────────────────────────────
if __name__ == "__main__":
    from rich.console import Console
    Console().print(f"[bold cyan]⚡ MifareBreaker Monitor[/]  log: [dim]{LOG_FILE}[/]")
    if args.ip:
        Console().print(f"[dim]Dashboard: http://{args.ip}/[/]")

    threading.Thread(target=reader, daemon=True).start()
    threading.Thread(target=_auto_download, daemon=True).start()

    try:
        with Live(build_ui(), refresh_per_second=3) as live:
            while True:
                live.update(build_ui())
                time.sleep(0.33)
    except KeyboardInterrupt:
        print(f"\n[dim]Session saved → {LOG_FILE}[/]")
