#!/usr/bin/env python3
"""Summarize Mejiro31 Linux key transitions without grabbing keys or saving text."""
import argparse
from collections import Counter
import json
import os
from pathlib import Path
import select
import struct
import time

EVENT = struct.Struct('@llHHi')


def keyboard_devices():
    result = {}
    for block in Path('/proc/bus/input/devices').read_text().split('\n\n'):
        fields = dict(line.split(': ', 1) for line in block.splitlines() if ': ' in line)
        if 'Vendor=2784 Product=0001' not in fields.get('I', ''):
            continue
        # Both boot keyboard and NKRO keyboard; exclude consumer/system controls.
        name = fields.get('N', '')
        if name not in ['Name="JEEBIS Mejiro31"', 'Name="JEEBIS Mejiro31 Keyboard"']:
            continue
        for handler in fields.get('H', '').split():
            if handler.startswith('event'):
                result[Path('/dev/input') / handler] = name
    return result


def track(state, timestamp, code, value, threshold):
    state['counts'][{0: 'release', 1: 'press', 2: 'os_repeat'}[value]] += 1
    previous = state['last'].get(code)
    if value == 1 and previous and previous[0] == 0:
        gap = timestamp - previous[1]
        if 0 <= gap <= threshold:
            state['rapid_repress'].append({'linux_keycode': code,
                                          'release_to_press_ms': round(gap, 3)})
    state['last'][code] = (value, timestamp)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--seconds', type=float, default=30)
    parser.add_argument('--threshold-ms', type=float, default=30)
    args = parser.parse_args()
    if args.seconds <= 0 or args.threshold_ms < 0:
        parser.error('seconds must be positive and threshold-ms nonnegative')
    devices = keyboard_devices()
    if not devices:
        parser.exit(1, 'Mejiro31 keyboard not found.\n')
    opened = {}
    try:
        for path, name in devices.items():
            try:
                fd = os.open(path, os.O_RDONLY | os.O_NONBLOCK)
            except PermissionError:
                parser.exit(1, f'Cannot read {path}; run this script with sudo.\n')
            opened[fd] = {'device': str(path), 'name': name, 'counts': Counter(),
                          'last': {}, 'rapid_repress': [], 'buffer': b''}
        print(f'Observing only Mejiro31 for {args.seconds:g}s. Keys remain usable; no text is saved.', flush=True)
        deadline = time.monotonic() + args.seconds
        while time.monotonic() < deadline:
            readable, _, _ = select.select(list(opened), [], [], max(0, deadline - time.monotonic()))
            for fd in readable:
                data = os.read(fd, EVENT.size * 128)
                if not data:
                    raise RuntimeError('Mejiro31 disconnected during capture')
                state = opened[fd]
                state['buffer'] += data
                while len(state['buffer']) >= EVENT.size:
                    sec, usec, kind, code, value = EVENT.unpack_from(state['buffer'])
                    state['buffer'] = state['buffer'][EVENT.size:]
                    if kind == 1 and value in (0, 1, 2):
                        track(state, sec * 1000 + usec / 1000, code, value, args.threshold_ms)
        print(json.dumps([{key: value for key, value in state.items() if key not in ('last', 'buffer')}
                          for state in opened.values()], indent=2))
    except KeyboardInterrupt:
        print('Capture interrupted.')
    finally:
        for fd in opened:
            os.close(fd)


if __name__ == '__main__':
    main()
