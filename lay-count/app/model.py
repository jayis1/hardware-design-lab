# Author: jayis1. SPDX-License-Identifier: MIT
"""LC1 evidence validation, bounded sessions, freshness and private exports."""
import csv
import os
import time
from collections import deque
FIELDS='time raw levels known state anchor starts completed aborted pulses abandoned completed_pulses active_pulses active_ms active_bad last_pulses last_ms last_bad last_end bad_total flags gaps tx'.split()
COUNTERS='starts completed aborted pulses abandoned completed_pulses bad_total gaps tx'.split()
RESULT='last_pulses last_ms last_bad last_end'.split()
STATES=('Waiting for endpoint','Ready at endpoint','Traversing','HALTED — reset required')
def validate(r):
    if set(r)!=set(FIELDS) or any(type(v)!=int or not 0<=v<=0xffffffff for v in r.values()):
        raise ValueError('field range')
    if any(r[k]>15 for k in ('raw','levels','known')) or r['state']>3:
        raise ValueError('input/state range')
    if r['flags']>31 or (r['state']==3)!=(r['flags']!=0):
        raise ValueError('fault contradiction')
    if r['anchor']>2 or r['last_end']>2 or max(r['active_bad'],r['last_bad'])>1:
        raise ValueError('side/quality range')
    if r['starts']!=r['completed']+r['aborted']+(r['state']==2):
        raise ValueError('attempt conservation')
    if r['pulses']!=r['completed_pulses']+r['abandoned']+r['active_pulses']:
        raise ValueError('pulse conservation')
    if r['bad_total']>r['completed'] or r['active_ms']>=600000:
        raise ValueError('quality/duration range')
    if r['state']!=2 and any(r[k] for k in ('active_ms','active_pulses','active_bad')):
        raise ValueError('inactive traverse')
    if r['state'] in (1,2) and r['anchor'] not in (1,2):
        raise ValueError('missing origin')
    if not r['completed']:
        if any(r[k] for k in RESULT+['completed_pulses']):raise ValueError('result without completion')
    else:
        if r['last_end'] not in (1,2) or not 0<r['last_ms']<600000:
            raise ValueError('result timing/side')
        if r['last_pulses']>r['completed_pulses'] or r['last_bad']>r['bad_total']:
            raise ValueError('result exceeds aggregate')
        if r['completed']==1 and (r['last_pulses']!=r['completed_pulses'] or r['last_bad']!=r['bad_total']):
            raise ValueError('single traverse aggregate')
    if r['gaps'] and not r['flags']&1:raise ValueError('missing gap flag')

class Session:
    def __init__(self,source):
        if source not in ('live','replay'):raise ValueError('invalid provenance')
        self.source=source
        self.rows=deque(maxlen=6000)
        self.connected=False
        self.blocked=False
        self.warning=''
        self.errors=0
        self.last_host=None
        self.first_host=None
        self.elapsed=0
        self.progress=0
    def reject(self,message):
        self.blocked=True;self.warning=message
        raise ValueError(message)
    def accept(self,row,now=None):
        now=time.monotonic() if now is None else now
        if self.blocked:raise ValueError('session blocked; reconnect explicitly')
        validate(row)
        if self.rows:
            prev=self.rows[-1]
            delta=(row['time']-prev['time'])&0xffffffff
            if delta==0:
                if row!=prev:self.reject('contradictory duplicate')
                return
            if delta>=0x80000000:self.reject('time rollback/reset suspected')
            if any(row[k]<prev[k] for k in COUNTERS):self.reject('counter rollback/reset suspected')
            if row['flags']&prev['flags']!=prev['flags']:self.reject('disappearing fault')
            if row['completed']==prev['completed'] and any(row[k]!=prev[k] for k in RESULT+['completed_pulses','bad_total']):
                self.reject('result changed without completion')
            if row['aborted']==prev['aborted'] and row['abandoned']!=prev['abandoned']:
                self.reject('abandoned pulses changed without abandonment')
            if prev['state']==3 and any(row[k]!=prev[k] for k in FIELDS if k not in ('time','raw','tx','flags')):
                self.reject('halted evidence changed')
            # Qualified rising edges require a qualified low between them.
            if row['pulses']-prev['pulses']>delta//10+1:
                self.reject('pulse rate impossible')
            self.elapsed+=delta
            if self.source=='live':
                assert self.last_host is not None and self.first_host is not None
                if now-self.last_host>1.0:self.reject('host pause; reconnect to discard backlog')
                drift=self.elapsed/1000-(now-self.first_host)
                if abs(drift)>0.75:self.reject('device/host progress mismatch; reconnect')
            self.progress+=1
        else:
            self.first_host=now
        self.rows.append(dict(row));self.last_host=now
    def fresh(self,now=None):
        now=time.monotonic() if now is None else now
        return (self.source=='live' and self.connected and not self.blocked and
                self.progress>0 and self.last_host is not None and now-self.last_host<=1)
    def export(self,path):
        now=time.monotonic()
        fd=os.open(path,os.O_WRONLY|os.O_CREAT|os.O_EXCL,0o600)
        with os.fdopen(fd,'w',newline='') as handle:
            writer=csv.writer(handle)
            writer.writerow(['author','jayis1','source',self.source,'connected',self.connected,
                             'fresh',self.fresh(now),'blocked',self.blocked,'parser_errors',self.errors,
                             'receive_age_s','' if self.last_host is None else now-self.last_host,
                             'warning',self.warning])
            writer.writerow(FIELDS)
            for row in self.rows:writer.writerow([row[k] for k in FIELDS])
