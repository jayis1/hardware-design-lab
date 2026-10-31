# Author: jayis1. SPDX-License-Identifier: MIT
"""PS1 structural and sparse-session validation. No hardware simulation."""
from collections import deque
import csv
import os
import time
FIELDS=['now','raw','state','fault','tx','words','bad','high_ms']
FIELDS += [f'{name}{i}' for name in ('ref','latest','errors') for i in range(16)]
FAULTS=['none','GPIO configuration','watchdog initialization','acquisition gap',
        'data setup','data hold','short clock','clock deadline','clock rearm','counter exhaustion']
STATES=['waiting for low baseline','ready','clock high: provisional','HALTED']

def visits(words,i):
    n=max(words-16,0)
    return n//16+int(i<n%16)

def validate(r):
    if set(r)!=set(FIELDS) or any(type(v) is not int or not 0<=v<=0xffffffff for v in r.values()):
        raise ValueError('fields outside PS1 unsigned range')
    if r['raw']>15 or r['state']>3 or r['fault']>9:
        raise ValueError('invalid state or input')
    if (r['state']==3)!=(r['fault']!=0): raise ValueError('inconsistent halt')
    if r['high_ms']>1000: raise ValueError('pulse duration')
    if r['state']==2 and (not r['raw']&1 or r['high_ms']>=1000): raise ValueError('active pulse')
    if r['state'] in (0,1) and r['high_ms']: raise ValueError('idle pulse')
    if r['state']==1 and r['raw']&1: raise ValueError('ready clock high')
    total=0
    for i in range(16):
        ref,last,err=(r[f'{p}{i}'] for p in ('ref','latest','errors'))
        count=visits(r['words'],i)
        if ref>7 or last>7 or (r['words']<=i and ref): raise ValueError('pattern value')
        if err>count or (not count and last): raise ValueError('slot capacity')
        if count and ((last!=ref and not err) or (last==ref and err==count)):
            raise ValueError('latest outcome contradicts errors')
        total+=err
    if total!=r['bad']: raise ValueError('error conservation')

class Session:
    def __init__(self,source):
        if source not in ('live','replay'): raise ValueError('source')
        self.source=source
        self.rows=deque(maxlen=2048)
        self.connected=source=='live'
        self.blocked=False
        self.warning=''
        self.errors=0
        self.progress=0
        self.last_progress=None
        self.origin_host=None
        self.device_elapsed=0
    def reject(self,message):
        self.blocked=True; self.warning=message
        raise ValueError(message)
    def accept(self,r,host=None):
        if self.blocked: raise ValueError(self.warning)
        validate(r)
        host=time.monotonic() if host is None else host
        if self.rows:
            p=self.rows[-1]
            dt=(r['now']-p['now'])&0xffffffff
            if dt==0:
                if r!=p: self.reject('contradictory duplicate')
                return
            if dt>=0x80000000: self.reject('device timestamp regression: reconnect')
            for name in ('words','bad','tx'):
                if r[name]<p[name]: self.reject('counter regression: possible reset')
            if p['fault'] and r['fault']!=p['fault']: self.reject('changed terminal fault')
            if p['fault']:
                frozen=['words','bad','high_ms']+FIELDS[8:]
                if any(r[k]!=p[k] for k in frozen): self.reject('halted evidence changed')
            if r['words']-p['words']>1+dt//30: self.reject('impossible clock rate')
            added=r['words']-p['words']
            if not r['fault']:
                if not added and p['state']==2:
                    if r['state']!=2 or r['high_ms']!=p['high_ms']+dt or r['raw']!=p['raw']:
                        self.reject('uncompleted pulse duration changed')
                elif not added and r['state']==2 and r['high_ms']>=dt:
                    self.reject('new pulse predates previous sample')
                if not added and p['state']==1 and r['state']==0:
                    self.reject('ready lost without a completed pulse')
                if added:
                    first=max(1,10-p['high_ms']) if p['state']==2 else 11
                    minimum=first+30*(added-1)
                    if r['state']==2: minimum+=20+r['high_ms']
                    if r['state']==1: minimum+=20
                    if dt<minimum: self.reject('pulse transitions exceed available time')
            for i in range(16):
                if p['words']>i and r[f'ref{i}']!=p[f'ref{i}']: self.reject('reference changed')
                old,new=visits(p['words'],i),visits(r['words'],i)
                delta=r[f'errors{i}']-p[f'errors{i}']
                if not 0<=delta<=new-old: self.reject('slot errors without comparisons')
                if new==old and r[f'latest{i}']!=p[f'latest{i}']: self.reject('unvisited slot changed')
                if new>old:
                    final_bad=int(r[f'latest{i}']!=r[f'ref{i}'])
                    if not final_bad<=delta<=new-old-(1-final_bad):
                        self.reject('new slot outcome contradicts increment')
            self.device_elapsed+=dt/1000
            if self.source=='live':
                assert self.origin_host is not None
                # Absolute bounded tolerance, not an ever-growing backlog budget.
                if self.last_progress is not None and host-self.last_progress>2:
                    self.reject('host reception pause: reconnect and flush')
                if abs((host-self.origin_host)-self.device_elapsed)>2:
                    self.reject('clock/delivery divergence: reconnect')
            self.progress+=1
        else:
            self.origin_host=host
        self.last_progress=host
        self.rows.append(r.copy())
    def fresh(self,host=None):
        host=time.monotonic() if host is None else host
        return (self.source=='live' and self.connected and not self.blocked and self.progress>=1
                and self.last_progress is not None and host-self.last_progress<=1)
    def export(self,path,host=None):
        fd=os.open(path,os.O_WRONLY|os.O_CREAT|os.O_EXCL,0o600)
        with os.fdopen(fd,'w',newline='') as out:
            writer=csv.writer(out)
            writer.writerow(['author','jayis1','source',self.source,'connected',self.connected,
                             'fresh',self.fresh(host),'blocked',self.blocked,'parser_errors',self.errors,
                             'warning',self.warning,'retained_rows',len(self.rows)])
            writer.writerow(FIELDS)
            writer.writerows([r[f] for f in FIELDS] for r in self.rows)
