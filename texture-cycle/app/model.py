# Author: jayis1. SPDX-License-Identifier: MIT
"""TC1 graph evidence. A directed cycle is not a defective participant."""
import csv
import os
import time
from collections import deque
FIELDS=('now state levels valid starts references preferences rejected aborted gaps faults '
        'tx_errors graph cycle anchored anchor age tainted selected outcome').split()
COUNTERS=('starts','references','preferences','rejected','aborted','gaps','tx_errors')
STATES=('WAIT','READY','ACTIVE','HALT')
NAMES='ABCD'

def cyclic(graph):
    # Independent depth-first path search, rather than firmware's closure.
    def visit(node,path):
        if node in path: return True
        return any(visit(other,path|{node}) for other in range(4) if graph&(1<<(4*node+other)))
    return any(visit(node,set()) for node in range(4))

def validate(r):
    if set(r)!=set(FIELDS) or any(type(v) is not int or not 0<=v<=0xffffffff for v in r.values()):
        raise ValueError('unsigned field contract')
    if r['state']>3 or r['levels']>15 or r['valid']>15 or r['faults']>7:
        raise ValueError('state or bitmask')
    if bool(r['faults'])!=(r['state']==3): raise ValueError('halt/fault mismatch')
    if r['graph']>65535 or r['graph']&0x8421 or r['tainted']>1:
        raise ValueError('graph bounds or self edge')
    if r['cycle']!=int(cyclic(r['graph'])): raise ValueError('cycle mismatch')
    if r['starts']!=sum(r[k] for k in ('references','preferences','rejected','aborted'))+(r['state']==2):
        raise ValueError('stroke conservation')
    if r['graph'].bit_count()>r['preferences'] or bool(r['preferences'])!=bool(r['graph']):
        raise ValueError('graph lacks declarations')
    if r['anchored']>1 or r['anchor']>3 or r['selected']>3:
        raise ValueError('identity bounds')
    if r['preferences']>r['references'] or (r['anchored'] and r['preferences']==r['references']):
        raise ValueError('reference consumption')
    if r['state']==3 and r['anchored']: raise ValueError('halt retains reference')
    if r['age']>10000 or r['outcome'] not in (0,4,8,12): raise ValueError('press bounds')
    if r['state']==2 and (r['valid']!=15 or not r['outcome'] or r['age']>=10000):
        raise ValueError('active state')
    if r['state']==2 and r['outcome']==12 and not r['tainted']: raise ValueError('ambiguous active')
    if r['state']==1 and (r['valid']!=15 or r['levels']&12): raise ValueError('ready state')

def describe(r):
    if r['cycle']: return 'CYCLE retained — no consistent strict ranking'
    missing=sum(not(r['graph']&((1<<(4*a+b))|(1<<(4*b+a)))) for a in range(4) for b in range(a+1,4))
    return f'No cycle observed; {missing} untested unordered pairs (not a proven ranking)'

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
                if old['graph']&~r['graph']: raise ValueError('lost preference edge')
                dp=r['preferences']-old['preferences']; dr=r['references']-old['references']
                if (r['graph']&~old['graph']).bit_count()>dp: raise ValueError('unfunded preference edge')
                if dp+r['anchored']>dr+old['anchored']: raise ValueError('reused reference')
                if r['anchor']!=old['anchor'] and not dr: raise ValueError('reference identity migration')
                if r['anchored'] and not old['anchored'] and not dr: raise ValueError('reference appeared')
                if old['state']==2 and r['starts']==old['starts']:
                    if r['state']==2:
                        if r['age']-old['age']!=dt: raise ValueError('active age')
                        if any(r[k]!=old[k] for k in ('selected','outcome','anchor','anchored')) or r['tainted']<old['tainted']:
                            raise ValueError('active identity changed')
                    if dp or dr:
                        if old['tainted'] or dp+dr!=1: raise ValueError('tainted or multiple completion')
                        if dr:
                            if old['outcome']!=4 or r['anchor']!=old['selected']:
                                raise ValueError('reference completion mismatch')
                        if dp:
                            if old['outcome']!=8 or not old['anchored'] or old['anchor']==old['selected']:
                                raise ValueError('invalid preference completion')
                            expected=old['graph']|(1<<(4*old['selected']+old['anchor']))
                            if r['graph']!=expected: raise ValueError('preference identity mismatch')
                if r['starts']-old['starts']>dt//5+1: raise ValueError('stroke rate')
                if old['state']==3:
                    frozen=[k for k in FIELDS if k not in ('now','tx_errors','faults')]
                    if any(r[k]!=old[k] for k in frozen): raise ValueError('halted evidence changed')
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
