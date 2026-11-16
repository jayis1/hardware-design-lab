# Author: jayis1. SPDX-License-Identifier: MIT
"""CD1 evidence validation and bounded local session storage; no dependencies."""
from collections import deque
import csv
import json
import os
import time

FIELDS = ('sequence now phase fault raw qualified valid starts completed aborted '
          'reference touched absent opened age quiet last_before last_after '
          'last_touched last_returned last_age gaps tx_errors').split()
LAST = ('last_before last_after last_touched last_returned last_age').split()
COUNTERS = 'sequence starts completed aborted gaps tx_errors'.split()
LIMIT = 1000000000
DEADLINE = 600000
PHASES = ['Baseline', 'Sealed reference', 'Open session', 'HALTED']

def require(condition, message):
    if not condition:
        raise ValueError(message)

def validate(r):
    require(set(r) == set(FIELDS), 'incompatible fields')
    require(all(type(v) is int and 0 <= v <= 0xffffffff for v in r.values()), 'integer range')
    require(r['phase'] <= 3 and r['fault'] <= 31, 'phase/fault range')
    require((r['phase'] == 3) == bool(r['fault']), 'halt/fault disagreement')
    require(all(r[k] <= LIMIT for k in COUNTERS), 'counter limit')
    require(all(r[k] <= 15 for k in ['raw', 'qualified', 'valid']), 'input range')
    require(all(r[k] <= 7 for k in ['reference', 'touched', 'absent'] + LAST[:4]), 'mask range')
    require(r['opened'] <= 1 and r['quiet'] <= 100, 'qualification range')
    require(r['age'] < DEADLINE and r['last_age'] < DEADLINE, 'deadline')
    require(r['starts'] == r['completed'] + r['aborted'] + (r['phase'] == 2), 'session conservation')
    require(not ((r['last_before'] ^ r['last_after']) & ~r['last_touched']), 'changed but untouched')
    require(not (r['last_returned'] & ~(r['last_before'] & r['last_after'] & r['last_touched'])),
            'impossible returned mask')
    if not r['completed']:
        require(not any(r[k] for k in LAST), 'result without completion')
    else:
        require(r['last_age'] >= 109, 'completion too short')
    if r['phase'] == 1:
        require(r['valid'] == 15 and r['raw'] == r['qualified'] and r['raw'] & 8,
                'invalid sealed baseline')
        require(r['reference'] == (r['qualified'] & 7), 'reference disagreement')
    if r['phase'] == 2:
        require(r['valid'] == 15, 'active before qualification')

class Session:
    def __init__(self, provenance):
        self.provenance = provenance
        self.connected = provenance == 'live'
        self.rows = deque(maxlen=2000)
        self.blocked = False
        self.warning = ''
        self.parser_errors = 0
        self.anchor_host = None
        self.last_host = None
        self.last_progress = None
        self.device_elapsed = 0
        self.advances = 0
        self.accepted = 0

    def fail(self, message):
        self.blocked = True
        self.warning = message
        raise ValueError(message)

    def accept(self, row, host=None):
        host = time.monotonic() if host is None else host
        if self.blocked:
            raise ValueError(self.warning or 'session blocked; reconnect explicitly')
        try:
            validate(row)
            if self.rows:
                old = self.rows[-1]
                dt = (row['now'] - old['now']) & 0xffffffff
                require(dt < 0x80000000, 'device timestamp rollback')
                if dt == 0:
                    require(row == old, 'contradictory duplicate')
                    return
                require(all(row[k] >= old[k] for k in COUNTERS), 'counter rollback; possible reset')
                ds = row['sequence'] - old['sequence']
                require(ds <= dt, 'acquisition exceeds elapsed ticks')
                dc = row['completed'] - old['completed']
                da = row['aborted'] - old['aborted']
                dn = row['starts'] - old['starts']
                require(dn <= ds and dc <= ds and da <= ds + (old['phase'] == 2), 'outcomes exceed acquisition')
                if not dc:
                    require(all(row[k] == old[k] for k in LAST), 'retained result changed')
                else:
                    available = ds + (old['age'] if old['phase'] == 2 else 0)
                    require(row['last_age'] <= available and dc * 109 <= available,
                            'completed sessions exceed acquired time')
                    if dc == 1 and not da and old['phase'] in (1, 2):
                        require(row['last_before'] == old['reference'], 'result changed its baseline')
                if old['phase'] == 3:
                    require(all(row[k] == old[k] for k in FIELDS if k not in ('now', 'tx_errors', 'fault')),
                            'halted evidence changed')
                    require((row['fault'] | old['fault']) == row['fault'], 'fault disappeared')
                if old['phase'] == 2 and row['phase'] == 2 and not dc and not da:
                    require(row['age'] == old['age'] + ds, 'active age discontinuity')
                    require(row['reference'] == old['reference'], 'active reference changed')
                    for k in ['touched', 'absent', 'opened']:
                        require((row[k] | old[k]) == row[k], 'active evidence disappeared')
                self.device_elapsed += dt
                if self.provenance == 'live':
                    assert self.last_host is not None and self.anchor_host is not None
                    require(host - self.last_host <= 2, 'reception pause; reconnect')
                    require(abs(self.device_elapsed / 1000 - (host - self.anchor_host)) <= 2,
                            'host/device progress diverged; reconnect')
                if ds:
                    self.advances += 1
                    self.last_progress = host
            else:
                self.anchor_host = host
            self.rows.append(dict(row))
            self.accepted += 1
            self.last_host = host
        except ValueError as exc:
            self.fail(str(exc))

    def fresh(self, host=None):
        host = time.monotonic() if host is None else host
        if self.provenance != 'live' or not self.connected or not self.rows or self.blocked:
            return False
        assert self.last_host is not None
        if host - self.last_host > 2:
            self.blocked = True
            self.warning = 'reception pause; reconnect'
            return False
        if self.last_progress is not None and host - self.last_progress > 2:
            self.blocked = True
            self.warning = 'acquisition sequence stopped; reconnect'
            return False
        return self.advances > 0 and self.rows[-1]['phase'] != 3

    def export(self, path, host=None):
        fresh = self.fresh(host)
        metadata = {'author': 'jayis1', 'provenance': self.provenance,
                    'connected': self.connected, 'fresh': fresh, 'blocked': self.blocked,
                    'warning': self.warning, 'parser_errors': self.parser_errors,
                    'accepted_snapshots': self.accepted, 'retained_snapshots': len(self.rows),
                    'note': 'Periodic snapshots, not a complete event log. Contacts do not prove identity.'}
        fd = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
        with os.fdopen(fd, 'w', newline='') as output:
            writer = csv.writer(output)
            writer.writerow(['metadata', json.dumps(metadata, sort_keys=True)])
            writer.writerow(FIELDS)
            for row in self.rows:
                writer.writerow([row[k] for k in FIELDS])

def pocket_names(mask):
    return ', '.join(name for i, name in enumerate('ABC') if mask & (1 << i)) or 'none'
