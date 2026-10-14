#!/usr/bin/env python3
"""
Simple monitor for MifareBreaker Serial JSON events.
"""
import argparse, serial, json, time
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument('--port', required=True)
parser.add_argument('--baud', type=int, default=115200)
parser.add_argument('--ip')
args = parser.parse_args()

ser = serial.Serial(args.port, args.baud, timeout=1)
logf = Path('mifare_log_{}.jsonl'.format(int(time.time())))
print('Logging to', logf)
with logf.open('w') as f:
    while True:
        line = ser.readline().decode('utf-8', errors='ignore').strip()
        if not line: continue
        try:
            j = json.loads(line)
        except Exception as e:
            print('RAW:', line)
            continue
        f.write(json.dumps(j) + '\n')
        f.flush()
        ev = j.get('event','')
        if ev=='card_detected':
            print('[CARD]', j['data'].get('uid'))
        elif ev=='key_found':
            d=j['data']; print('[KEY]', d.get('sector'), d.get('key_type'), d.get('key'))
        else:
            print('[E]', ev, j.get('data'))
