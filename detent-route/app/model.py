# Author: jayis1. SPDX-License-Identifier: MIT
"""DR1 fixed-policy snapshots and conservative session validation."""
import csv
import os
import time
from collections import deque

FIELDS = ('sequence now fault raw qualified ready station active origin age starts cw ccw '
          'returns jumps aborted reversals cw_laps ccw_laps overlaps shortcuts timeouts '
          'gaps tx_errors direction run last_valid last_from last_to last_ms').split()
COUNTERS = FIELDS[10:24]
OUTCOMES = ('cw','ccw','returns','jumps')
MASK = (1 << 32) - 1

def require(condition, message):
    if not condition:
        raise ValueError(message)

def validate(r):
    require(set(r) == set(FIELDS), 'field set')
    require(all(type(v) is int and 0 <= v <= MASK for v in r.values()), 'unsigned range')
    for k in ('raw','qualified','fault'):
        require(r[k] <= 15, k)
    for k in ('ready','active','last_valid'):
        require(r[k] <= 1, k)
    for k in ('station','origin','last_from','last_to'):
        require(r[k] <= 4, k)
    require(r['direction'] <= 2 and r['run'] <= 3, 'run')
    require(r['age'] < 10000, 'active deadline')
    completed = sum(r[k] for k in OUTCOMES)
    require(completed + r['aborted'] + r['active'] == r['starts'], 'journey conservation')
    require(r['reversals'] <= r['cw'] + r['ccw'], 'reversals')
    require(4*r['cw_laps'] <= r['cw'] and 4*r['ccw_laps'] <= r['ccw'], 'laps')
    require(r['timeouts'] <= r['aborted'], 'timeouts')
    require(r['direction'] or not r['run'], 'directionless run')
    if r['active']:
        require(r['origin'] < 4 and r['station'] == 4 and r['ready'] and not r['fault'], 'active state')
    else:
        require(not r['age'] and r['origin'] == 4, 'idle state')
    require(bool(r['last_valid']) == bool(completed), 'last presence')
    if completed:
        require(r['last_from'] < 4 and r['last_to'] < 4 and 6 <= r['last_ms'] < 10000, 'last result')
    else:
        require(r['last_from'] == 4 and r['last_to'] == 4 and not r['last_ms'], 'empty result')

class Session:
    def __init__(self, source):
        self.source = source
        self.rows = deque(maxlen=2000)
        self.connected = False
        self.blocked = False
        self.warning = ''
        self.losses = 0
        self.last_host = None
        self.base_host = None
        self.device_elapsed = 0
        self.progress = False

    def accept(self, row, host=None):
        if self.blocked:
            raise ValueError('session blocked; reconnect explicitly')
        host = time.monotonic() if host is None else host
        try:
            validate(row)
            if self.rows:
                old = self.rows[-1]
                ds = (row['sequence'] - old['sequence']) & MASK
                dt = (row['now'] - old['now']) & MASK
                if ds == 0:
                    require(row == old, 'contradictory duplicate')
                    return
                require(ds < (1 << 31) and 0 < dt < (1 << 31), 'reset or reordering')
                delta = {k: row[k]-old[k] for k in COUNTERS}
                require(all(v >= 0 for v in delta.values()), 'counter rollback')
                require((row['fault'] | old['fault']) == row['fault'], 'fault disappeared')
                finished = sum(delta[k] for k in OUTCOMES)
                require(finished + delta['aborted'] <= delta['starts'] + old['active'], 'unstarted outcome')
                require(delta['starts'] <= (dt+5)//6, 'start rate')
                require(delta['reversals'] <= delta['cw']+delta['ccw'], 'reversal growth')
                for direction, key in ((1,'cw'),(2,'ccw')):
                    credit = old['run'] if old['direction'] == direction else 0
                    require(4*delta[key+'_laps'] <= credit+delta[key], 'lap growth')
                require(delta['timeouts'] <= delta['aborted'], 'timeout growth')
                if not finished:
                    for k in ('last_valid','last_from','last_to','last_ms'):
                        require(row[k] == old[k], 'retained result changed')
                if old['active'] and row['active'] and not delta['starts']:
                    require(row['origin'] == old['origin'] and row['age'] == old['age']+dt, 'continuing transit')
                if finished > 0:
                    kind = ('returns','cw','jumps','ccw')[(row['last_to']-row['last_from'])%4]
                    require(delta[kind] > 0, 'last classification')
                    require(row['last_ms'] <= dt + (old['age'] if old['active'] else 0), 'completion age')
                    if old['active'] and not delta['starts']:
                        require(row['last_from'] == old['origin'], 'origin changed')
                if old['fault']:
                    for k in FIELDS[4:23]+FIELDS[24:]:
                        require(row[k] == old[k], 'halted evidence changed')
                if self.source == 'live':
                    assert self.last_host is not None and self.base_host is not None
                    require(host-self.last_host <= 2, 'host pause; export and reconnect')
                    require(abs((self.device_elapsed+dt)/1000-(host-self.base_host)) <= 2,
                            'device/host progress mismatch; reconnect')
                self.device_elapsed += dt
                self.losses += ds-1
                self.progress = True
            else:
                self.base_host = host
            self.last_host = host
            self.rows.append(dict(row))
        except ValueError as exc:
            self.blocked = True
            self.warning = str(exc)
            raise

    def live(self, host=None):
        host = time.monotonic() if host is None else host
        return (self.source == 'live' and self.connected and not self.blocked and
                self.progress and self.last_host is not None and host-self.last_host <= 2)

    def export(self, path, errors=0):
        # Refuse symlink/existing-file overwrites; metadata survives empty sessions.
        fd = os.open(path, os.O_WRONLY|os.O_CREAT|os.O_EXCL, 0o600)
        with os.fdopen(fd,'w',newline='') as handle:
            writer = csv.writer(handle)
            writer.writerow(['author','kind','source','connected','blocked','live','parser_errors',
                             'sequence_losses','warning','age_seconds']+FIELDS)
            metadata = ['jayis1','metadata',self.source,int(self.connected),int(self.blocked),
                        int(self.live()),errors,self.losses,self.warning,
                        '' if self.last_host is None else max(0,time.monotonic()-self.last_host)]
            writer.writerow(metadata)
            for row in self.rows:
                writer.writerow([metadata[0],'snapshot']+metadata[2:]+[row[k] for k in FIELDS])
