# Author: jayis1. SPDX-License-Identifier: MIT
"""LR1 semantic validation and bounded session storage. No third-party packages."""
from collections import deque
import csv
import os
import time

FIELDS = ('sequence now fault raw qualified valid phase seen latch disturbed age quiet '
          'starts completed rejected').split() + ['b%d'%i for i in range(12)] + ['gaps','tx_errors']
COUNTERS = ['sequence','starts','completed','rejected','gaps','tx_errors'] + ['b%d'%i for i in range(12)]
PHASES = ['Baseline','Ready','Travel','Return window','HALTED']
REACH = ['No checkpoint witnessed','NEAR only','FAR only','NEAR and FAR']
LIMIT = 1000000000

def require(condition, message):
    if not condition:
        raise ValueError(message)

def validate(r):
    require(set(r)==set(FIELDS), 'unexpected fields')
    require(all(type(v) is int and 0 <= v <= 0xffffffff for v in r.values()), 'integer range')
    require(all(r[k] <= LIMIT for k in COUNTERS), 'counter limit')
    require(all(r[k] <= 15 for k in ('fault','raw','qualified','valid')), 'mask range')
    require(r['phase']<5 and r['seen']<4 and r['latch']<2 and r['disturbed']<2,'enum range')
    require(bool(r['fault']) == (r['phase']==4),'fault/phase')
    require(r['age']<30000 and r['quiet']<=100 and r['gaps']<=1,'window range')
    require(sum(r['b%d'%i] for i in range(12))==r['completed'],'histogram conservation')
    require(r['starts']==r['completed']+r['rejected']+(r['phase'] in (2,3)),'trip conservation')
    require(r['phase'] not in (2,3) or r['valid']==15,'unqualified capture')
    require(r['phase']!=3 or (r['raw']&7)==1,'return without HOME')
    require(r['phase']!=1 or r['quiet']==100,'premature ready')
    require(r['starts']<=r['sequence'],'starts exceed samples')
    require(r['completed']*101<=r['sequence'],'completions exceed observed time')

def transition(a,b):
    dt=(b['now']-a['now']) & 0xffffffff
    require(dt < 0x80000000, 'clock rollback or ambiguous long interval; reconnect')
    if dt==0:
        require(a==b,'contradictory same-time record')
        return
    require(all(b[k]>=a[k] for k in COUNTERS),'counter rollback; reconnect')
    ds=b['sequence']-a['sequence']
    require(ds<=dt,'sample growth exceeds clock')
    if not b['fault']:
        require(ds==dt,'missing acquisitions without terminal fault')
    require(b['fault'] & a['fault']==a['fault'],'fault disappeared')
    dc=b['completed']-a['completed']
    credit=a['quiet']+1 if a['phase']==3 else 0
    require(dc*101<=ds+credit,'impossible completion rate')
    require(b['starts']-a['starts']<=ds,'impossible start rate')
    if a['fault']:
        for key in FIELDS:
            if key not in ('now','fault','tx_errors'):
                require(a[key]==b[key],'halted evidence changed')
    elif ds==0:
        require(b['completed']==a['completed'],'zero-sample completion')
        require(b['starts']==a['starts'],'zero-sample start')
    if a['phase'] in (2,3) and b['phase'] in (2,3) and b['starts']==a['starts']:
        require(b['age']==a['age']+ds,'active age mismatch')
        require(b['seen'] & a['seen']==a['seen'],'checkpoint disappeared')
        if a['phase']==3:
            require(b['phase']==3,'return reverted to travel')
            require(b['seen']==a['seen'],'return gained checkpoints')
            require(b['latch']==a['latch'],'return baseline latch changed')
            require(b['disturbed']>=a['disturbed'],'disturbance disappeared')
            require(b['quiet']==a['quiet']+ds,'return credit mismatch')
    if dc==1 and a['phase'] in (2,3) and b['starts']==a['starts']:
        changed=[i for i in range(12) if b['b%d'%i]!=a['b%d'%i]]
        require(len(changed)==1,'split completion')
        reach=changed[0]//3
        require(reach & a['seen']==a['seen'],'travel checkpoint forgotten')
        if a['phase']==3:
            require(reach==a['seen'],'return row migration')
            outcome=changed[0]%3
            require(outcome==2 or (not a['disturbed'] and outcome==a['latch']), 'return outcome migration')

class Session:
    def __init__(self, provenance):
        self.provenance=provenance
        self.rows=deque(maxlen=2000)
        self.latest=None
        self.error=''
        self.received=None
        self.progress=None
        self.anchor_host=None
        self.elapsed=0
    def accept(self,row):
        if self.error:
            raise ValueError(self.error)
        try:
            validate(row)
            if self.latest:
                transition(self.latest,row)
        except ValueError as exc:
            self.error=str(exc)
            raise
        now=time.monotonic()
        self.received=now
        if self.latest is None:
            self.anchor_host=now
        elif row['now']!=self.latest['now']:
            self.progress=now
            self.elapsed+=(row['now']-self.latest['now']) & 0xffffffff
        if row!=self.latest:
            self.rows.append(dict(row))
        self.latest=dict(row)
    def status(self,connected):
        if self.error: return 'BLOCKED: '+self.error
        if self.provenance!='serial': return self.provenance+' — not live hardware'
        if not connected: return 'Disconnected — retained evidence'
        if self.latest and self.latest['fault']: return 'HALTED acquisition; retained evidence'
        now=time.monotonic()
        if self.progress is None or self.anchor_host is None: return 'Waiting for advancing telemetry'
        drift=abs(self.elapsed/1000-(now-self.anchor_host))
        if now-self.progress>1 or drift>2:
            return 'STALE / backlog or oscillator drift — reconnect'
        return 'Advancing telemetry (not proof of absolute freshness)'
    def export(self,path,connected,parser_errors):
        # Exclusive creation prevents silently replacing a previous experiment.
        fd=os.open(path,os.O_WRONLY|os.O_CREAT|os.O_EXCL,0o600)
        with os.fdopen(fd,'w',newline='') as handle:
            writer=csv.writer(handle)
            writer.writerow(['author','provenance','status','parser_errors','retained_rows'])
            writer.writerow(['jayis1',self.provenance,self.status(connected),parser_errors,len(self.rows)])
            writer.writerow(FIELDS)
            for row in self.rows: writer.writerow([row[k] for k in FIELDS])
