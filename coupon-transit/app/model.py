# Author: jayis1. SPDX-License-Identifier: MIT
"""CT1 receive-only evidence model. All timing values are nominal milliseconds."""
import csv
import os
import time
from collections import deque
HOLDER='state origin elapsed started delivered returned abandoned sum_ms max_ms last_ms last_kind ambiguities timeouts'.split()
FIELDS='now mask known gaps tx fault halted'.split()+[f'{p}_{k}' for p in ('a','b') for k in HOLDER]
COUNTERS=['gaps','tx']+[f'{p}_{k}' for p in ('a','b') for k in
    ('started','delivered','returned','abandoned','sum_ms','max_ms','ambiguities','timeouts')]
def require(condition,message):
    if not condition: raise ValueError(message)
def validate(r):
    require(set(r)==set(FIELDS),'schema')
    require(all(type(v) is int and 0<=v<=0xffffffff for v in r.values()),'integer bound')
    require(r['mask']<=15 and r['known']<=15 and r['fault']<=15 and r['halted']<=1,'mask/flag bound')
    require(bool(r['fault'])==bool(r['halted']),'fault state')
    for i,p in enumerate(('a','b')):
        h={k:r[f'{p}_{k}'] for k in HOLDER}
        require(h['state']<=2 and h['origin']<=2 and h['last_kind']<=2,'holder state')
        require(h['started']==h['delivered']+h['returned']+h['abandoned']+(h['state']==2),'conservation')
        require(h['state']!=0 or not(h['origin'] or h['elapsed']),'wait fields')
        require(h['state']==0 or h['origin'] in (1,2),'origin')
        require(h['state']==2 or h['elapsed']==0,'inactive elapsed')
        require(max(h['elapsed'],h['last_ms'],h['max_ms'])<5000,'deadline')
        require(h['timeouts']<=h['abandoned'],'timeouts')
        require(h['delivered'] or not(h['sum_ms'] or h['max_ms']),'empty aggregate')
        if h['delivered']:
            require(1<=h['max_ms']<=h['sum_ms'],'aggregate maximum')
            require(h['max_ms']+h['delivered']-1<=h['sum_ms']<=h['delivered']*h['max_ms'],'aggregate sum')
        if not h['last_kind']:
            require(not(h['last_ms'] or h['delivered'] or h['returned']),'absent last outcome')
        else: require(h['last_ms']>0,'last duration')
        if h['last_kind']==1: require(h['delivered'] and h['last_ms']<=h['max_ms'],'last delivery')
        if h['last_kind']==2: require(h['returned']>0,'last return')
        require(not r['halted'] or h['state']==0,'halted holder')
        if h['state']:
            require((r['known']>>(2*i))&3==3,'qualification')
            require((r['mask']>>(2*i))&3==(h['origin'] if h['state']==1 else 0),'dock mask')

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
                delta=(row['now']-old['now'])&0xffffffff
                if not delta:
                    require(row==old,'contradictory duplicate')
                    return
                require(delta<0x80000000,'device time rollback')
                require(all(row[k]>=old[k] for k in COUNTERS),'counter rollback')
                require(row['fault']|old['fault']==row['fault'],'fault disappeared')
                for p in ('a','b'):
                    get=lambda r,k:r[f'{p}_{k}']
                    deliveries=get(row,'delivered')-get(old,'delivered')
                    returns=get(row,'returned')-get(old,'returned')
                    starts=get(row,'started')-get(old,'started')
                    # One start may use a baseline established before this
                    # interval; every additional start needs a fresh 50 ms.
                    require(starts<=1+delta//50,'start/rearm rate')
                    if not starts and get(old,'state')==2 and get(row,'state')==2:
                        require(get(row,'origin')==get(old,'origin') and
                                get(row,'elapsed')==get(old,'elapsed')+delta,
                                'continuing transit changed')
                    if not deliveries:
                        require(all(get(row,k)==get(old,k) for k in ('sum_ms','max_ms')),'delivery aggregate changed')
                    if not deliveries+returns:
                        require(all(get(row,k)==get(old,k) for k in ('last_ms','last_kind')),'last outcome changed')
                    # Necessary bounds; a sparse stream cannot reconstruct every move.
                    sum_delta=get(row,'sum_ms')-get(old,'sum_ms')
                    require(deliveries<=sum_delta<=deliveries*4999,'new delivery duration')
                    credit=get(old,'elapsed') if get(old,'state')==2 else 0
                    require(sum_delta<=delta+credit,'delivery time budget')
                    if deliveries and not returns:
                        require(get(row,'last_kind')==1,'latest outcome must be delivery')
                    if returns and not deliveries:
                        require(get(row,'last_kind')==2,'latest outcome must be return')
                    if deliveries==1:
                        require(get(row,'max_ms')==max(get(old,'max_ms'),sum_delta),'new maximum')
                        if not returns:
                            require(get(row,'last_ms')==sum_delta,'single delivery duration')
                    if deliveries+returns:
                        require(get(row,'last_ms')<=delta+credit,'last outcome time budget')
                        if get(row,'last_kind')==2:
                            require(sum_delta+get(row,'last_ms')<=delta+credit,'return and delivery time budget')
                    if old['halted']:
                        require(all(get(row,k)==get(old,k) for k in HOLDER),'halted evidence changed')
                if old['halted']:
                    require(row['halted'] and row['gaps']==old['gaps'] and row['mask']==old['mask']
                            and row['known']==old['known'],'halted recovery')
                if self.source=='live':
                    assert self.last_host is not None and self.origin_host is not None
                    require(now-self.last_host<=1,'host pause; reconnect required')
                    elapsed=self.device_elapsed+delta
                    require(abs(elapsed/1000-(now-self.origin_host))<=.5,
                            'host/device drift or backlog; reconnect required')
                    self.device_elapsed=elapsed
                self.progress+=1
            else: self.origin_host=now
            if len(self.rows)==self.rows.maxlen: self.evicted+=1
            self.rows.append(dict(row)); self.last_host=now
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
    def export(self,path,now=None):
        now=time.monotonic() if now is None else now
        fd=os.open(path,os.O_WRONLY|os.O_CREAT|os.O_EXCL,0o600)
        with os.fdopen(fd,'w',newline='') as handle:
            writer=csv.writer(handle)
            writer.writerow(['author','jayis1','source',self.source,'connected',self.connected,
                'freshness',self.freshness(now),'age_s','' if self.last_host is None else now-self.last_host,
                'parser_errors',self.errors,'blocked',self.blocked,'evicted',self.evicted])
            writer.writerow(FIELDS)
            writer.writerows([[r[k] for k in FIELDS] for r in self.rows])
