# Author: jayis1. SPDX-License-Identifier: MIT
"""Sieve Shuttle evidence invariants and bounded local session store."""
import json
import os
import time
FIELDS=('now raw mask qualified state fault visits x y returns turns gaps tx_errors '
        'last pending clear_ms elapsed axis run peak_x peak_y').split()+[f'm{i}' for i in range(16)]
COUNTERS='visits x y returns turns gaps tx_errors peak_x peak_y'.split()+[f'm{i}' for i in range(16)]
STATES=['WAIT FOR CLEAR','ARMED','PROVISIONAL ZONE','HALTED']
ZONES=['West','East','North','South','none']
REASONS=['configuration','watchdog','counter capacity','sampling gap','ambiguous zones','visit timeout']
def reasons(value):
    return ', '.join(name for i,name in enumerate(REASONS) if value&(1<<i)) or 'none'
def validate(r):
    if set(r)!=set(FIELDS) or any(type(v) is not int or not 0<=v<=0xffffffff for v in r.values()):
        raise ValueError('field or integer range')
    if any(r[k]>15 for k in ('raw','mask','qualified')) or r['state']>3 or r['fault']>63:
        raise ValueError('state range')
    if (r['state']==3)!=bool(r['fault']): raise ValueError('halt disagreement')
    if any(r[k]>1000000000 for k in COUNTERS): raise ValueError('capacity')
    if r['last']>4 or r['pending']>4 or r['axis']>2: raise ValueError('zone / axis')
    if r['clear_ms']>100 or r['elapsed']>30000: raise ValueError('duration range')
    if (r['last']<4)!=bool(r['visits']): raise ValueError('anchor disagreement')
    if r['state']==2 and (r['pending']==4 or r['qualified']!=15): raise ValueError('capture')
    if r['state'] in (0,1) and r['pending']!=4: raise ValueError('idle pending')
    if r['state']==1 and r['qualified']!=15: raise ValueError('unqualified armed state')
    totals=dict(x=0,y=0,returns=0,turns=0)
    for a in range(4):
        for b in range(4):
            key='returns' if a==b else ('turns' if a//2!=b//2 else ('x' if a<2 else 'y'))
            totals[key]+=r[f'm{4*a+b}']
    if any(r[k]!=v for k,v in totals.items()): raise ValueError('matrix classification')
    if sum(totals.values())+bool(r['visits'])!=r['visits']: raise ValueError('visit conservation')
    for axis in ('x','y'):
        if r['peak_'+axis]>r[axis] or bool(r['peak_'+axis])!=bool(r[axis]): raise ValueError('peak')
    if (r['axis']==2)!=(r['run']==0): raise ValueError('run identity')
    if r['axis']<2:
        if r['last']//2!=r['axis'] or r['run']>r['peak_x' if r['axis']==0 else 'peak_y']:
            raise ValueError('run bounds')

class Session:
    def __init__(self,source):
        self.source=source
        self.connected=False
        self.blocked=False
        self.warning=''
        self.errors=0
        self.rows=[]
        self.first_host=None
        self.last_host=None
        self.progress_ms=0
        self.advances=0
    def reject(self,message):
        self.blocked=True
        self.warning=message
        raise ValueError(message)
    def accept(self,row,host=None):
        if self.blocked: raise ValueError(self.warning)
        validate(row)
        host=time.monotonic() if host is None else host
        if self.rows:
            prev=self.rows[-1]
            dt=(row['now']-prev['now'])&0xffffffff
            if not dt:
                if row!=prev: self.reject('contradictory duplicate')
                return
            if dt>=0x80000000: self.reject('timestamp rollback; reconnect')
            if any(row[k]<prev[k] for k in COUNTERS): self.reject('counter rollback')
            if row['fault']|prev['fault']!=row['fault']: self.reject('fault disappeared')
            if row['qualified']|prev['qualified']!=row['qualified']:
                self.reject('qualification disappeared')
            if prev['state']==3 and any(row[k]!=prev[k] for k in FIELDS if k not in ('now','raw','tx_errors')):
                self.reject('terminal evidence changed')
            visits=row['visits']-prev['visits']
            if visits>dt//100+1: self.reject('visit rate impossible')
            credit=prev['clear_ms'] if prev['state']==2 else 0
            if visits and dt<visits*100-credit: self.reject('insufficient clearing time')
            for axis in ('x','y'):
                if row['peak_'+axis]>prev['peak_'+axis]+row[axis]-prev[axis]:
                    self.reject('peak growth exceeds new traversals')
            if not visits and any(row[k]!=prev[k] for k in
                    ['last','axis','run','peak_x','peak_y','x','y','returns','turns']+[f'm{i}' for i in range(16)]):
                self.reject('accounting changed without visit')
            if visits==1:
                old,new=prev['last'],row['last']
                if prev['state']==2 and new!=prev['pending']:
                    self.reject('committed zone differs from pending zone')
                for i in range(16):
                    expected=int(old<4 and i==4*old+new)
                    if row[f'm{i}']-prev[f'm{i}']!=expected:
                        self.reject('impossible single-visit matrix transition')
                axis=new//2 if old<4 and old!=new and old//2==new//2 else 2
                run=(prev['run']+1 if prev['axis']==axis else 1) if axis<2 else 0
                if row['axis']!=axis or row['run']!=run: self.reject('impossible run transition')
                for a,name in enumerate(('peak_x','peak_y')):
                    if row[name]!=max(prev[name],run if a==axis else 0): self.reject('impossible peak')
            self.progress_ms+=dt
            if self.source=='live':
                assert self.last_host is not None and self.first_host is not None
                if host-self.last_host>1.5: self.reject('host reception pause; reconnect')
                if abs((host-self.first_host)-self.progress_ms/1000)>1:
                    self.reject('host/device progress disagreement; reconnect')
            self.advances+=1
        else:
            self.first_host=host
        self.last_host=host
        self.rows.append(dict(row))
        self.rows=self.rows[-2000:]
    def fresh(self,host=None):
        host=time.monotonic() if host is None else host
        return (self.source=='live' and self.connected and not self.blocked and self.advances>0
                and self.last_host is not None and host-self.last_host<=1)
    def export(self,path,note=''):
        document={'author':'jayis1','format':'sieve-shuttle-evidence-1','source':self.source,
                  'connected':self.connected,'fresh':bool(self.fresh()),'blocked':self.blocked,
                  'warning':self.warning,'parser_errors':self.errors,'note':note,
                  'snapshot_limit':2000,'snapshots':self.rows}
        fd=os.open(path,os.O_WRONLY|os.O_CREAT|os.O_EXCL,0o600)
        with os.fdopen(fd,'w') as handle:
            json.dump(document,handle,indent=2); handle.write('\n')
