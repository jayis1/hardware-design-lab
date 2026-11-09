# Author: jayis1. SPDX-License-Identifier: MIT
"""SS1 snapshot contracts and bounded, provenance-preserving local sessions."""
from collections import deque
import csv
import os
import time
LIMIT=1_000_000_000
STATES=('WAIT','TRACK','HALT')
FIELDS=('sequence now state fault raw qualified valid sector unknown forward backward '
        'skipped reversals direction run tx_errors gaps').split()+[
            f'dwell{i}' for i in range(16)]+[f'longest{i}' for i in range(16)]
CUMULATIVE=['unknown','forward','backward','skipped','reversals','tx_errors','gaps']+FIELDS[17:]

def require(condition,message):
    if not condition: raise ValueError(message)

def decode(gray):
    gray^=gray>>1
    return gray^(gray>>2)

def validate(r):
    require(set(r)==set(FIELDS),'field names')
    require(all(type(v) is int and 0<=v<=0xffffffff for v in r.values()),'unsigned range')
    require(all(r[k]<=LIMIT for k in FIELDS if k!='now'),'counter range')
    require(r['state']<=2 and r['fault']<=15,'state/fault range')
    require((r['state']==2)==bool(r['fault']),'terminal fault contract')
    require(r['raw']<16 and r['qualified']<16 and r['sector']<16,'code range')
    require(r['valid']<=1 and r['direction']<=2,'flag range')
    require(r['state']!=1 or r['valid'],'tracking needs valid code')
    require(r['state']!=0 or not r['valid'],'waiting cannot be qualified')
    require(not r['valid'] or r['sequence']>=5,'qualification too early')
    require(not r['valid'] or r['sector']==decode(r['qualified']),'Gray/sector mismatch')
    require(r['gaps']<=1 and bool(r['gaps'])==bool(r['fault']&4),'gap fault mismatch')
    total=r['unknown']+sum(r[f'dwell{i}'] for i in range(16))
    require(total==max(0,r['sequence']-1),'interval partition')
    for i in range(16):
        dwell,peak=r[f'dwell{i}'],r[f'longest{i}']
        require(peak<=dwell and bool(peak)==bool(dwell),'run/dwell relation')
    require(r['run']<=r[f"longest{r['sector']}"],'run maximum')
    require(not r['run'] or (r['valid'] and r['raw']==r['qualified']),'run code')
    moves=r['forward']+r['backward']+r['skipped']
    require(moves<=r['sequence']//5,'movement rate')
    require(r['reversals']<=r['forward']+r['backward'],'reversal count')
    require(r['direction']!=1 or r['forward'],'forward anchor')
    require(r['direction']!=2 or r['backward'],'backward anchor')
    require(r['valid'] or (not moves and total==r['unknown'] and not r['run']),'unqualified evidence')

def transition(a,b):
    dt=(b['now']-a['now'])&0xffffffff
    require(0<dt<0x80000000,'clock reset or contradictory duplicate')
    ds=b['sequence']-a['sequence']
    require(0<=ds<=dt,'acquisition reset or impossible progress')
    require(b['fault']&a['fault']==a['fault'],'lost sticky fault')
    require(all(b[k]>=a[k] for k in CUMULATIVE),'historical evidence regressed')
    if a['state']==2:
        require(all(a[k]==b[k] for k in FIELDS if k not in ('now','fault','tx_errors')),'halted evidence changed')
        return
    require(not a['valid'] or b['valid'],'lost qualified anchor')
    if b['state']!=2: require(ds==dt,'missing acquired intervals')
    df=b['forward']-a['forward']; db=b['backward']-a['backward']; dk=b['skipped']-a['skipped']
    moves=df+db+dk
    require(moves<=(ds+4)//5,'sparse movement rate')
    require(b['reversals']-a['reversals']<=df+db,'invented reversals')
    if not moves and a['valid']:
        require(a['sector']==b['sector'] and a['direction']==b['direction'],'position without movement')
        for i in range(16):
            if i!=a['sector']: require(a[f'dwell{i}']==b[f'dwell{i}'],'dwell migrated without movement')
    if moves==1 and a['valid']:
        delta=(b['sector']-a['sector'])%16
        require((df==1 and delta==1) or (db==1 and delta==15) or
                (dk==1 and delta not in (0,1,15)),'single move classification')
        direction=1 if df else 2 if db else 0
        require(b['direction']==direction,'latest direction')
        reverse=int(bool(direction and a['direction'] and direction!=a['direction']))
        require(b['reversals']-a['reversals']==reverse,'single move reversal')
    for i in range(16):
        require(b[f'longest{i}']<=max(a[f'longest{i}'],(a['run'] if a['sector']==i else 0)+ds),
                'maximum run exceeds acquisition')
    if a['sector']==b['sector'] and not moves:
        require(b['run']<=a['run']+ds,'run grew too quickly')
    else: require(b['run']<=ds,'new sector run too long')

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
            require(not self.warning,'session blocked; reconnect or reload')
            validate(row)
            host=time.monotonic()
            if self.rows:
                previous=self.rows[-1]
                if row==previous: return
                transition(previous,row)
                dt=(row['now']-previous['now'])&0xffffffff
                if self.provenance=='live':
                    assert self.last_host is not None and self.first_host is not None
                    require(host-self.last_host<=2,'receive pause; reconnect')
                    elapsed=self.device_elapsed+dt/1000
                    require(abs((host-self.first_host)-elapsed)<=2,'clock/backlog drift; reconnect')
                    self.device_elapsed=elapsed
                self.advances+=1
            else: self.first_host=host
            self.last_host=host
            self.rows.append(dict(row))
        except ValueError as exc:
            self.warning=str(exc)
            raise
    def fresh(self):
        return bool(self.provenance=='live' and self.connected and not self.warning and
                    self.rows and self.rows[-1]['state']==1 and self.advances and
                    self.last_host is not None and time.monotonic()-self.last_host<=1)
    def export(self,path):
        # Exclusive create protects an existing notebook and rejects symlinks.
        fd=os.open(path,os.O_WRONLY|os.O_CREAT|os.O_EXCL,0o600)
        with os.fdopen(fd,'w',newline='') as out:
            writer=csv.writer(out)
            writer.writerow(['author','jayis1','provenance',self.provenance,'connected',self.connected,
                             'fresh',self.fresh(),'parser_errors',self.errors,'warning',self.warning])
            writer.writerow(FIELDS)
            for row in self.rows: writer.writerow([row[k] for k in FIELDS])
