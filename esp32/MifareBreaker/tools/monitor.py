#!/usr/bin/env python3
"""
Simple monitor for MifareBreaker Serial JSON events.
"""
import argparse, serial, json, time
from pathlib import Path
from rich import print
from rich.table import Table
from rich.live import Live
from rich.panel import Panel

parser = argparse.ArgumentParser()
parser.add_argument('--port', required=True)
parser.add_argument('--baud', type=int, default=115200)
parser.add_argument('--ip')
args = parser.parse_args()

ser = serial.Serial(args.port, args.baud, timeout=1)
logf = Path('mifare_log_{}.jsonl'.format(int(time.time())))
print('Logging to', logf)

def render_state(card_info, sectors, logs):
    t = Table.grid(expand=True)
    t.add_column(ratio=1)
    t.add_column(ratio=3)
    left = Panel('\n'.join([f"UID: {card_info.get('uid','-')}", f"Sectors: {card_info.get('sectors','-')}", f"Cracked: {card_info.get('cracked','-')}"]), title='Card')
    right = Panel('\n'.join(logs[-20:][::-1]), title='Log')
    t.add_row(left, right)
    return t

card_info = {}
sectors = {}
logs = []

with logf.open('w') as f, Live(render_state(card_info,sectors,logs), refresh_per_second=4) as live:
    while True:
        line = ser.readline().decode('utf-8', errors='ignore').strip()
        if not line: continue
        try:
            j = json.loads(line)
        except Exception as e:
            print('[RAW]', line)
            continue
        f.write(json.dumps(j) + '\n')
        f.flush()
        ev = j.get('event','')
        logs.append(f"[{time.strftime('%H:%M:%S')}] {ev} {j.get('data','')}")
        if ev=='card_detected':
            card_info['uid'] = j['data'].get('uid')
            card_info['sectors'] = j['data'].get('sectors')
        if ev=='key_found':
            d=j['data']; idx = d.get('sector')
            card_info['cracked'] = card_info.get('cracked',0)+1
            sectors[idx] = d
        live.update(render_state(card_info,sectors,logs))
