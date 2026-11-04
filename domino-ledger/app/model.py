# Author: jayis1. SPDX-License-Identifier: MIT
"""DL1 evidence validation. Sparse snapshots are not complete event histories."""
import csv
from collections import deque
import os
import time

FIELDS = ('now samples phase fault raw mask valid starts completed full partial '
          'disturbed aborted outcome age seen rebound baseline quiet '
          'offset_a offset_b offset_c offset_d tx_errors').split()
OFFSETS = FIELDS[19:23]
COUNTERS = 'samples starts completed full partial disturbed aborted tx_errors'.split()
PHASES = ('WAIT', 'READY', 'ACTIVE', 'DONE', 'HALT')
OUTCOMES = ('pending', 'full / no observed return', 'partial / no observed return',
            'full / re-erected', 'partial / re-erected', 'aborted')
ABSENT = 4294967295

def require(condition, message):
    if not condition:
        raise ValueError(message)

def validate(r):
    require(set(r) == set(FIELDS), 'schema')
    require(all(type(v) is int and 0 <= v <= ABSENT for v in r.values()), 'uint32')
    require(r['phase'] <= 4 and r['fault'] <= 4 and r['outcome'] <= 5, 'enum')
    require((r['phase'] == 4) == (r['fault'] != 0), 'halt/fault')
    require(all(r[k] <= 15 for k in ('raw','mask','valid','seen','rebound')), 'mask')
    require(not r['rebound'] & ~r['seen'], 'return before fall')
    require(r['completed'] == r['full'] + r['partial'], 'completion conservation')
    require(r['starts'] == r['completed'] + r['aborted'] + (r['phase']==2), 'trial conservation')
    require(r['disturbed'] <= r['completed'] and r['aborted'] <= 1, 'outcome totals')
    require(r['age'] <= 10000 and r['baseline'] <= 500 and r['quiet'] < 250, 'timer')
    require(bool(r['starts']) == bool(r['seen']), 'trial presence')
    if r['phase']==2:
        require(r['outcome']==0 and r['age']<10000, 'active deadline')
    present=[]
    for i,key in enumerate(OFFSETS):
        if r['seen'] & (1<<i):
            require(r[key] <= r['age'] and r[key] < 10000, 'fall offset')
            present.append(r[key])
        else:
            require(r[key] == ABSENT, 'absent offset')
    require(not present or min(present)==0, 'missing first fall')
    if r['outcome'] in (1,2,3,4):
        full = r['outcome'] in (1,3)
        require(full == (r['seen']==15), 'full mask')
        require((r['outcome']>=3) == bool(r['rebound']), 'return outcome')
        require(r['completed']>0 and r['full' if full else 'partial']>0, 'missing outcome counter')
        if not full:
            require(r['age']==10000, 'partial before deadline')
        else:
            require(r['age']>=250, 'full before quiet window')
    if r['outcome']==5:
        require(r['aborted']==1 and r['phase']==4, 'abandonment')
    if r['phase']==3:
        require(r['outcome'] in (1,2,3,4), 'done outcome')
    if r['phase'] in (1,2,3):
        require(r['valid']==15, 'unqualified phase')

class Session:
    def __init__(self, provenance):
        self.provenance=provenance
        self.rows=deque(maxlen=2000)
        self.warning=''
        self.last_progress=None
        self.origin_host=None
        self.device_elapsed=0
    @property
    def latest(self):
        return self.rows[-1] if self.rows else None
    def accept(self, row, host=None):
        if self.warning:
            raise ValueError(self.warning)
        try:
            return self._accept(row, time.monotonic() if host is None else host)
        except ValueError as exc:
            self.warning=str(exc)
            raise
    def _accept(self, row, host):
        validate(row)
        old=self.latest
        if old:
            dt=(row['now']-old['now']) & ABSENT
            if dt==0:
                require(row==old, 'contradictory duplicate')
                return False
            require(dt<0x80000000, 'timestamp rollback; reconnect')
            require(all(row[k]>=old[k] for k in COUNTERS), 'counter rollback; reconnect')
            require(row['disturbed']-old['disturbed']<=row['completed']-old['completed'],
                    'return classification without completion')
            require(row['samples']-old['samples']<=dt, 'sample rate')
            require(row['starts']-old['starts']<=dt, 'trial rate')
            if old['phase']==4:
                require(all(row[k]==old[k] for k in FIELDS if k not in ('now','tx_errors')), 'halt changed evidence')
            if row['starts']==old['starts']:
                require(row['seen'] & old['seen']==old['seen'], 'lost first fall')
                require(row['rebound'] & old['rebound']==old['rebound'], 'lost return')
                for i,key in enumerate(OFFSETS):
                    if old['seen'] & (1<<i):
                        require(row[key]==old[key], 'rewritten first fall')
                    elif row['seen'] & (1<<i):
                        require(old['phase']==2 and old['age']<row[key]<=old['age']+dt, 'backdated fall')
                if old['phase']==2:
                    require(old['age']<=row['age']<=old['age']+dt, 'trial clock')
                    if row['phase']==2:
                        require(row['age']==old['age']+dt, 'active clock')
                else:
                    require(all(row[k]==old[k] for k in ('age','seen','rebound','outcome',*OFFSETS)), 'frozen trial changed')
            self.device_elapsed+=dt
            if self.provenance=='live':
                require(host-self.last_progress<=2, 'host reception pause; reconnect')
                # A fixed 2s budget bounds cumulative backlog and clock jumps.
                require(abs(self.device_elapsed/1000-(host-self.origin_host))<=2,
                        'host/device clock disagreement; reconnect')
        else:
            self.origin_host=host
        self.last_progress=host
        self.rows.append(dict(row))
        return True
    def fresh(self, connected, host=None):
        host=time.monotonic() if host is None else host
        return bool(connected and self.provenance=='live' and not self.warning and
                    self.latest and self.latest['phase']!=4 and
                    self.last_progress is not None and host-self.last_progress<=1)
    def export(self, path, connected, errors):
        fd=os.open(path,os.O_WRONLY|os.O_CREAT|os.O_EXCL,0o600)
        with os.fdopen(fd,'w',newline='') as handle:
            writer=csv.writer(handle)
            writer.writerow(['author','jayis1','provenance',self.provenance,'connected',connected,
                             'fresh',self.fresh(connected),'parser_errors',errors,'warning',self.warning,
                             'retained_snapshots',len(self.rows)])
            writer.writerow(FIELDS)
            writer.writerows([r[k] for k in FIELDS] for r in self.rows)
