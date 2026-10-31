# Author: jayis1. SPDX-License-Identifier: MIT
"""SQ1 evidence validation, bounded session retention and private CSV export."""
import csv
import os
import time
from collections import deque
FIELDS=('time raw levels known state starts completed aborted timeouts gaps tx '
        'flags age span seen qualify dark limit').split()+[
    f'{name}{i}' for i in range(4) for name in ('first','last','lit','pulses')]
RESULT=['span','seen']+FIELDS[18:]
COUNTERS='starts completed aborted timeouts gaps tx'.split()

def require(condition,message):
    if not condition: raise ValueError(message)

def validate(r):
    require(set(r)==set(FIELDS),'field schema')
    require(all(type(v) is int and 0<=v<=0xffffffff for v in r.values()),'integer bound')
    require((r['qualify'],r['dark'],r['limit'])==(5,250,10000),'unsupported policy')
    require(all(r[k]<=15 for k in ('raw','levels','known','seen')),'mask bound')
    require(r['state']<=3 and not (r['flags'] & ~29),'state/flag bound')
    require(r['starts']==r['completed']+r['aborted']+(r['state']==2),'capture conservation')
    require(r['timeouts']<=r['aborted'],'timeout conservation')
    require(r['age']<10000 and r['span']<10000,'duration bound')
    require(r['state']==2 or r['age']==0,'inactive age')
    require((r['state']==3)==bool(r['flags'] & ~1),'terminal flags')
    require(bool(r['gaps'])==bool(r['flags']&1),'gap flags')
    require(r['state']!=1 or (r['levels']==0 and r['known']==15),'ready levels')
    require(r['state']!=2 or r['known']==15,'active qualification')
    require(bool(r['completed'])==bool(r['seen']),'result presence')
    ends=[]; firsts=[]
    for i in range(4):
        first,last,lit,pulses=(r[f'{k}{i}'] for k in ('first','last','lit','pulses'))
        if not r['seen']&(1<<i):
            require(not(first or last or lit or pulses),'absent detector evidence')
            continue
        require(0<=first<last<=r['span'],'pulse envelope')
        require(1<=pulses<=lit<=last-first,'pulse occupancy')
        require(pulses!=1 or lit==last-first,'single pulse duration')
        require(pulses-1<=last-first-lit,'fragment dark intervals')
        firsts.append(first); ends.append(last)
    require(max(ends,default=0)==r['span'],'capture span')
    require(not firsts or min(firsts)==0,'capture origin')

class Session:
    def __init__(self,source='none'):
        self.source=source
        self.connected=False
        self.rows=deque(maxlen=500)
        self.evicted=0
        self.errors=0
        self.blocked=''
        self.last_host=None
        self.origin_host=None
        self.device_elapsed=0
        self.progress=0
    def accept(self,row,now=None):
        now=time.monotonic() if now is None else now
        try:
            require(not self.blocked,self.blocked)
            validate(row)
            if self.rows:
                old=self.rows[-1]
                delta=(row['time']-old['time']) & 0xffffffff
                if delta==0:
                    require(row==old,'contradictory duplicate')
                    return
                require(delta<0x80000000,'device time rollback')
                require(all(row[k]>=old[k] for k in COUNTERS),'counter rollback')
                require((row['flags']|old['flags'])==row['flags'],'sticky flag disappeared')
                if row['completed']==old['completed']:
                    require(all(row[k]==old[k] for k in RESULT),'completed evidence changed')
                if old['state']==3:
                    require(row['state']==3,'terminal recovery without reconnect')
                    require(all(row[k]==old[k] for k in COUNTERS if k!='tx'),'halted accounting changed')
                # A completed capture must include 250 dark milliseconds, plus
                # a nonempty lit interval. Existing active age supplies credit.
                newly=row['completed']-old['completed']
                require(newly*251<=delta+old['age'],'completion rate')
                if self.source=='live':
                    assert self.last_host is not None and self.origin_host is not None
                    require(now-self.last_host<=1,'host pause; reconnect required')
                    elapsed=self.device_elapsed+delta
                    require(abs(elapsed/1000-(now-self.origin_host))<=.5,
                            'host/device drift or backlog; reconnect required')
                    self.device_elapsed=elapsed
                self.progress+=1
            else:
                self.origin_host=now
            if len(self.rows)==self.rows.maxlen: self.evicted+=1
            self.rows.append(dict(row))
            self.last_host=now
        except ValueError as exc:
            self.blocked=str(exc)
            raise
    def freshness(self,now=None):
        now=time.monotonic() if now is None else now
        if self.blocked: return 'BLOCKED'
        if self.source!='live': return 'OFFLINE'
        if not self.connected: return 'DISCONNECTED'
        if not self.rows or not self.progress: return 'WAITING'
        assert self.last_host is not None
        return 'LIVE' if now-self.last_host<=1 else 'STALE'
    def decision(self,tolerance):
        if not self.rows or not self.rows[-1]['completed']:
            return 'No completed exposure; darken all four points, then sweep'
        r=self.rows[-1]
        if r['seen']!=15: return 'Missing illuminated points; inspect alignment and lighting'
        if any(r[f'pulses{i}']>1 for i in range(4)):
            return 'Qualified re-openings detected; inspect return motion or lamp flicker'
        widths=[r[f'lit{i}'] for i in range(4)]
        spread=max(widths)-min(widths)
        if spread>tolerance:
            return f'Width spread {spread} ms exceeds local comparison tolerance'
        return f'All four points: width spread {spread} ms; no qualified re-openings'
    def export(self,path,now=None):
        now=time.monotonic() if now is None else now
        fd=os.open(path,os.O_WRONLY|os.O_CREAT|os.O_EXCL,0o600)
        with os.fdopen(fd,'w',newline='') as handle:
            writer=csv.writer(handle)
            writer.writerow(['author','jayis1','source',self.source,'connected',self.connected,
                             'freshness',self.freshness(now),'age_s',
                             '' if self.last_host is None else now-self.last_host,
                             'parser_errors',self.errors,'blocked',self.blocked,'evicted',self.evicted])
            writer.writerow(FIELDS)
            writer.writerows([[r[k] for k in FIELDS] for r in self.rows])
