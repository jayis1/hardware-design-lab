# Author: jayis1. SPDX-License-Identifier: MIT
"""BT1: declared braid words, not physical topology certificates."""
import csv
import os
import time
from collections import deque
FIELDS=('now state levels valid starts accepted rejected aborted gaps faults tx_errors '
        'age tainted selected outcome depth cancelled permutation positive negative '
        'word0 word1 word2 word3').split()
COUNTERS=('starts','accepted','rejected','aborted','gaps','tx_errors')
STATES=('WAIT','READY','ACTIVE','HALT')
TOKENS={1:'L+',2:'R+',3:'L-',4:'R-'}

def word(r):
    return [(r['word'+str(i//8)]>>(4*(i%8)))&15 for i in range(r['accepted'])]

def analyse(codes):
    # Independent signed-integer reduction, not firmware's modulo inverse test.
    stack=[]; strands=[0,1,2]; positive=negative=0
    for code in codes:
        if code not in (1,2,3,4): raise ValueError('crossing code')
        generator=code if code<=2 else -(code-2)
        if stack and stack[-1]==-generator: stack.pop()
        else: stack.append(generator)
        at=abs(generator)-1
        strands[at:at+2]=reversed(strands[at:at+2])
        if generator>0: positive+=1
        else: negative+=1
    return stack,sum(v*3**i for i,v in enumerate(strands)),positive,negative

def validate(r):
    if set(r)!=set(FIELDS) or any(type(v) is not int or not 0<=v<=0xffffffff for v in r.values()):
        raise ValueError('unsigned field contract')
    if r['state']>3 or r['levels']>15 or r['valid']>15 or r['faults']>63:
        raise ValueError('state or masks')
    if bool(r['faults'])!=(r['state']==3): raise ValueError('halt/fault mismatch')
    if bool(r['gaps'])!=bool(r['faults']&8): raise ValueError('gap without permanent halt')
    if r['accepted']>32 or r['depth']>32 or r['tainted']>1 or r['selected']>3:
        raise ValueError('word bounds')
    if r['starts']!=r['accepted']+r['rejected']+r['aborted']+(r['state']==2):
        raise ValueError('stroke conservation')
    if r['age']>10000 or r['outcome'] not in (0,4,8,12): raise ValueError('press bounds')
    if r['state']==2:
        if r['valid']!=15 or not r['outcome'] or r['age']>=10000: raise ValueError('active state')
        if not r['tainted'] and (r['selected'] not in (1,2) or r['outcome']==12):
            raise ValueError('untainted invalid stroke')
    if r['state']==1 and (r['valid']!=15 or r['levels']&12): raise ValueError('ready state')
    for i in range(r['accepted'],32):
        if (r['word'+str(i//8)]>>(4*(i%8)))&15: raise ValueError('nonzero unused word')
    stack,p,pos,neg=analyse(word(r))
    if (r['depth'],r['permutation'],r['positive'],r['negative'])!=(len(stack),p,pos,neg):
        raise ValueError('word/summary disagreement')
    if r['accepted']!=r['depth']+2*r['cancelled']: raise ValueError('reduction conservation')

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
                if word(r)[:old['accepted']]!=word(old): raise ValueError('immutable prefix changed')
                added=r['accepted']-old['accepted']
                starts=r['starts']-old['starts']
                # First start may qualify next tick; each subsequent start
                # needs five release samples and five new press samples.
                if starts and dt<1+10*(starts-1): raise ValueError('stroke rate')
                if added>starts+(old['state']==2): raise ValueError('crossing without start')
                if added:
                    minimum=1 if old['state']==2 else 6
                    if dt<minimum+10*(added-1): raise ValueError('completion rate')
                if old['state']==2 and starts==0:
                    if r['state']==2:
                        if r['age']-old['age']!=dt: raise ValueError('active age')
                        if any(r[k]!=old[k] for k in ('selected','outcome')) or r['tainted']<old['tainted']:
                            raise ValueError('active identity changed')
                    if added:
                        if not old['age']<r['age']<10000 or r['age']-old['age']>dt:
                            raise ValueError('completion deadline or age')
                        if old['tainted'] or added!=1 or old['selected'] not in (1,2) or old['outcome'] not in (4,8):
                            raise ValueError('tainted completion')
                        code=old['selected']+(2 if old['outcome']==8 else 0)
                        if word(r)[-1]!=code: raise ValueError('completion identity')
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
            writer=csv.writer(handle)
            writer.writerow(['author','jayis1','provenance',self.provenance,'label',"'"+label,
                             'connected',self.connected,'fresh',self.fresh(),'parser_errors',self.errors,
                             'warning',"'"+self.warning,'received',self.received,'retained',len(self.rows)])
            writer.writerow(FIELDS)
            for r in self.rows: writer.writerow([r[k] for k in FIELDS])
