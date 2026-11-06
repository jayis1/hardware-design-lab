# Author: jayis1. SPDX-License-Identifier: MIT
"""PC1 validation and evidence sessions. Endpoint bounds cannot prove hidden history."""
from collections import deque
import csv
import os
import time
FIELDS = 'sequence now state fault raw qualified valid starts completed aborted age baseline pulses a b neither both uncertain aa ab ba bb anchor tx_errors gaps'.split()
STATES = ('WAIT', 'READY', 'ACTIVE', 'HALT')
RUN = 'age pulses a b neither both uncertain aa ab ba bb anchor'.split()
BINS = 'a b neither both uncertain'.split()
PAIRS = 'aa ab ba bb'.split()
COUNTERS = 'starts completed aborted tx_errors gaps'.split()

def require(condition, message):
    if not condition:
        raise ValueError(message)

def validate(r):
    require(set(r) == set(FIELDS), 'field contract')
    require(all(type(v) is int and 0 <= v <= 0xffffffff for v in r.values()), 'integer range')
    require(r['state'] <= 3 and r['fault'] in (0, 1, 2, 4, 8, 16, 32), 'state/fault range')
    require((r['state'] == 3) == bool(r['fault']), 'fault/state mismatch')
    require(all(r[x] <= 15 for x in ('raw', 'qualified', 'valid')), 'contact range')
    require(r['age'] <= 60000 and r['baseline'] <= 50 and r['anchor'] <= 2, 'run range')
    require(all(r[x] <= 1000000 for x in COUNTERS + ['pulses']), 'counter range')
    require(r['starts'] == r['completed'] + r['aborted'] + (r['state'] == 2), 'trial conservation')
    require(r['gaps'] <= 1 and (not r['gaps'] or r['fault'] == 4), 'gap without halt')
    require(r['pulses'] == sum(r[x] for x in BINS), 'arrival conservation')
    pairs = sum(r[x] for x in PAIRS)
    require(pairs <= r['a'] + r['b'] and (not r['pulses'] or pairs < r['pulses']), 'pair conservation')
    require(r['aa'] + r['ba'] <= r['a'] and r['ab'] + r['bb'] <= r['b'], 'pair destination count')
    require(r['aa'] + r['ab'] + (r['anchor'] == 1) <= r['a'] and
            r['ba'] + r['bb'] + (r['anchor'] == 2) <= r['b'], 'pair source budget')
    require(r['anchor'] != 1 or r['a'] > 0, 'missing A anchor')
    require(r['anchor'] != 2 or r['b'] > 0, 'missing B anchor')
    require(r['starts'] > 0 or not any(r[x] for x in RUN), 'run without start')
    if r['state'] in (0, 1) and r['starts']:
        require(r['age'] >= 5, 'completed before qualified RUN release')
    require(r['pulses'] <= (r['age'] + 5) // 10, 'arrival sample budget')
    if r['state'] == 1:
        require(r['valid'] == 15 and not r['qualified'] & 12 and r['baseline'] == 50, 'unarmed READY')
    if r['state'] == 2:
        require(r['valid'] == 15 and r['qualified'] & 8 and r['age'] < 60000, 'invalid ACTIVE')
    require(r['fault'] != 8 or r['age'] == 60000, 'deadline age')

class Session:
    def __init__(self, provenance):
        require(provenance in ('live', 'replay'), 'provenance')
        self.provenance = provenance
        self.rows = deque(maxlen=512)
        self.connected = False
        self.warning = ''
        self.errors = 0
        self.last_progress = None
        self.anchor_host = None
        self.device_elapsed = 0
        self.progress = 0

    def accept(self, row, host=None):
        if self.warning:
            raise ValueError(self.warning)
        try:
            return self._accept(row, time.monotonic() if host is None else host)
        except ValueError as exc:
            self.warning = str(exc)
            raise

    def _accept(self, r, host):
        validate(r)
        if self.rows:
            p = self.rows[-1]
            if r == p:
                return False
            dt = (r['now'] - p['now']) & 0xffffffff
            ds = r['sequence'] - p['sequence']
            require(0 < dt < 0x80000000 and 0 <= ds <= dt, 'reset or contradictory duplicate')
            for name in COUNTERS:
                require(r[name] >= p[name], 'counter rollback: ' + name)
            if p['state'] == 3:
                require(all(r[x] == p[x] for x in FIELDS if x not in ('now', 'tx_errors')), 'halted evidence changed')
            else:
                require(ds > 0 or r['state'] == 3, 'clock without acquisition')
                if r['state'] != 3:
                    require(ds == dt, 'unreported acquisition gap')
                starts = r['starts'] - p['starts']
                outcomes = r['completed'] + r['aborted'] - p['completed'] - p['aborted']
                require(starts <= (ds + 4) // 5, 'too many starts')
                require(outcomes <= starts + (p['state'] == 2), 'outcome without run')
                if not starts:
                    if p['state'] != 2:
                        require(all(r[x] == p[x] for x in RUN), 'frozen run changed')
                    else:
                        da = r['age'] - p['age']
                        require(0 <= da <= ds, 'run age outside acquisition')
                        if r['state'] == 2:
                            require(da == ds, 'continuing run age')
                        for x in BINS + PAIRS + ['pulses']:
                            require(r[x] >= p[x], 'run evidence rollback')
                        added = r['pulses'] - p['pulses']
                        require(added <= (da + 9) // 10, 'new arrival sample budget')
                        require(sum(r[x] - p[x] for x in PAIRS) <= r['a'] + r['b'] - p['a'] - p['b'], 'new pair without attributed arrival')
                        if not added:
                            require(r['anchor'] == p['anchor'], 'anchor changed without arrival')
                else:
                    require(r['age'] < ds, 'new run backdated')
            self.device_elapsed += dt
            if self.provenance == 'live':
                require(abs((host - self.anchor_host) * 1000 - self.device_elapsed) <= 2000,
                        'host/device progress mismatch; reconnect')
            if ds:
                self.progress += 1
                self.last_progress = host
        else:
            self.anchor_host = host
            self.last_progress = host
        self.rows.append(dict(r))
        return True

    def fresh(self, host=None):
        host = time.monotonic() if host is None else host
        return bool(self.provenance == 'live' and self.connected and not self.warning and
                    self.progress and self.rows and self.rows[-1]['state'] != 3 and
                    self.last_progress is not None and 0 <= host - self.last_progress <= 1)

    def export(self, path):
        fd = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
        with os.fdopen(fd, 'w', newline='') as handle:
            writer = csv.writer(handle)
            writer.writerow(['author', 'provenance', 'connected', 'fresh', 'parser_errors', 'warning'])
            writer.writerow(['jayis1', self.provenance, self.connected, self.fresh(), self.errors, self.warning])
            writer.writerow(FIELDS)
            writer.writerows([r[x] for x in FIELDS] for r in self.rows)
