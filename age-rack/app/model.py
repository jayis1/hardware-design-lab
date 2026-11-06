# Author: jayis1. SPDX-License-Identifier: MIT
"""AR1 cohort-rank validation and bounded, provenance-preserving sessions."""
from collections import deque
import csv
import os
import time
FIELDS = ('sequence now state fault raw qualified valid epoch occupied insertions removals abandoned ambiguous last_mask'.split()
          + [prefix+str(i) for prefix in ('rank','stamp','low','high') for i in range(4)]
          + ['tx_errors','gaps'])
STATES = ('WAIT','RUNNING','HALT')
COUNTERS = ['epoch','insertions','removals','abandoned','ambiguous','tx_errors','gaps'] + ['rank'+str(i) for i in range(4)]
LAST = ['last_mask'] + [p+str(i) for p in ('low','high') for i in range(4)]
def require(condition, message):
    if not condition:
        raise ValueError(message)
def validate(r):
    require(set(r)==set(FIELDS), 'field contract')
    require(all(type(v) is int and 0<=v<=0xffffffff for v in r.values()), 'integer range')
    require(r['state'] in (0,1,2) and r['fault'] in (0,1,2,4,8), 'state/fault range')
    require((r['state']==2)==bool(r['fault']), 'halt/fault mismatch')
    require(all(r[x]<=15 for x in ('raw','qualified','valid','occupied','last_mask')), 'contact range')
    require(r['gaps']==int(r['fault']==4), 'gap/fault mismatch')
    require(all(r[x]<=1000000 for x in COUNTERS), 'counter limit')
    require(r['abandoned']<=4 and (not r['abandoned'] or r['state']==2), 'abandonment')
    require(r['insertions']==r['removals']+r['abandoned']+r['occupied'].bit_count(), 'inventory conservation')
    require(r['epoch']<=r['insertions']<=4*r['epoch'], 'cohort count')
    require(bool(r['epoch'])==bool(r['insertions']), 'cohort presence')
    require(r['state']!=0 or not (r['insertions'] or r['occupied']), 'WAIT history')
    require(r['state']!=2 or not r['occupied'], 'halt inventory')
    require(r['state']!=1 or (r['valid']==15 and r['occupied']==r['qualified']), 'qualified inventory')
    require(r['removals']==r['ambiguous']+sum(r['rank'+str(i)] for i in range(4)), 'classification conservation')
    require(bool(r['last_mask'])==bool(r['removals']), 'latest removal presence')
    require(r['last_mask'].bit_count()<=r['removals'], 'latest removal count')
    latest=[0]*4; ambiguous=0
    for i in range(4):
        stamp=r['stamp'+str(i)]; lo=r['low'+str(i)]; hi=r['high'+str(i)]
        require(bool(stamp)==bool(r['occupied']&(1<<i)) and stamp<=r['epoch'], 'live stamp')
        require(0<=lo<=hi<=3, 'rank interval')
        if r['last_mask']&(1<<i):
            if lo==hi: latest[lo]+=1
            else: ambiguous+=1
        else:
            require(not (lo or hi), 'interval outside last group')
    require(ambiguous<=r['ambiguous'], 'latest ambiguity budget')
    require(all(latest[i]<=r['rank'+str(i)] for i in range(4)), 'latest rank budget')

class Session:
    def __init__(self, provenance):
        require(provenance in ('live','replay'), 'provenance')
        self.provenance=provenance
        self.rows=deque(maxlen=512)
        self.connected=False
        self.warning=''
        self.errors=0
        self.last_progress=None
        self.anchor_host=None
        self.device_elapsed=0
        self.progress=0
    def accept(self, row, host=None):
        if self.warning:
            raise ValueError(self.warning)
        try:
            return self._accept(row,time.monotonic() if host is None else host)
        except ValueError as exc:
            self.warning=str(exc)
            raise
    def _accept(self,r,host):
        validate(r)
        if self.rows:
            p=self.rows[-1]
            if r==p:
                return False
            dt=(r['now']-p['now'])&0xffffffff
            ds=r['sequence']-p['sequence']
            require(0<dt<0x80000000 and 0<=ds<=dt, 'reset or contradictory duplicate')
            require(all(r[x]>=p[x] for x in COUNTERS), 'counter rollback')
            if p['state']==2:
                require(all(r[x]==p[x] for x in FIELDS if x not in ('now','tx_errors')), 'halted evidence changed')
            else:
                require(r['state']!=0 or p['state']==0, 'baseline regression')
                require(ds or r['state']==2, 'clock without acquisition')
                if r['state']!=2:
                    require(ds==dt, 'unreported acquisition gap')
                added=r['insertions']-p['insertions']
                removed=r['removals']-p['removals']
                epochs=r['epoch']-p['epoch']
                require(epochs<=added<=4*epochs, 'new cohort count')
                require(added<=4*((ds+4)//5), 'insertion sample budget')
                require(removed<=p['occupied'].bit_count()+added, 'removal inventory budget')
                require(removed<=4*((ds+4)//5), 'removal sample budget')
                if not removed:
                    require(all(r[x]==p[x] for x in LAST), 'retained intervals changed')
                changed=0; lost=0
                for i in range(4):
                    old=p['stamp'+str(i)]; new=r['stamp'+str(i)]
                    if new and new!=old:
                        require(new>p['epoch'], 'retroactive insertion stamp')
                        changed+=1
                    if old and new!=old:
                        lost+=1
                require(changed<=added, 'new stamps without insertions')
                require(lost<=removed+r['abandoned']-p['abandoned'], 'lost stamps without removals')
                # If no hidden removal occurred, compare exact group ranks with
                # prior inventory, independently of same-sample additions.
                if removed and removed==lost and not added and r['state']!=2:
                    mask=sum(1<<i for i in range(4) if p['stamp'+str(i)] and not r['stamp'+str(i)])
                    # Only assert last-group equality when all removals are
                    # one contact: sparse multi-group traffic is not ordered.
                    if removed==1:
                        i=(mask.bit_length()-1); stamp=p['stamp'+str(i)]
                        lo=sum(0<p['stamp'+str(j)]<stamp for j in range(4))
                        hi=sum(j!=i and 0<p['stamp'+str(j)]<=stamp for j in range(4))
                        require(r['last_mask']==mask and r['low'+str(i)]==lo and r['high'+str(i)]==hi, 'single departure rank changed')
                        require(r['ambiguous']-p['ambiguous']==int(lo!=hi), 'single departure ambiguity changed')
                        require(all(r['rank'+str(j)]-p['rank'+str(j)]==int(lo==hi and j==lo)
                                    for j in range(4)), 'single departure histogram changed')
            self.device_elapsed+=dt
            if self.provenance=='live':
                require(abs((host-self.anchor_host)*1000-self.device_elapsed)<=2000, 'host/device progress mismatch; reconnect')
            if ds:
                self.last_progress=host
                self.progress+=1
        else:
            self.anchor_host=host
            self.last_progress=host
        self.rows.append(dict(r))
        return True
    def fresh(self,host=None):
        host=time.monotonic() if host is None else host
        return bool(self.provenance=='live' and self.connected and not self.warning and
                    self.progress and self.rows and self.rows[-1]['state']==1 and
                    self.last_progress is not None and 0<=host-self.last_progress<=1)
    def export(self,path):
        fd=os.open(path,os.O_WRONLY|os.O_CREAT|os.O_EXCL,0o600)
        with os.fdopen(fd,'w',newline='') as handle:
            writer=csv.writer(handle)
            writer.writerow(['author','provenance','connected','fresh','parser_errors','warning'])
            writer.writerow(['jayis1',self.provenance,self.connected,self.fresh(),self.errors,self.warning])
            writer.writerow(FIELDS)
            writer.writerows([r[x] for x in FIELDS] for r in self.rows)
