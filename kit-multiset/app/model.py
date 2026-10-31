# Author: jayis1. SPDX-License-Identifier: MIT
"""KM1 evidence validation and bounded session history; no external packages."""
import csv
import os
import time
from collections import deque
FIELDS = ('now state levels valid starts good bad aborted gaps faults tx_errors '
          'age tainted a b c last_a last_b last_c last_result last_age').split()
COUNTERS = ('starts','good','bad','aborted','gaps','tx_errors')
STATES = ('WAIT','READY','ACTIVE','HALT')

def validate(r):
    if set(r)!=set(FIELDS) or any(type(v) is not int or not 0<=v<=0xffffffff for v in r.values()):
        raise ValueError('unsigned field contract')
    if r['state']>3 or r['levels']>15 or r['valid']>15 or r['faults']>7:
        raise ValueError('state or bitmask')
    if bool(r['faults']) != (r['state']==3): raise ValueError('halt/fault mismatch')
    if r['tainted']>1 or r['last_result']>3 or max(r['age'],r['last_age'])>60000:
        raise ValueError('batch bounds')
    outcomes=r['good']+r['bad']+r['aborted']
    if r['starts']!=outcomes+(r['state']==2): raise ValueError('batch conservation')
    if (outcomes==0)!=(r['last_result']==0): raise ValueError('missing retained outcome')
    if r['state']==2 and (r['valid']!=15 or not r['levels']&8): raise ValueError('active without tray')
    if r['state']==1 and (r['valid']!=15 or r['levels']): raise ValueError('ready baseline')
    result=r['last_result']
    if result==1 and (not r['good'] or [r['last_a'],r['last_b'],r['last_c']]!=[2,1,1]):
        raise ValueError('matching outcome is not recipe')
    if result==2 and not r['bad']: raise ValueError('missing mismatch count')
    if result==3 and not r['aborted']: raise ValueError('missing abort count')

class Session:
    def __init__(self, provenance):
        if provenance not in ('live','replay'): raise ValueError('provenance')
        self.provenance=provenance
        self.rows=deque(maxlen=2000)
        self.connected=provenance=='live'
        self.warning=''
        self.errors=0
        self.progress=None
        self.anchor=None
        self.device_elapsed=0
        self.advances=0
        self.received=0

    def accept(self, r, host=None):
        if self.warning: raise ValueError(self.warning)
        host=time.monotonic() if host is None else host
        try:
            validate(r)
            if self.rows:
                old=self.rows[-1]
                dt=(r['now']-old['now'])&0xffffffff
                if dt==0:
                    if r!=old: raise ValueError('contradictory duplicate')
                    return
                if dt>=0x80000000: raise ValueError('device clock rollback')
                if any(r[k]<old[k] for k in COUNTERS): raise ValueError('counter rollback')
                if old['faults'] & ~r['faults']: raise ValueError('lost sticky fault')
                if r['starts']-old['starts']>dt//5+1: raise ValueError('impossible batch rate')
                outcomes=sum(r[k]-old[k] for k in ('good','bad','aborted'))
                if not outcomes and any(r[k]!=old[k] for k in ('last_a','last_b','last_c','last_result','last_age')):
                    raise ValueError('retained outcome changed without disposition')
                if old['state']==3:
                    frozen=[k for k in FIELDS if k not in ('now','tx_errors','faults')]
                    if any(r[k]!=old[k] for k in frozen): raise ValueError('halted accounting changed')
                if r['starts']==old['starts'] and old['state']==2 and r['state']==2:
                    if r['age']-old['age']!=dt: raise ValueError('continuing batch duration')
                    if r['tainted']<old['tainted']: raise ValueError('lost batch taint')
                    if any(r[k]<old[k] or r[k]-old[k]>dt//5+1 for k in ('a','b','c')):
                        raise ValueError('continuing stroke count')
                if self.provenance=='live':
                    if self.progress is not None and host-self.progress>2:
                        raise ValueError('reception pause; reconnect to discard backlog')
                    self.device_elapsed+=dt/1000
                    assert self.anchor is not None
                    if abs((host-self.anchor)-self.device_elapsed)>2:
                        raise ValueError('host/device drift or backlog; reconnect')
                self.advances+=1
            else:
                self.anchor=host
            self.rows.append(dict(r)); self.received+=1; self.progress=host
        except ValueError as exc:
            self.warning=str(exc)
            raise

    def fresh(self, now=None):
        now=time.monotonic() if now is None else now
        return (self.provenance=='live' and self.connected and not self.warning and
                self.advances>0 and self.progress is not None and now-self.progress<=2)

    def export(self, path, label):
        if len(label)>120 or any(ord(c)<32 for c in label): raise ValueError('label must be printable, <=120 characters')
        # Exclusive create and restrictive permissions protect previous evidence.
        fd=os.open(path,os.O_WRONLY|os.O_CREAT|os.O_EXCL,0o600)
        with os.fdopen(fd,'w',newline='') as f:
            w=csv.writer(f)
            # Prefix free-form strings so spreadsheet programs do not execute them.
            w.writerow(['author','jayis1','provenance',self.provenance,'label',"'"+label,
                        'connected',self.connected,'fresh',self.fresh(),'parser_errors',self.errors,
                        'warning',"'"+self.warning,'received',self.received,'retained',len(self.rows)])
            w.writerow(FIELDS)
            for row in self.rows: w.writerow([row[k] for k in FIELDS])
