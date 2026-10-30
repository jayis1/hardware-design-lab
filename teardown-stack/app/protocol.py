# Author: jayis1. SPDX-License-Identifier: MIT
"""Receive-only TS1 protocol and bounded evidence session; Python stdlib only."""
import binascii
from collections import deque
import os
import re
import stat
import time
FIELDS = ('now state mask known depth stack0 stack1 stack2 stack3 started completed '
          'aborted pushes pops abandoned order_errors collisions gaps tx fault').split()
COUNTERS = FIELDS[9:19]
PATTERN = re.compile(rb'TS1(?:,[0-9A-F]{8}){20}\*[0-9A-F]{4}\n')

def encode(row):
    """Used for negative tests, not for simulated live hardware."""
    payload = ('TS1' + ''.join(f',{row[k]:08X}' for k in FIELDS)).encode()
    return payload + f'*{binascii.crc_hqx(payload, 0xffff):04X}\n'.encode()

def decode(line):
    if not PATTERN.fullmatch(line):
        raise ValueError('Malformed TS1 frame')
    payload, crc = line.rstrip(b'\n').split(b'*')
    if binascii.crc_hqx(payload, 0xffff) != int(crc, 16):
        raise ValueError('CRC mismatch')
    r: dict[str, int] = dict(zip(FIELDS, (int(x, 16) for x in payload.split(b',')[1:])))
    depth, state = r['depth'], r['state']
    if state > 3 or depth > 4 or r['mask'] > 15 or r['known'] > 15 or r['fault'] > 15:
        raise ValueError('Out of range')
    if (state == 2) != bool(depth) or (state == 3) != bool(r['fault']):
        raise ValueError('Contradictory state')
    if r['started'] != r['completed'] + r['aborted'] + bool(depth):
        raise ValueError('Session conservation')
    if r['pushes'] != r['pops'] + r['abandoned'] + depth:
        raise ValueError('Cup conservation')
    if r['completed'] > r['pops'] or r['started'] > r['pushes'] or r['order_errors'] > r['aborted']:
        raise ValueError('Impossible accounting')
    stack = [r[f'stack{i}'] for i in range(4)]
    if any(x > 3 for x in stack[:depth]) or len(set(stack[:depth])) != depth or any(stack[depth:]):
        raise ValueError('Invalid stack')
    bits = sum(1 << x for x in stack[:depth])
    if state == 2 and (r['known'] != 15 or r['mask'] != bits):
        raise ValueError('Stack/contact disagreement')
    if state == 1 and (r['known'] != 15 or r['mask']):
        raise ValueError('Invalid empty baseline')
    return r

def read_replay(path):
    """Avoid blocking on FIFOs, devices or allocating an unbounded recording."""
    fd = os.open(path, os.O_RDONLY | os.O_NONBLOCK)
    try:
        info = os.fstat(fd)
        if not stat.S_ISREG(info.st_mode) or info.st_size > 2_000_000:
            raise ValueError('Replay must be a regular file at most 2 MB')
        with os.fdopen(fd, 'rb', closefd=False) as stream:
            data = stream.read(2_000_001)
        if len(data) > 2_000_000:
            raise ValueError('Replay grew beyond limit')
        return data
    finally:
        os.close(fd)

class Session:
    def __init__(self, provenance):
        self.provenance = provenance
        self.connected = False
        self.rows = deque(maxlen=1000)
        self.buffer = b''
        self.errors = 0
        self.blocked = False
        self.warning = ''
        self.last_host = None
        self.anchor = None
        self.elapsed = 0.0
        self.progress = 0

    def block(self, message):
        self.blocked = True
        self.warning = message

    def fresh(self, now=None):
        now = time.monotonic() if now is None else now
        return (self.provenance == 'live' and self.connected and not self.blocked
                and self.progress >= 2 and self.last_host is not None
                and 0 <= now - self.last_host <= 0.75)

    def accept(self, r, host):
        if self.blocked:
            return
        if self.rows:
            previous = self.rows[-1]
            delta = (r['now'] - previous['now']) & 0xffffffff
            if not delta:
                if r != previous:
                    self.block('Contradictory duplicate; reconnect required')
                return
            if delta >= 0x80000000 or any(r[k] < previous[k] for k in COUNTERS):
                self.block('Possible reset or out-of-order evidence; reconnect required')
                return
            if previous['fault'] & ~r['fault']:
                self.block('Sticky fault disappeared')
                return
            if previous['state'] == 3 and any(r[k] != previous[k] for k in FIELDS[4:17]):
                self.block('Halted accounting changed')
                return
            if r['pushes'] - previous['pushes'] > delta or r['pops'] - previous['pops'] > delta:
                self.block('Movement count exceeds sample opportunities')
                return
            self.elapsed += delta / 1000.0
            if self.provenance == 'live':
                # Fixed allowance, not ever-growing drift credit. May require
                # reconnect on long sessions with inaccurate HSI oscillators.
                if host - self.last_host > 1.5 or abs((host - self.anchor) - self.elapsed) > 0.75:
                    self.block('Timing discontinuity/backlog; reconnect required')
                    return
        else:
            self.anchor = host
        self.rows.append(r)
        self.last_host = host
        self.progress += 1

    def feed(self, data, host=None):
        host = time.monotonic() if host is None else host
        # Process bounded chunks; a huge caller buffer never becomes parser state.
        for start in range(0, len(data), 256):
            self.buffer += data[start:start+256]
            while self.buffer:
                marker = self.buffer.find(b'TS1,')
                if marker < 0:
                    if len(self.buffer) > 3:
                        self.errors += 1
                        self.buffer = self.buffer[-3:]
                    break
                if marker:
                    self.errors += 1
                    self.buffer = self.buffer[marker:]
                later = self.buffer.find(b'TS1,', 4)
                newline = self.buffer.find(b'\n')
                if later >= 0 and (newline < 0 or later < newline):
                    self.errors += 1
                    self.buffer = self.buffer[later:]
                    continue
                if newline < 0:
                    if len(self.buffer) > 200:
                        self.errors += 1
                        self.buffer = self.buffer[-3:]
                    break
                line, self.buffer = self.buffer[:newline+1], self.buffer[newline+1:]
                try:
                    row = decode(line)
                except ValueError:
                    self.errors += 1
                else:
                    self.accept(row, host)

    def finish(self):
        if self.buffer or not self.rows or self.errors or self.blocked:
            raise ValueError('Replay incomplete, corrupt, empty or inconsistent')
