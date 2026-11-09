# Author: jayis1. SPDX-License-Identifier: MIT
"""LV1 policy validation; necessary sparse bounds, not hidden-history proofs."""
from collections import deque
import time

FIELDS = ('sequence now state fault valid starts completed rejected aa ab ba bb '
          'age answer tainted last_truth last_answer last_ms reason tx_errors gaps').split()
CELLS = ('aa', 'ab', 'ba', 'bb')
COUNTERS = ('sequence', 'starts', 'completed', 'rejected', *CELLS, 'tx_errors', 'gaps')

def require(condition, message):
    if not condition:
        raise ValueError(message)

def validate(r):
    require(set(r) == set(FIELDS), 'schema')
    require(all(type(v) is int and 0 <= v <= 4294967295 for v in r.values()), 'unsigned fields')
    require(all(r[f] <= 1000000000 for f in COUNTERS), 'counter limit')
    require(r['state'] <= 3 and r['fault'] <= 15 and r['valid'] <= 15, 'state range')
    require((r['state'] == 3) == bool(r['fault']), 'fault state')
    require(r['starts'] <= r['sequence'], 'starts versus acquired samples')
    require(r['starts'] == r['completed'] + r['rejected'] + (r['state'] == 2), 'trial conservation')
    require(sum(r[f] for f in CELLS) == r['completed'], 'matrix conservation')
    require(r['age'] < 60000 and r['answer'] <= 2 and r['tainted'] <= 1, 'active fields')
    require(r['state'] == 2 or not (r['age'] or r['answer'] or r['tainted']), 'inactive fields')
    require(r['state'] != 2 or r['valid'] == 15, 'unqualified active')
    require(r['reason'] <= 5 and r['last_truth'] <= 2 and r['last_answer'] <= 2 and r['last_ms'] < 60000, 'result bounds')
    if not r['completed']:
        require(not (r['last_truth'] or r['last_answer'] or r['last_ms']), 'invented result')
    else:
        require(r['last_truth'] and r['last_answer'] and r['last_ms'], 'missing result')
        cell = CELLS[2*(r['last_truth']-1)+r['last_answer']-1]
        require(r[cell] > 0, 'result absent from matrix')

class Session:
    def __init__(self, origin):
        self.origin = origin
        self.rows = deque(maxlen=2000)
        self.connected = False
        self.error = ''
        self.progress_host = None
        self.anchor_host = None
        self.elapsed = 0
        self.advances = 0

    def accept(self, r, host=None):
        if self.error:
            raise ValueError(self.error)
        try:
            return self._accept(r, time.monotonic() if host is None else host)
        except ValueError as exc:
            self.error = str(exc)
            raise

    def _accept(self, r, host):
        validate(r)
        if self.rows:
            old = self.rows[-1]
            dt = (r['now'] - old['now']) & 0xffffffff
            if dt == 0:
                require(r == old, 'contradictory duplicate')
                return False
            require(dt < 0x80000000, 'clock rollback')
            require(all(r[f] >= old[f] for f in COUNTERS), 'counter rollback/reset')
            ds = r['sequence']-old['sequence']
            require(ds <= dt, 'acquisition exceeds elapsed time')
            if r['state'] != 3:
                require(ds == dt, 'missing acquisition without halt')
            require(r['fault'] | old['fault'] == r['fault'], 'lost fault')
            require(r['valid'] | old['valid'] == r['valid'], 'lost qualification')
            dc = r['completed'] - old['completed']
            dr = r['rejected'] - old['rejected']
            starts = r['starts'] - old['starts']
            require(starts <= ds, 'impossible starts')
            require(dc + dr <= ds + (old['state'] == 2 and r['state'] == 3), 'impossible dispositions')
            if not dc:
                require(all(r[f] == old[f] for f in ('last_truth','last_answer','last_ms')), 'retained result changed')
            else:
                if old['state'] != 2:
                    require(ds >= 11, 'insufficient acquisition for a new complete trial')
                if old['state'] == 2 and not starts:
                    require(not old['tainted'], 'tainted trial cannot be accepted')
                    require(not old['answer'] or r['last_answer'] == old['answer'], 'completed answer changed')
                cell = CELLS[2*(r['last_truth']-1)+r['last_answer']-1]
                require(r[cell] > old[cell], 'last result has no matching new count')
                require(r['last_ms'] <= ds + old['age'], 'result exceeds available acquisition')
            if old['state'] == 2 and r['state'] == 2 and not starts:
                require(r['age'] == old['age'] + ds, 'active age discontinuity')
                require(r['tainted'] >= old['tainted'], 'lost taint')
                require(not old['answer'] or r['answer'] == old['answer'], 'answer changed')
            if old['state'] == 3:
                require(all(r[f] == old[f] for f in FIELDS if f not in ('now','tx_errors','fault')), 'halted evidence changed')
            if self.origin == 'live':
                require(host-self.progress_host <= 2, 'host pause: reconnect required')
                self.elapsed += dt
                require(abs(self.elapsed/1000-(host-self.anchor_host)) <= 2, 'device/host drift or backlog')
            self.advances += 1
        else:
            self.anchor_host = host
        self.progress_host = host
        self.rows.append(dict(r))
        return True

    def fresh(self, host=None):
        host = time.monotonic() if host is None else host
        return bool(self.connected and not self.error and self.advances and self.rows
                    and self.rows[-1]['state'] != 3 and self.progress_host is not None
                    and host-self.progress_host <= 2)
