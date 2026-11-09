# Author: jayis1. SPDX-License-Identifier: MIT
"""TC1 evidence contracts. Necessary sparse bounds are not a history proof."""
from collections import deque
import csv
import os
import time
FIELDS=('sequence now state fault raw qualified valid starts good bad mismatch reference '
        'minimum maximum total last last_ms reason steps teeth age direction tx_errors gaps').split()
STATES=('SEEK','ACTIVE','HALT')
REASONS=('none','reversal','quadrature jump','encoder edge count','index/tooth boundary',
         '120 s deadline','acquisition gap','platform fault')
LIMIT=1_000_000_000

def require(ok,message):
    if not ok: raise ValueError(message)

def validate(r):
    require(set(r)==set(FIELDS),'field names')
    require(all(type(v) is int and 0<=v<=0xffffffff for v in r.values()),'unsigned range')
    require(all(r[k]<=LIMIT for k in FIELDS if k!='now'),'bounded counters')
    require(r['state']<3 and r['fault']<16,'state/fault range')
    require((r['state']==2)==bool(r['fault']),'terminal fault')
    require(r['raw']<16 and r['qualified']<16 and r['valid']<8,'input range')
    require(r['state']!=1 or r['valid']==7,'active qualification')
    require(r['starts']==r['good']+r['bad']+int(r['state']==1),'trial conservation')
    require(r['starts']<=r['sequence'],'start rate')
    require(r['mismatch']<r['good'] if r['good'] else not r['mismatch'],'reference cannot mismatch itself')
    require(r['steps']<=97 and r['direction']<=2 and r['age']<=120000,'capture range')
    require(r['state']!=1 or (r['steps']<=96 and r['age']<120000),'active deadline/edges')
    require(bool(r['steps'])==bool(r['direction']),'direction anchor')
    require(r['teeth']<=r['age']//10,'pulse rate')
    require(r['total']<=r['sequence'],'sum rate')
    require(r['gaps']<=1 and bool(r['gaps'])==bool(r['fault']&4),'gap contract')
    require(r['reason']<8 and bool(r['reason'])==bool(r['bad']),'rejection reason')
    if not r['good']:
        require(not any(r[k] for k in ('reference','minimum','maximum','total','last','last_ms','mismatch')),'invented result')
    else:
        require(r['minimum']<=r['last']<=r['maximum']<=11999,'result range')
        require(r['minimum']<=r['reference']<=r['maximum'],'reference range')
        require(r['minimum']*r['good']<=r['total']<=r['maximum']*r['good'],'count aggregate')
        require(r['mismatch'] or r['minimum']==r['maximum']==r['reference'],'missing mismatch')
        require(480<=r['last_ms']<120000,'result duration')

def transition(a,b):
    dt=(b['now']-a['now'])&0xffffffff
    ds=b['sequence']-a['sequence']
    require(0<dt<0x80000000 and 0<=ds<=dt,'clock/acquisition reset')
    require(b['fault']&a['fault']==a['fault'],'lost sticky fault')
    for k in ('starts','good','bad','mismatch','total','tx_errors','gaps','maximum'):
        require(b[k]>=a[k],'regressed '+k)
    if a['state']==2:
        require(all(a[k]==b[k] for k in FIELDS if k not in ('now','fault','tx_errors')),'halted evidence changed')
        return
    if b['state']!=2: require(ds==dt,'missing acquisition')
    dg=b['good']-a['good']; db=b['bad']-a['bad']; dn=b['starts']-a['starts']
    require(dn<=ds,'impossible starts')
    require(dg+db<=dn+int(a['state']==1),'invented completions')
    if a['good']:
        require(b['reference']==a['reference'] and b['minimum']<=a['minimum'],'reference/minimum changed')
    require(b['mismatch']-a['mismatch']<=dg,'invented mismatch')
    if not dg:
        require(all(a[k]==b[k] for k in ('minimum','maximum','total','last','last_ms','reference')),'result changed without completion')
    else:
        budget=ds+(a['age'] if a['state']==1 else 0)
        require(b['last_ms']<=budget and 480*dg<=budget,'completion duration budget')
        require(b['total']-a['total']<=budget//10,'pulse budget')
        if dg==1:
            require(b['total']-a['total']==b['last'],'single completion sum')
            require(b['mismatch']-a['mismatch']==int(bool(a['good']) and b['last']!=a['reference']),'single mismatch')
            require(b['minimum']==(min(a['minimum'],b['last']) if a['good'] else b['last']),'minimum attribution')
            require(b['maximum']==max(a['maximum'],b['last']),'maximum attribution')
    if not db: require(b['reason']==a['reason'],'reason changed without rejection')
    if a['state']==1 and not dg and not db and not dn and b['state']==1:
        require(b['age']==a['age']+ds,'active age')
        require(0<=b['steps']-a['steps']<=(ds+4)//5,'active encoder rate')
        require(0<=b['teeth']-a['teeth']<=(ds+9)//10,'active pulse rate')
        require(not a['direction'] or b['direction']==a['direction'],'unrejected reversal')
    elif b['state']==1:
        require(b['age']<=ds,'new capture age')

class Session:
    def __init__(self,provenance):
        require(provenance in ('live','replay'),'provenance')
        self.provenance=provenance
        self.rows=deque(maxlen=512)
        self.connected=False
        self.errors=0
        self.warning=''
        self.first_host=None
        self.last_host=None
        self.device_elapsed=0
        self.advances=0
    def accept(self,row):
        try:
            require(not self.warning,'blocked session: reconnect or reload')
            validate(row)
            host=time.monotonic()
            if self.rows:
                previous=self.rows[-1]
                if row==previous: return
                transition(previous,row)
                dt=(row['now']-previous['now'])&0xffffffff
                if self.provenance=='live':
                    assert self.last_host is not None and self.first_host is not None
                    require(host-self.last_host<=2,'receive pause: reconnect')
                    elapsed=self.device_elapsed+dt/1000
                    require(abs(host-self.first_host-elapsed)<=2,'clock/backlog drift: reconnect')
                    self.device_elapsed=elapsed
                self.advances+=1
            else: self.first_host=host
            self.rows.append(dict(row))
            self.last_host=host
        except ValueError as exc:
            self.warning=str(exc)
            raise
    def fresh(self):
        return bool(self.provenance=='live' and self.connected and not self.warning and
                    self.rows and self.rows[-1]['state']!=2 and self.advances and
                    self.last_host is not None and time.monotonic()-self.last_host<=1)
    def export(self,path):
        fd=os.open(path,os.O_WRONLY|os.O_CREAT|os.O_EXCL,0o600)
        with os.fdopen(fd,'w',newline='') as out:
            writer=csv.writer(out)
            writer.writerow(['author','jayis1','provenance',self.provenance,'connected',self.connected,
                             'fresh',self.fresh(),'parser_errors',self.errors,'warning',self.warning])
            writer.writerow(FIELDS)
            for row in self.rows: writer.writerow([row[k] for k in FIELDS])
