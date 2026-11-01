# Author: jayis1. SPDX-License-Identifier: MIT
"""Deadband-aware BO1 evidence, sparse consistency checks and private exports."""
import csv
import os
import time
from collections import deque
FIELDS=('now state levels valid starts accepted rejected aborted gaps faults tx_errors '
        'age dwell selected outcome evidence conflict '+ ' '.join('c'+str(i) for i in range(9))).split()
COUNTERS=('starts','accepted','rejected','aborted','gaps','tx_errors')+tuple('c'+str(i) for i in range(9))
STATES=('WAIT','READY','ACTIVE','DONE','HALT')

def conflict(mask):
    # Bellman-Ford with lexicographic (bound, strictness) weights, independent
    # of the embedded Floyd-Warshall implementation. A strict zero cycle fails.
    edges=[]
    for p,(a,b) in enumerate(((0,1),(0,2),(1,2))):
        if mask & (1<<(3*p)): edges.extend(((a,b,1,0),(b,a,1,0)))
        if mask & (2<<(3*p)): edges.append((a,b,-1,-1))
        if mask & (4<<(3*p)): edges.append((b,a,-1,-1))
    distances=[(0,0)]*3
    for iteration in range(3):
        changed=False
        for a,b,weight,strict in edges:
            value=(distances[a][0]+weight,distances[a][1]+strict)
            if value<distances[b]:
                distances[b]=value; changed=True
        if not changed: return 0
    return 1

def validate(r):
    if set(r)!=set(FIELDS) or any(type(v) is not int or not 0<=v<=0xffffffff for v in r.values()):
        raise ValueError('unsigned field contract')
    if r['state']>4 or r['levels']>15 or r['valid']>15 or r['faults']>15:
        raise ValueError('state or masks')
    if bool(r['faults'])!=(r['state']==4): raise ValueError('halt/fault mismatch')
    if r['aborted']>1 or (r['aborted'] and r['state']!=4): raise ValueError('abort requires halt')
    if r['gaps']>1 or bool(r['gaps'])!=bool(r['faults']&1): raise ValueError('gap mismatch')
    if r['starts']!=r['accepted']+r['rejected']+r['aborted']+(r['state']==2):
        raise ValueError('trial conservation')
    if sum(r['c'+str(i)] for i in range(9))!=r['accepted']:
        raise ValueError('outcome conservation')
    evidence=sum(1<<i for i in range(9) if r['c'+str(i)])
    if r['evidence']!=evidence or r['conflict']!=conflict(evidence):
        raise ValueError('evidence or feasibility disagreement')
    if r['selected']>3 or r['outcome']>3 or r['age']>10000 or r['dwell']>500:
        raise ValueError('trial bounds')
    if r['state']==2 and (not r['selected'] or r['age']>=10000 or r['dwell']>=500 or
                          r['valid']!=15 or r['levels']&3!=r['selected']):
        raise ValueError('active state')
    if r['state']==1 and (r['levels']&3 or r['valid']!=15): raise ValueError('ready state')

class Session:
    def __init__(self,provenance):
        if provenance not in ('live','replay'): raise ValueError('provenance')
        self.provenance=provenance; self.rows=deque(maxlen=2000)
        self.connected=provenance=='live'; self.warning=''; self.errors=0
        self.progress=None; self.anchor=None; self.device_elapsed=0
        self.advances=0; self.received=0

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
                added=r['accepted']-old['accepted']; starts=r['starts']-old['starts']
                if added>starts+(old['state']==2): raise ValueError('result without start')
                # One ongoing trial can finish on the next sample; subsequent
                # trials need at least 500 observed settlement intervals each.
                remaining=500-old['dwell'] if old['state']==2 else 501
                if added and dt<remaining+500*(added-1): raise ValueError('settlement rate')
                if starts>dt: raise ValueError('start rate')
                if old['state']==2 and starts==0:
                    if r['state']==2:
                        if r['age']-old['age']!=dt or r['selected']!=old['selected']:
                            raise ValueError('active age or identity')
                        if r['dwell']>old['dwell']+dt: raise ValueError('invented settlement')
                        if r['outcome']!=old['outcome'] and r['dwell']>=dt:
                            raise ValueError('settlement changed outcome')
                    if added:
                        if added!=1 or not old['age']<r['age']<10000 or r['age']-old['age']>dt:
                            raise ValueError('completion deadline')
                        changes=[i for i in range(9) if r['c'+str(i)]!=old['c'+str(i)]]
                        if len(changes)!=1 or changes[0]//3!=old['selected']-1:
                            raise ValueError('completion changed pair')
                        result=changes[0]%3
                        if r['outcome']!=result or r['dwell']!=500:
                            raise ValueError('completion outcome mismatch')
                        if result!=old['outcome'] and dt<501:
                            raise ValueError('completion reused other outcome settlement')
                if old['state']==4:
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
            writer=csv.writer(handle)
            writer.writerow(['author','jayis1','provenance',self.provenance,'label',"'"+label,
                             'connected',self.connected,'fresh',self.fresh(),'parser_errors',self.errors,
                             'warning',"'"+self.warning,'received',self.received,'retained',len(self.rows)])
            writer.writerow(FIELDS)
            for r in self.rows: writer.writerow([r[k] for k in FIELDS])
