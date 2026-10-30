# Author: jayis1. SPDX-License-Identifier: MIT
"""Bounded MR1 decoder; CRC is corruption detection, not authentication."""
import binascii
from collections import deque
import os
import re
import stat

FIELDS = ('now state mask known pending admitted left right abandoned rejected '
          'alternations longest streak latency_sum latency_max segments ambiguous '
          'orphans full timeouts gaps tx fault').split()
MONOTONIC = FIELDS[5:12] + ['latency_sum', 'latency_max'] + FIELDS[15:22]
MAXIMUM = 1_000_000_000
MAX_FILE = 2 * 1024 * 1024

def validate(r):
    done = r['left'] + r['right']
    if r['state'] > 2 or r['mask'] > 15 or r['known'] > 15 or r['pending'] > 8:
        raise ValueError('Invalid state or contact range')
    if r['fault'] > 15 or bool(r['fault']) != (r['state'] == 2):
        raise ValueError('Fault/state disagreement')
    if r['state'] != 1 and (r['pending'] or r['streak']):
        raise ValueError('Unarmed queue or streak')
    if r['state'] == 1 and (r['known'] != 15 or not r['segments']):
        raise ValueError('Armed without qualified baseline')
    if r['admitted'] != done + r['abandoned'] + r['pending']:
        raise ValueError('Admission conservation failed')
    if not 0 <= r['streak'] <= r['longest'] <= done:
        raise ValueError('Impossible source streak')
    if done and (not r['longest'] or r['alternations'] >= done):
        raise ValueError('Impossible alternation count')
    if not done and (r['alternations'] or r['latency_sum'] or r['latency_max']):
        raise ValueError('Latency without delivery')
    if not 0 <= r['latency_max'] < 5000:
        raise ValueError('Transit outside policy')
    if not max(done, r['latency_max']) <= r['latency_sum'] <= done*r['latency_max']:
        raise ValueError('Impossible latency total')
    if any(r[k] > MAXIMUM for k in FIELDS[5:22]):
        raise ValueError('Exhausted counter')

def decode(line):
    if len(line) > 512 or not re.fullmatch(rb'MR1(?:,[0-9]{1,10}){23}\*[0-9A-F]{4}', line):
        raise ValueError('Invalid MR1 framing or version')
    body, checksum = line.split(b'*')
    if binascii.crc_hqx(body, 65535) != int(checksum, 16):
        raise ValueError('CRC mismatch')
    values = [int(x) for x in body.split(b',')[1:]]
    if values[0] > 0xffffffff:
        raise ValueError('Invalid tick')
    result = dict(zip(FIELDS, values))
    validate(result)
    return result

class Stream:
    def __init__(self):
        self.buffer = b''
        self.errors = 0
    def feed(self, data):
        rows = []
        # Bound accumulation even when presented a very large external chunk.
        for start in range(0, len(data), 512):
            self.buffer += data[start:start+512]
            while self.buffer:
                marker = self.buffer.find(b'MR1,')
                if marker < 0:
                    if len(self.buffer) > 3:
                        self.errors += 1
                        self.buffer = self.buffer[-3:]
                    break
                if marker:
                    self.errors += 1
                    self.buffer = self.buffer[marker:]
                end = self.buffer.find(b'\n')
                next_marker = self.buffer.find(b'MR1,', 4)
                if next_marker >= 0 and (end < 0 or next_marker < end):
                    self.errors += 1
                    self.buffer = self.buffer[next_marker:]
                    continue
                if end < 0:
                    if len(self.buffer) > 512:
                        self.errors += 1
                        self.buffer = self.buffer[-3:]
                    break
                line, self.buffer = self.buffer[:end], self.buffer[end+1:]
                try:
                    rows.append(decode(line))
                except ValueError:
                    self.errors += 1
        return rows

class Session:
    def __init__(self, provenance):
        self.provenance = provenance
        self.connected = provenance == 'live'
        self.rows = deque(maxlen=500)
        self.evicted = self.errors = 0
        self.warning = ''
        self.first_host = self.last_host = None
        self.elapsed = 0
        self.advances = 0
    def fresh(self, now):
        return (self.connected and self.provenance == 'live' and not self.warning
                and self.advances > 0 and self.last_host is not None
                and 0 <= now-self.last_host <= 1.0)
    def accept(self, r, host):
        if self.warning:
            raise ValueError(self.warning)
        try:
            self._accept(r, host)
        except ValueError as error:
            self.warning = str(error)
            raise
    def _accept(self, r, host):
        validate(r)
        if self.rows:
            p = self.rows[-1]
            dt = (r['now'] - p['now']) & 0xffffffff
            if dt == 0:
                if r != p:
                    raise ValueError('Contradictory duplicate; reconnect required')
                return
            if dt >= 0x80000000 or any(r[k] < p[k] for k in MONOTONIC):
                raise ValueError('Possible reset or reordered evidence; reconnect required')
            if p['fault'] & ~r['fault']:
                raise ValueError('Sticky fault disappeared')
            if p['state'] == 2 and any(r[k] != p[k] for k in FIELDS if k not in ('now','tx','fault')):
                raise ValueError('Terminal accounting changed')
            done = r['left']+r['right']-p['left']-p['right']
            total = r['latency_sum']-p['latency_sum']
            if r['state'] == 1 and p['state'] != 1 and r['segments'] == p['segments']:
                raise ValueError('Arming without a new baseline segment')
            if any(r[k]-p[k] > done for k in ('alternations','longest','streak')):
                raise ValueError('Interleaving changed without matching deliveries')
            if not done <= total <= done*4999:
                raise ValueError('Latency changed without matching deliveries')
            if not done and r['latency_max'] != p['latency_max']:
                raise ValueError('Maximum changed without delivery')
            if done > dt or r['admitted']-p['admitted'] > dt:
                raise ValueError('Too many edges for elapsed samples')
            if r['known'] & p['known'] != p['known'] and r['gaps'] == p['gaps']:
                raise ValueError('Qualification lost without a gap')
            elapsed = self.elapsed + dt
            if self.provenance == 'live':
                wall = host-self.first_host
                allowance = 2.0 + min(wall*0.01, 1.0)
                if host < self.last_host or abs(wall-elapsed/1000) > allowance:
                    raise ValueError('Clock jump or serial backlog; reconnect required')
            self.elapsed = elapsed
            self.advances += 1
        else:
            self.first_host = host
        self.last_host = host
        if len(self.rows) == self.rows.maxlen:
            self.evicted += 1
        self.rows.append(r.copy())

def replay(path):
    fd = os.open(path, os.O_RDONLY | os.O_NONBLOCK)
    try:
        info = os.fstat(fd)
        if not stat.S_ISREG(info.st_mode) or info.st_size > MAX_FILE:
            raise ValueError('Replay requires a regular file up to 2 MiB')
        data = bytearray()
        while len(data) <= MAX_FILE:
            chunk = os.read(fd, min(8192, MAX_FILE+1-len(data)))
            if not chunk:
                break
            data.extend(chunk)
        if len(data) > MAX_FILE:
            raise ValueError('Replay grew past 2 MiB')
    finally:
        os.close(fd)
    stream = Stream()
    rows = stream.feed(bytes(data))
    if stream.errors or stream.buffer or not rows:
        raise ValueError('Replay is empty, corrupt, or has a partial tail')
    session = Session('replay')
    for row in rows:
        session.accept(row, 0)
    return session
