# Author: jayis1. SPDX-License-Identifier: MIT
"""TW1 fixed-policy records and bounded evidence sessions; standard library only."""
from collections import deque
import csv
import os
import time
LANE = 'phase starts deliveries refills canceled abandoned age last_ms'.split()
FIELDS = ('sequence now state fault raw qualified valid'.split() +
          [side+'_'+x for side in ('a','b') for x in LANE] +
          'alternating repeated ties anchor streak peak tx_errors gaps'.split())
STATES = ('RUNNING', 'HALT')
PHASES = ('UNKNOWN', 'HOME', 'END', 'OUTBOUND', 'RETURNING')
COUNTERS = ([side+'_'+x for side in ('a','b') for x in
             ('starts','deliveries','refills','canceled','abandoned')] +
            'alternating repeated ties peak tx_errors gaps'.split())
def require(condition, message):
    if not condition:
        raise ValueError(message)
def validate(r):
    require(set(r) == set(FIELDS), 'field contract')
    require(all(type(v) is int and 0 <= v <= 0xffffffff for v in r.values()), 'integer range')
    require(r['state'] in (0,1) and r['fault'] in (0,1,2,4,8,16,32,64), 'state/fault range')
    require(bool(r['state']) == bool(r['fault']), 'halt/fault mismatch')
    require(all(r[x] <= 15 for x in ('raw','qualified','valid')), 'contact range')
    require(r['gaps'] <= 1 and (not r['gaps'] or r['fault'] == 4), 'gap without halt')
    require(all(r[x] <= 1000000 for x in COUNTERS if x not in ('alternating','repeated','peak')), 'counter range')
    for i, side in enumerate(('a','b')):
        p = {x:r[side+'_'+x] for x in LANE}
        active = p['phase'] in (3,4)
        require(p['phase'] <= 4, 'phase range')
        require(p['starts'] == p['deliveries'] + p['refills'] + p['canceled'] + p['abandoned'] + active, 'excursion conservation')
        require(abs(p['deliveries']-p['refills']) <= 1, 'direction balance')
        require(p['abandoned'] <= 1 and (not p['abandoned'] or r['state']), 'abandonment without terminal fault')
        require(p['age'] < 60000 and p['last_ms'] < 60000, 'duration range')
        require(not p['last_ms'] or p['last_ms'] >= 5, 'completion too short')
        require(bool(p['deliveries'] or p['refills']) == bool(p['last_ms']), 'completion duration missing')
        if r['state']:
            require(p['phase'] == 0, 'halted phase')
            require(p['abandoned'] or not p['age'], 'retained age without abandonment')
        else:
            require(p['deliveries'] <= p['refills'] or p['phase'] in (2,4), 'delivery balance endpoint')
            require(p['refills'] <= p['deliveries'] or p['phase'] in (1,3), 'refill balance endpoint')
            q = (r['qualified'] >> (2*i)) & 3
            valid = (r['valid'] >> (2*i)) & 3
            require(not p['phase'] or valid == 3, 'unqualified phase')
            require(p['phase'] != 1 or q == 1, 'HOME endpoint')
            require(p['phase'] != 2 or q == 2, 'END endpoint')
            require(not active or q == 0, 'active endpoint')
            require(active or not p['age'], 'idle age')
    require(r['ties'] <= min(r['a_deliveries'],r['b_deliveries']), 'tie budget')
    singles = r['a_deliveries'] + r['b_deliveries'] - 2*r['ties']
    require(r['anchor'] <= 2 and bool(r['anchor']) == bool(r['streak']), 'anchor/streak')
    require(r['alternating'] + r['repeated'] + bool(r['anchor']) <= singles, 'pair budget')
    if r['anchor']:
        require(r[('a' if r['anchor']==1 else 'b')+'_deliveries'] > 0, 'anchor without delivery')
    require(r['streak'] <= r['peak'] <= singles, 'streak bound')
    require(not r['peak'] or r['peak'] <= r['repeated']+1, 'peak repeat budget')
    require(not singles or r['peak'], 'missing peak')

class Session:
    def __init__(self, provenance):
        require(provenance in ('live','replay'), 'provenance')
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
            dt = (r['now']-p['now']) & 0xffffffff
            ds = r['sequence']-p['sequence']
            require(0 < dt < 0x80000000 and 0 <= ds <= dt, 'reset or contradictory duplicate')
            require(all(r[x] >= p[x] for x in COUNTERS), 'counter rollback')
            if p['state']:
                require(all(r[x] == p[x] for x in FIELDS if x not in ('now','tx_errors')), 'halted evidence changed')
            else:
                require(ds or r['state'], 'clock without acquisition')
                if not r['state']:
                    require(ds == dt, 'unreported sampling gap')
                added = []
                for side in ('a','b'):
                    key = lambda x: side+'_'+x
                    starts = r[key('starts')]-p[key('starts')]
                    deliveries = r[key('deliveries')]-p[key('deliveries')]
                    refills = r[key('refills')]-p[key('refills')]
                    active = p[key('phase')] in (3,4)
                    require(starts <= (ds+4)//5, 'start sample budget')
                    require(deliveries+refills <= starts+active, 'completion without excursion')
                    if not (deliveries+refills):
                        require(r[key('last_ms')] == p[key('last_ms')], 'retained duration changed')
                    else:
                        require(r[key('last_ms')] <= ds + (p[key('age')] if active else 0), 'completion backdated')
                        if not starts:
                            require((not deliveries or p[key('phase')] == 3) and
                                    (not refills or p[key('phase')] == 4), 'completion direction changed')
                    if not starts and active and r[key('phase')] in (3,4):
                        require(r[key('phase')] == p[key('phase')] and
                                r[key('age')]-p[key('age')] == ds, 'continuing excursion changed')
                    if r[key('phase')] in (3,4) and (starts or not active):
                        require(r[key('age')] < ds, 'new excursion backdated')
                    if r['state'] and r[key('abandoned')]:
                        if starts:
                            require(r[key('age')] < ds, 'new abandoned excursion backdated')
                        else:
                            require(active and p[key('age')] <= r[key('age')] <= p[key('age')]+ds,
                                    'abandoned excursion age outside acquisition')
                    added.append(deliveries)
                new_ties = r['ties']-p['ties']
                require(new_ties <= min(added), 'tie without both deliveries')
                new_singles = sum(added)-2*new_ties
                if new_ties and not new_singles:
                    require(not r['anchor'] and not r['streak'], 'tie retained order')
                if sum(added) == 1:
                    delivery_side = 1 if added[0] else 2
                    same = p['anchor'] == delivery_side
                    streak = p['streak']+1 if same else 1
                    require(r['anchor'] == delivery_side and r['streak'] == streak and
                            r['peak'] == max(p['peak'], streak) and
                            r['repeated'] == p['repeated']+same and
                            r['alternating'] == p['alternating']+bool(p['anchor'] and not same),
                            'single delivery order mismatch')
                require(r['alternating']+r['repeated']-p['alternating']-p['repeated'] <= new_singles, 'pair without new delivery')
                require(r['peak']-p['peak'] <= new_singles, 'peak without deliveries')
                if not sum(added):
                    require(all(r[x] == p[x] for x in ('anchor','streak')), 'delivery order changed without delivery')
            self.device_elapsed += dt
            if self.provenance == 'live':
                require(abs((host-self.anchor_host)*1000-self.device_elapsed) <= 2000, 'host/device progress mismatch; reconnect')
            if ds:
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
                    self.progress and self.rows and not self.rows[-1]['state'] and
                    self.last_progress is not None and 0 <= host-self.last_progress <= 1)
    def export(self, path):
        fd = os.open(path, os.O_WRONLY|os.O_CREAT|os.O_EXCL, 0o600)
        with os.fdopen(fd, 'w', newline='') as handle:
            writer = csv.writer(handle)
            writer.writerow(['author','provenance','connected','fresh','parser_errors','warning'])
            writer.writerow(['jayis1',self.provenance,self.connected,self.fresh(),self.errors,self.warning])
            writer.writerow(FIELDS)
            writer.writerows([r[x] for x in FIELDS] for r in self.rows)
