# Author: jayis1. SPDX-License-Identifier: MIT
"""SH1 invariants and immutable trial history. All times are nominal ms."""
import time
FIELDS = ['now','phase','fault','raw','mask','valid','age','quiet','count','tx_errors']
FIELDS += [name for i in range(32) for name in (f't{i}',f'm{i}')]
PHASES = ['WAIT','READY','ACTIVE','DONE','HALT']
FAULTS = ['none','sampling gap','counter exhaustion','configuration','watchdog','deadline','log full']

def require(condition, message):
    if not condition: raise ValueError(message)

def validate(r):
    require(set(r)==set(FIELDS),'schema mismatch')
    require(all(type(v) is int and 0<=v<=0xffffffff for v in r.values()),'integer range')
    require(r['phase']<=4 and r['fault']<=6,'state range')
    require(all(r[k]<=15 for k in ('raw','mask','valid')),'contact range')
    require(r['age']<=60000 and r['quiet']<=250 and r['count']<=32,'trial range')
    require((r['phase']==4)==bool(r['fault']),'fault state')
    previous=0; stamp=0
    for i in range(32):
        at,mask=r[f't{i}'],r[f'm{i}']
        if i>=r['count']:
            require(at==mask==0,'unused event')
            continue
        require(mask<=15 and mask!=previous,'unchanged event')
        require(at<=r['age'] and at<60000,'event time')
        require(at==0 if i==0 else at>stamp,'event ordering')
        previous,stamp=mask,at
    if r['phase'] in (0,1):
        require(r['count']==r['age']==r['quiet']==0,'pretrial evidence')
    if r['phase'] in (2,3):
        require(r['count']>0 and r['valid']==15 and previous==r['mask'],'active evidence')
    if r['phase']==2:
        require(r['age']<60000 and r['quiet']<250,'expired active state')
    if r['phase']==3:
        require(r['age']<60000,'completion deadline')
        require(r['quiet']==250 and r['mask']==15 and r['age']>=stamp+250,'settlement')
    if r['quiet'] and r['phase']!=4: require(r['mask']==15,'quiet contact state')
    if not r['count']: require(r['age']==0,'age without trial')
    if r['fault']==5: require(r['age']==60000,'deadline fault')
    if r['fault']==6: require(r['count']==32,'capacity fault')

class Session:
    def __init__(self, provenance):
        require(provenance in ('live','replay'),'provenance')
        self.provenance=provenance
        self.connected=provenance=='live'
        self.rows=[]
        self.received=None
        self.anchor=None
        self.elapsed=0
        self.progress=0
        self.errors=0
        self.warning=''
    def accept(self, r, host=None):
        validate(r)
        host=time.monotonic() if host is None else host
        if self.warning: raise ValueError(self.warning)
        try:
            self._accept(r,host)
        except ValueError as exc:
            self.warning=str(exc)
            raise
    def _accept(self,r,host):
        elapsed=self.elapsed
        if self.rows:
            old=self.rows[-1]
            delta=(r['now']-old['now'])&0xffffffff
            require(delta<0x80000000,'timestamp rollback; reconnect required')
            if delta==0:
                require(r==old,'contradictory duplicate')
                return
            require(r['phase']>=old['phase'],'phase rollback')
            require(r['count']>=old['count'] and r['tx_errors']>=old['tx_errors'],'counter rollback')
            require(r['age']>=old['age'] and r['age']-old['age']<=delta,'age progression')
            require(r['quiet']<=old['quiet']+r['age']-old['age'],'settlement credit')
            for i in range(old['count']):
                require((r[f't{i}'],r[f'm{i}'])==(old[f't{i}'],old[f'm{i}']),'history rewrite')
            if old['phase'] in (3,4):
                require(all(r[k]==old[k] for k in FIELDS if k not in ('now','raw','tx_errors','phase','fault')),'terminal evidence changed')
                if old['phase']==4: require(r['fault']==old['fault'],'fault changed')
                elif r['phase']==4: require(r['fault']==2,'unexpected post-completion fault')
            if old['phase']==2:
                for i in range(old['count'],r['count']):
                    require(r[f't{i}']>old['age'],'backdated event')
                if r['phase']==2: require(r['age']-old['age']==delta,'active clock mismatch')
            require(r['count']-old['count']<=delta,'too many event groups')
            elapsed+=delta
            if self.provenance=='live':
                require(host-self.received<=1.5,'host pause; reconnect required')
                require(abs((host-self.anchor)*1000-elapsed)<=500,'clock/backlog divergence; reconnect required')
            self.progress+=1
        else:
            self.anchor=host
        self.elapsed=elapsed
        self.received=host
        self.rows.append(r.copy())
        self.rows=self.rows[-600:]
    def fresh(self,now=None):
        now=time.monotonic() if now is None else now
        return bool(self.connected and not self.warning and self.progress and self.received is not None and now-self.received<=1.5)
    def events(self):
        if not self.rows: return []
        r=self.rows[-1]; previous=0; result=[]
        labels=lambda bits:' '.join('ABCD'[i] for i in range(4) if bits&(1<<i))
        for i in range(r['count']):
            mask=r[f'm{i}']
            result.append(dict(group=i+1,ms=r[f't{i}'],closed=labels(mask&~previous),
                               opened=labels(previous&~mask),occupancy=labels(mask)))
            previous=mask
        return result
