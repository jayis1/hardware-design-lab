# Author: jayis1. SPDX-License-Identifier: MIT
"""KE1 evidence validation. Sparse snapshots are bounds, not a full trace proof."""
from collections import deque
import csv
import os
import time

FIELDS = 'sequence now state result fault raw qualified valid opened starts completed canceled aborted age last_ms baseline tx_errors gaps'.split()
STATES = ('WAIT', 'READY', 'ACTIVE', 'HALT')
RESULTS = ('none', 'access cycle', 'returned without opening', 'early key removal',
           'opening without prior retraction', 'contradictory endpoints',
           'unkeyed movement', 'extension while open', 'deadline',
           'acquisition gap', 'counter exhausted', 'hardware fault')
COUNTERS = ('starts', 'completed', 'canceled', 'aborted', 'tx_errors', 'gaps')

def require(condition, message):
    if not condition:
        raise ValueError(message)

def validate(r):
    require(set(r) == set(FIELDS), 'field contract')
    require(all(type(v) is int and 0 <= v <= 0xffffffff for v in r.values()), 'integer range')
    require(r['state'] <= 3 and r['result'] < len(RESULTS) and r['fault'] <= 31, 'enum range')
    require(all(r[x] <= 15 for x in ('raw', 'qualified', 'valid')), 'contact range')
    require(r['opened'] <= 1, 'opened flag')
    require(all(r[x] <= 1000000 for x in COUNTERS), 'counter range')
    require(r['starts'] == r['completed'] + r['canceled'] + r['aborted'] + (r['state'] == 2), 'trial conservation')
    require(r['baseline'] <= 50 and r['age'] <= 60000 and r['last_ms'] <= 60000 and r['gaps'] <= 1, 'time range')
    require((r['state'] == 3) == bool(r['fault']), 'fault/state mismatch')
    require((r['result'] >= 3) == (r['state'] == 3), 'result/state mismatch')
    require(not r['gaps'] or (r['fault'] & 16 and r['result'] == 9), 'gap without halt')
    q = r['qualified']
    if r['state'] == 1:
        require(q == 10 and r['valid'] == 15 and r['baseline'] == 50, 'unarmed READY')
    if r['state'] == 2:
        require(r['valid'] == 15 and q & 4 and q & 3 != 3 and r['age'] < 60000, 'invalid ACTIVE')
        if not q & 8:
            require(q & 7 == 5 and r['opened'], 'unproven opening')
    require(r['result'] != 1 or r['completed'] > 0, 'missing cycle')
    require(r['result'] != 2 or r['canceled'] > 0, 'missing return')

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
                return False  # duplicate delivery cannot renew freshness
            dt = (r['now'] - p['now']) & 0xffffffff
            ds = r['sequence'] - p['sequence']
            require(0 < dt < 0x80000000 and ds >= 0, 'reset or contradictory duplicate')
            require(ds <= dt, 'acquisition exceeds clock')
            for name in COUNTERS:
                require(r[name] >= p[name], 'counter rollback: ' + name)
            if p['state'] == 3:
                require(all(r[x] == p[x] for x in FIELDS if x not in ('now', 'tx_errors')), 'halted evidence changed')
            else:
                require(ds > 0 or r['state'] == 3, 'clock without acquisition')
                if r['state'] != 3:
                    require(ds == dt, 'unreported acquisition gap')
                starts = r['starts'] - p['starts']
                outcomes = sum(r[x] - p[x] for x in ('completed', 'canceled', 'aborted'))
                require(starts <= (ds + 4) // 5, 'too many insertions')
                require(outcomes <= starts + (p['state'] == 2), 'outcome without trial')
                if r['state'] == 2:
                    if starts == 0:
                        require(p['state'] == 2 and r['age'] == p['age'] + ds, 'active age changed')
                        require(r['opened'] >= p['opened'], 'opening evidence disappeared')
                    else:
                        require(r['age'] < ds, 'new trial backdated')
                if outcomes == 0:
                    require(r['last_ms'] == p['last_ms'], 'retained duration changed')
                    if r['state'] != 3:
                        require(r['result'] == p['result'], 'retained result changed')
                else:
                    require(r['last_ms'] <= ds + (p['age'] if p['state'] == 2 else 0), 'outcome backdated')
                    if r['state'] != 3:
                        require((r['result'] == 1 and r['completed'] > p['completed']) or
                                (r['result'] == 2 and r['canceled'] > p['canceled']), 'latest outcome type')
            self.device_elapsed += dt
            if self.provenance == 'live':
                require(abs((host - self.anchor_host) * 1000 - self.device_elapsed) <= 2000,
                        'host/device progress mismatch; reconnect')
            if ds > 0:
                self.last_progress = host
                self.progress += 1
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
        # Never overwrite evidence, follow existing symlinks, or expose files
        # under a permissive umask. The metadata row survives empty sessions.
        fd = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
        with os.fdopen(fd, 'w', newline='') as handle:
            writer = csv.writer(handle)
            writer.writerow(['author', 'provenance', 'connected', 'fresh', 'parser_errors', 'warning'])
            writer.writerow(['jayis1', self.provenance, self.connected, self.fresh(), self.errors, self.warning])
            writer.writerow(FIELDS)
            writer.writerows([r[x] for x in FIELDS] for r in self.rows)
