# Author: jayis1. SPDX-License-Identifier: MIT
"""Bounded RA1 framing, invariant validation and provenance-preserving records."""
import binascii
import json
import os
import re
import stat
from collections import deque
FIELDS = ('now phase mask starts completed aborted duration forward reverse '
          'seated_reverse rollback faults gaps tx_errors').split()
LIMIT = 2 * 1024 * 1024


def decode(line):
    if len(line) > 256:
        raise ValueError('oversize record')
    match = re.fullmatch(rb'(RA1(?:,[0-9]{1,10}){14})\*([0-9A-F]{4})\n', line)
    if not match or binascii.crc_hqx(match[1], 65535) != int(match[2], 16):
        raise ValueError('unsupported frame or CRC')
    v = dict(zip(FIELDS, map(int, match[1].split(b',')[1:])))
    if any(x > 0xffffffff for x in v.values()):
        raise ValueError('integer range')
    p = v['phase']
    if p > 3 or v['mask'] > 15 or v['faults'] & ~127:
        raise ValueError('enum range')
    if v['starts'] != v['completed'] + v['aborted'] + (p == 2):
        raise ValueError('trial conservation')
    if (v['duration'] > 30000 or (p == 2 and v['duration'] == 30000)
            or v['forward'] + v['reverse'] > 1000000):
        raise ValueError('trial bounds')
    if v['seated_reverse'] > v['reverse'] or v['rollback'] > v['reverse']:
        raise ValueError('reverse bounds')
    if v['reverse'] and not v['rollback']:
        raise ValueError('missing drawdown')
    if v['rollback'] < v['reverse'] - v['forward']:
        raise ValueError('drawdown below net reverse travel')
    if not v['starts'] and (v['duration'] or v['forward'] or v['reverse']):
        raise ValueError('orphan trial evidence')
    if (p == 1 and v['mask'] & 12 != 12) or (p == 2 and v['mask'] & 4):
        raise ValueError('phase/contact mismatch')
    if p == 3 and not v['faults'] & 35:
        raise ValueError('halt without cause')
    if (v['gaps'] and not v['faults'] & 16) or (v['tx_errors'] and not v['faults'] & 64):
        raise ValueError('missing fault flag')
    return v


class Stream:
    def __init__(self):
        self.pending = b''
        self.errors = 0

    def feed(self, data):
        # Process bytes, retaining at most one bounded frame; a fresh full
        # marker recovers after a truncated UART transmission without LF.
        rows = []
        for byte in data:
            self.pending += bytes([byte])
            if self.pending.endswith(b'RA1,') and len(self.pending) > 4:
                self.errors += 1
                self.pending = b'RA1,'
            if byte == 10:
                try:
                    rows.append(decode(self.pending))
                except ValueError:
                    self.errors += 1
                self.pending = b''
            elif len(self.pending) > 256:
                self.errors += 1
                self.pending = self.pending[-3:]
        return rows

    def finish(self):
        if self.pending:
            raise ValueError('truncated recording tail')


class Session:
    def __init__(self, source):
        self.source = source
        self.rows = deque(maxlen=2000)
        self.dropped = 0
        self.parser_errors = 0
        self.blocked = ''
        self.last_host = None
        self.first_host = None
        self.device_elapsed = 0
        self.progress = 0

    def reject(self, message):
        self.blocked = message
        raise ValueError(message)

    def accept(self, row, host):
        if self.blocked:
            raise ValueError(self.blocked)
        if self.rows:
            old = self.rows[-1]
            delta = (row['now'] - old['now']) & 0xffffffff
            if not delta:
                if row != old:
                    self.reject('contradictory duplicate; reconnect and reset fixture')
                return
            if delta >= 0x80000000:
                self.reject('device time regressed; suspected reset')
            for key in ('starts', 'completed', 'aborted', 'gaps', 'tx_errors'):
                if row[key] < old[key]:
                    self.reject('cumulative counter regressed; suspected reset')
            if old['faults'] & ~row['faults']:
                self.reject('sticky evidence disappeared')
            if row['starts'] == old['starts']:
                for key in ('duration', 'forward', 'reverse', 'seated_reverse', 'rollback'):
                    if row[key] < old[key]:
                        self.reject('trial evidence regressed')
                    if old['phase'] != 2 and row[key] != old[key]:
                        self.reject('retained trial evidence changed without a new start')
            if old['phase'] == 3:
                for key in FIELDS:
                    if key not in ('now', 'faults', 'tx_errors') and row[key] != old[key]:
                        self.reject('halted evidence changed')
            self.device_elapsed += delta
            if self.source == 'live':
                if host - self.last_host > 2:
                    self.reject('host reception pause; reconnect before relying on data')
                host_ms = (host - self.first_host) * 1000
                if abs(self.device_elapsed - host_ms) > 2000 + .05 * host_ms:
                    self.reject('serial backlog or clock discontinuity')
        else:
            self.first_host = host
        if len(self.rows) == self.rows.maxlen:
            self.dropped += 1
        self.rows.append(row)
        self.last_host = host
        self.progress += 1

    def fresh(self, host, connected):
        return bool(connected and self.source == 'live' and not self.blocked and
                    self.progress >= 2 and self.last_host is not None and
                    host - self.last_host < 1)

    def export(self, path, connected, host):
        payload = dict(author='jayis1', source=self.source,
                       connected=connected, fresh=self.fresh(host, connected),
                       age_seconds=None if self.last_host is None else host-self.last_host,
                       parser_errors=self.parser_errors, blocked=self.blocked,
                       dropped_snapshots=self.dropped, units='qualified encoder edges',
                       warning='Diagnostic evidence, not safe-lock proof; snapshots may miss trials',
                       snapshots=list(self.rows))
        # Exclusive creation avoids accidentally destroying an earlier experiment.
        fd = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
        with os.fdopen(fd, 'w') as output:
            json.dump(payload, output, indent=2)
            output.write('\n')


def read_replay(path):
    fd = os.open(path, os.O_RDONLY | os.O_NONBLOCK)
    try:
        info = os.fstat(fd)
        if not stat.S_ISREG(info.st_mode) or info.st_size > LIMIT:
            raise ValueError('replay must be a regular file of at most 2 MiB')
        with os.fdopen(fd, 'rb', closefd=False) as source:
            data = source.read(LIMIT + 1)
        if len(data) > LIMIT:
            raise ValueError('recording grew beyond limit')
    finally:
        os.close(fd)
    stream = Stream()
    session = Session('replay')
    for row in stream.feed(data):
        session.accept(row, 0)
    stream.finish()
    session.parser_errors = stream.errors
    if stream.errors or not session.rows:
        raise ValueError('recording contains invalid frames or no snapshots')
    return session
