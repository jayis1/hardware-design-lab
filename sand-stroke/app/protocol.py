# Author: jayis1. SPDX-License-Identifier: MIT
"""Strict SS1 decoder, bounded framing, session integrity and replay."""
import binascii
from collections import deque
import os
import re
import stat
FIELDS = ('now state mask known anchor face elapsed started full_a full_b '
          'aborted ms_a ms_b short_returns lifts flips collisions timeouts gaps tx fault').split()
COUNTERS = FIELDS[7:20]
PATTERN = re.compile(rb'SS1(?:,[0-9A-F]{8}){21}\*[0-9A-F]{4}\n')
LIMIT = 2 * 1024 * 1024

def decode(line):
    if not PATTERN.fullmatch(line):
        raise ValueError('Invalid SS1 frame shape or version')
    body, checksum = line.rstrip(b'\n').split(b'*')
    if binascii.crc_hqx(body, 65535) != int(checksum, 16):
        raise ValueError('CRC mismatch')
    row = dict(zip(FIELDS, (int(x, 16) for x in body.split(b',')[1:])))
    s, mask = row['state'], row['mask']
    if s > 3 or mask > 15 or row['known'] > 15 or row['face'] > 1 or row['fault'] > 15:
        raise ValueError('Field range')
    if (s == 3) != bool(row['fault']):
        raise ValueError('Fault/state disagreement')
    if row['started'] != row['full_a'] + row['full_b'] + row['aborted'] + (s == 2):
        raise ValueError('Stroke conservation')
    for face in ('a', 'b'):
        count, duration = row['full_'+face], row['ms_'+face]
        if not count <= duration <= count * 9999:
            raise ValueError('Duration/count disagreement')
    if sum(row[k] for k in ('short_returns', 'lifts', 'flips', 'timeouts')) > row['aborted']:
        raise ValueError('Rejection accounting')
    if s in (0, 3):
        if row['anchor'] or row['face'] or row['elapsed']:
            raise ValueError('Uncorrelated state retains active fields')
    else:
        if row['known'] != 15 or not mask & 4 or row['anchor'] not in (1, 2):
            raise ValueError('Missing baseline')
        if row['face'] != bool(mask & 8):
            raise ValueError('Face contradiction')
        if s == 1 and ((mask & 3) != row['anchor'] or row['elapsed']):
            raise ValueError('Ready endpoint contradiction')
        if s == 2 and ((mask & 3) or row['elapsed'] >= 10000):
            raise ValueError('Active traversal contradiction')
    return row

class Stream:
    def __init__(self):
        self.buffer = b''
        self.errors = 0

    def feed(self, chunk):
        self.buffer += chunk
        rows = []
        while self.buffer:
            start = self.buffer.find(b'SS1,')
            if start < 0:
                # Preserve a possible split marker, never an unlimited tail.
                if len(self.buffer) > 3:
                    self.errors += 1
                    self.buffer = self.buffer[-3:]
                break
            if start:
                self.errors += 1
                self.buffer = self.buffer[start:]
            newline = self.buffer.find(b'\n')
            later = self.buffer.find(b'SS1,', 4)
            if later >= 0 and (newline < 0 or later < newline):
                self.errors += 1
                self.buffer = self.buffer[later:]
                continue
            if newline < 0:
                if len(self.buffer) > 198:
                    self.errors += 1
                    self.buffer = self.buffer[-3:]
                break
            frame, self.buffer = self.buffer[:newline+1], self.buffer[newline+1:]
            try:
                rows.append(decode(frame))
            except ValueError:
                self.errors += 1
        return rows

class Session:
    def __init__(self, provenance):
        self.provenance = provenance
        self.rows = deque(maxlen=1000)
        self.warning = ''
        self.errors = 0
        self.evicted = 0
        self.progress = None
        self.origin = None
        self.elapsed = 0
        self.advances = 0
        self.connected = provenance == 'live'

    def fail(self, reason):
        self.warning = reason
        raise ValueError(reason)

    def accept(self, row, host):
        if self.warning:
            raise ValueError(self.warning)
        if self.rows:
            old = self.rows[-1]
            delta = (row['now'] - old['now']) & 0xffffffff
            if not delta:
                if row != old:
                    self.fail('Contradictory duplicate; reconnect')
                return False
            if delta >= 0x80000000:
                self.fail('Clock rollback; reconnect')
            if any(row[k] < old[k] for k in COUNTERS):
                self.fail('Counter rollback; reconnect')
            if old['fault'] & ~row['fault']:
                self.fail('Fault disappeared; reconnect')
            if old['state'] == 3 and any(row[k] != old[k] for k in FIELDS[1:] if k not in ('tx','fault')):
                self.fail('Halted evidence changed')
            duration_growth = 0
            for face in ('a', 'b'):
                count = row['full_'+face] - old['full_'+face]
                duration = row['ms_'+face] - old['ms_'+face]
                if not count <= duration <= count * 9999:
                    self.fail('Duration changed without matching completions')
                duration_growth += duration
            if duration_growth > delta + old['elapsed']:
                self.fail('Duration grew beyond available time')
            self.elapsed += delta
            if self.provenance == 'live':
                if host - self.progress > 1.5 or abs(self.elapsed / 1000 - (host-self.origin)) > .75:
                    self.fail('Host/device progress mismatch; reconnect')
            self.advances += 1
        else:
            self.origin = host
        if len(self.rows) == self.rows.maxlen:
            self.evicted += 1
        self.rows.append(dict(row))
        self.progress = host
        return True

    def fresh(self, host):
        return (self.connected and self.provenance == 'live' and not self.warning
                and self.advances > 0 and self.progress is not None
                and 0 <= host-self.progress <= .75)

def replay(path):
    fd = os.open(path, os.O_RDONLY | os.O_NONBLOCK)
    try:
        info = os.fstat(fd)
        if not stat.S_ISREG(info.st_mode) or info.st_size > LIMIT:
            raise ValueError('Replay requires a regular file up to two MiB')
        with os.fdopen(fd, 'rb', closefd=False) as file:
            data = file.read(LIMIT+1)
    finally:
        os.close(fd)
    if not data or len(data) > LIMIT or not data.endswith(b'\n'):
        raise ValueError('Empty, oversized or truncated recording')
    session = Session('replay')
    for line in data.splitlines(keepends=True):
        session.accept(decode(line), 0)
    return session
