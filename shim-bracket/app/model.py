# Author: jayis1. SPDX-License-Identifier: MIT
"""SB1 declaration evidence. Bounds are nominal labels, not calibrated sizes."""
import csv
import os
import time
from collections import deque
FIELDS=('now state levels valid starts accepted rejected aborted gaps faults tx_errors '
        'pass refuse conflict age tainted selected outcome').split()
COUNTERS=('starts','accepted','rejected','aborted','gaps','tx_errors')
STATES=('WAIT','READY','ACTIVE','HALT')
THICKNESS=('0.25','0.50','0.75','1.00')

def validate(r):
    if set(r)!=set(FIELDS) or any(type(v) is not int or not 0<=v<=0xffffffff for v in r.values()):
        raise ValueError('unsigned field contract')
    if r['state']>3 or r['levels']>15 or r['valid']>15 or r['faults']>7:
        raise ValueError('state or bitmask')
    if bool(r['faults'])!=(r['state']==3): raise ValueError('halt/fault mismatch')
    if r['pass']>15 or r['refuse']>15 or r['tainted']>1 or r['conflict']>1:
        raise ValueError('evidence mask')
    conflict=any(r['pass']&(1<<i) and r['refuse']&(1<<j) for i in range(4) for j in range(i+1))
    if bool(r['conflict'])!=conflict: raise ValueError('contradiction must be retained')
    if r['starts']!=r['accepted']+r['rejected']+r['aborted']+(r['state']==2):
        raise ValueError('declaration conservation')
    if r['pass'].bit_count()+r['refuse'].bit_count()>r['accepted']:
        raise ValueError('mask lacks declarations')
    if bool(r['accepted'])!=bool(r['pass']|r['refuse']): raise ValueError('missing evidence')
    if r['age']>10000 or r['selected']>3 or r['outcome'] not in (0,4,8,12):
        raise ValueError('press bounds')
    if r['state']==2 and (r['valid']!=15 or not r['outcome']): raise ValueError('active state')
    if r['state']==2 and (r['age']>=10000 or (r['outcome']==12 and not r['tainted'])):
        raise ValueError('expired or untainted ambiguous active state')
    if r['state']==1 and (r['valid']!=15 or r['levels']&12): raise ValueError('ready state')

def bracket(r):
    if r['conflict']: return 'CONTRADICTORY — no interval; export and restart survey'
    passing=[i for i in range(4) if r['pass']&(1<<i)]
    refusing=[i for i in range(4) if r['refuse']&(1<<i)]
    lower=THICKNESS[max(passing)] if passing else 'unknown'
    upper=THICKNESS[min(refusing)] if refusing else 'unknown'
    return f'Declared fit lower: {lower} mm | declared refusal upper: {upper} mm (nominal)'

class Session:
    def __init__(self,provenance):
        if provenance not in ('live','replay'): raise ValueError('provenance')
        self.provenance=provenance
        self.rows=deque(maxlen=2000)
        self.connected=provenance=='live'
        self.warning=''; self.errors=0; self.progress=None; self.anchor=None
        self.device_elapsed=0; self.advances=0; self.received=0

    def accept(self,r,host=None):
        if self.warning: raise ValueError(self.warning)
        host=time.monotonic() if host is None else host
        try:
            validate(r)
            if self.rows:
                old=self.rows[-1]; dt=(r['now']-old['now'])&0xffffffff
                if not dt:
                    if r!=old: raise ValueError('contradictory duplicate')
                    return
                if dt>=0x80000000: raise ValueError('device clock rollback')
                if any(r[k]<old[k] for k in COUNTERS): raise ValueError('counter rollback')
                if old['faults']&~r['faults']: raise ValueError('lost fault')
                if any(old[k]&~r[k] for k in ('pass','refuse')): raise ValueError('lost declaration')
                added=sum((r[k]&~old[k]).bit_count() for k in ('pass','refuse'))
                if added>r['accepted']-old['accepted']: raise ValueError('new mask without declaration')
                if old['state']==2 and r['starts']==old['starts'] and r['accepted']>old['accepted']:
                    if old['tainted'] or old['outcome'] not in (4,8):
                        raise ValueError('tainted declaration accepted')
                    if r['accepted']!=old['accepted']+1:
                        raise ValueError('multiple completions without new start')
                    expected_pass=old['pass'] | ((1<<old['selected']) if old['outcome']==4 else 0)
                    expected_refuse=old['refuse'] | ((1<<old['selected']) if old['outcome']==8 else 0)
                    if r['pass']!=expected_pass or r['refuse']!=expected_refuse:
                        raise ValueError('completion changed captured identity')
                if r['starts']-old['starts']>dt//5+1: raise ValueError('declaration rate')
                if old['state']==3:
                    frozen=[k for k in FIELDS if k not in ('now','tx_errors','faults')]
                    if any(r[k]!=old[k] for k in frozen): raise ValueError('halted evidence changed')
                if old['state']==2 and r['state']==2 and old['starts']==r['starts']:
                    if r['age']-old['age']!=dt: raise ValueError('active age')
                    if any(r[k]!=old[k] for k in ('selected','outcome')) or r['tainted']<old['tainted']:
                        raise ValueError('active identity changed')
                if self.provenance=='live':
                    if self.progress is not None and host-self.progress>2:
                        raise ValueError('reception pause; reconnect')
                    self.device_elapsed+=dt/1000
                    assert self.anchor is not None
                    if abs(host-self.anchor-self.device_elapsed)>2:
                        raise ValueError('host/device drift or backlog; reconnect')
                self.advances+=1
            else: self.anchor=host
            self.rows.append(dict(r)); self.received+=1; self.progress=host
        except ValueError as exc:
            self.warning=str(exc); raise

    def fresh(self,now=None):
        now=time.monotonic() if now is None else now
        return (self.provenance=='live' and self.connected and not self.warning and
                self.advances>0 and self.progress is not None and now-self.progress<=2)

    def export(self,path,label):
        if len(label)>120 or any(ord(c)<32 for c in label): raise ValueError('printable label, <=120 characters')
        fd=os.open(path,os.O_WRONLY|os.O_CREAT|os.O_EXCL,0o600)
        with os.fdopen(fd,'w',newline='') as handle:
            w=csv.writer(handle)
            w.writerow(['author','jayis1','provenance',self.provenance,'label',"'"+label,
                        'connected',self.connected,'fresh',self.fresh(),'parser_errors',self.errors,
                        'warning',"'"+self.warning,'received',self.received,'retained',len(self.rows)])
            w.writerow(FIELDS)
            for r in self.rows: w.writerow([r[k] for k in FIELDS])
