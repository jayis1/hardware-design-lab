# Author: jayis1. SPDX-License-Identifier: MIT
"""Snapshot invariants, bounded session history and private local export."""
from collections import deque
import csv
import os
import time
FIELDS='sequence now state result fault raw qualified valid base vacancy source starts moves canceled abandoned clockwise counterclockwise age last_ms baseline tx_errors gaps'.split()
STATES=('WAIT','READY','CARRY','HALT')
RESULTS=('NONE','MOVED','CANCELED','NONADJACENT','AMBIGUOUS','TIMEOUT','LOST','ABORT')
COUNTERS='starts moves canceled abandoned clockwise counterclockwise tx_errors gaps'.split()
FROZEN='result base vacancy source starts moves canceled abandoned clockwise counterclockwise age last_ms baseline'.split()

def validate(r):
    if set(r)!=set(FIELDS) or any(type(v)!=int or not 0<=v<=0xffffffff for v in r.values()):
        raise ValueError('field or integer bounds')
    for key,limit in dict(state=3,result=7,fault=31,raw=15,qualified=15,valid=1,base=15,
                          vacancy=4,source=4,starts=1000000,age=10000,last_ms=10000,baseline=50).items():
        if r[key]>limit: raise ValueError('range: '+key)
    if r['starts']!=r['moves']+r['canceled']+r['abandoned']+(r['state']==2):
        raise ValueError('lift conservation')
    if r['moves']!=r['clockwise']+r['counterclockwise']: raise ValueError('direction conservation')
    if r['fault'] and r['state']!=3: raise ValueError('fault without HALT')
    if r['state'] in (1,2):
        if not r['valid'] or r['vacancy']>=4 or r['base']!=(15^(1<<r['vacancy'])):
            raise ValueError('uncorrelated tracking')
    if r['state']==1 and (r['source']!=4 or r['qualified']!=r['base']):
        raise ValueError('READY occupancy')
    if r['state']==2:
        if (r['source']-r['vacancy'])%4 not in (1,3) or r['source']>=4 or r['age']>=10000:
            raise ValueError('CARRY identity/deadline')
        if r['qualified']!=(r['base'] & ~(1<<r['source'])): raise ValueError('CARRY occupancy')
    if r['state']==3 and r['result']<3: raise ValueError('HALT outcome')

class Session:
    def __init__(self,origin):
        if origin not in ('live','replay'): raise ValueError('origin')
        self.origin=origin
        self.rows=deque(maxlen=2000)
        self.errors=0
        self.warning=''
        self.blocked=False
        self.host_last=None
        self.host_first=None
        self.device_elapsed=0
        self.advances=0
        self.dropped=0
    def accept(self,r,host=None):
        if self.blocked: raise ValueError(self.warning)
        host=time.monotonic() if host is None else host
        try:
            validate(r)
            if self.rows:
                p=self.rows[-1]
                dt=(r['now']-p['now'])&0xffffffff
                ds=r['sequence']-p['sequence']
                if dt==0 and r==p: return
                if not 0<dt<0x80000000 or ds<0 or ds>dt:
                    raise ValueError('clock/sequence regression or contradictory duplicate')
                if not ds and not (r['fault']&4): raise ValueError('stopped acquisition sequence')
                if any(r[k]<p[k] for k in COUNTERS) or (r['fault']|p['fault'])!=r['fault']:
                    raise ValueError('counter rollback / fault disappearance')
                if p['state']==3 and (r['state']!=3 or any(r[k]!=p[k] for k in FROZEN)):
                    raise ValueError('terminal evidence changed')
                for key in ('starts','moves','canceled','abandoned'):
                    if r[key]-p[key]>ds: raise ValueError('event rate exceeds acquisition')
                if r['starts']==p['starts'] and r['state']==2 and p['state']==2:
                    if any(r[k]!=p[k] for k in ('base','vacancy','source')) or r['age']-p['age']!=ds:
                        raise ValueError('pending transfer changed')
                if r['moves']==p['moves'] and any(r[k]!=p[k] for k in ('clockwise','counterclockwise')):
                    raise ValueError('direction migrated')
                if r['starts']==p['starts'] and p['state']==1 and r['state'] in (0,1) and r['vacancy']!=p['vacancy']:
                    raise ValueError('vacancy moved without lift')
                if self.origin=='live':
                    assert self.host_last is not None and self.host_first is not None
                    if host-self.host_last>2: raise ValueError('reception pause: reconnect required')
                    elapsed=self.device_elapsed+dt
                    host_ms=(host-self.host_first)*1000
                    allowance=1500+min(500,host_ms*0.01)
                    if abs(elapsed-host_ms)>allowance: raise ValueError('clock/backlog divergence: reconnect')
                self.device_elapsed+=dt
                self.advances+=1
            else:
                self.host_first=host
            self.host_last=host
            if len(self.rows)==self.rows.maxlen: self.dropped+=1
            self.rows.append(dict(r))
        except ValueError as exc:
            self.blocked=True; self.warning=str(exc)
            raise
    def fresh(self,now=None,connected=False):
        now=time.monotonic() if now is None else now
        return bool(connected and self.origin=='live' and not self.blocked and self.advances
                    and self.host_last is not None and now-self.host_last<1
                    and self.rows and self.rows[-1]['state']!=3)
    def export(self,path,connected):
        # Never overwrite an existing evidence file; mode is private on creation.
        fd=os.open(path,os.O_WRONLY|os.O_CREAT|os.O_EXCL,0o600)
        with os.fdopen(fd,'w',newline='') as handle:
            writer=csv.writer(handle)
            writer.writerow(['author','jayis1','origin',self.origin,'connected',connected,
                             'fresh',self.fresh(connected=connected),'parser_errors',self.errors,
                             'blocked',self.blocked,'warning',self.warning,'dropped_rows',self.dropped])
            writer.writerow(FIELDS)
            writer.writerows([r[k] for k in FIELDS] for r in self.rows)
