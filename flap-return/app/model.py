# Author: jayis1. SPDX-License-Identifier: MIT
"""FR1 necessary invariants; sparse telemetry is not a complete waveform."""
import time
FIELDS=['now','phase','fault','raw','mask','valid','age','baseline','tx_errors']
LANE=['first','last','closures','reopens','quiet']
FIELDS += [f'{key}{i}' for i in range(3) for key in LANE]
PHASES=['WAIT','READY','ACTIVE','DONE','HALT']
FAULTS=['none','sampling gap','capacity','configuration','watchdog','deadline','preclosed','reheld']
def require(condition,message):
    if not condition: raise ValueError(message)
def validate(r):
    require(set(r)==set(FIELDS),'schema')
    require(all(type(v) is int and 0<=v<=0xffffffff for v in r.values()),'integer range')
    require(r['phase']<=4 and r['fault']<=7,'state range')
    require((r['phase']==4)==bool(r['fault']),'fault state')
    require(all(r[k]<=15 for k in ('raw','mask','valid')),'contact range')
    age=r['age']; phase=r['phase']
    require(age<=10000 and r['baseline']<=250,'time range')
    if phase in (1,2,3): require(r['valid']==15 and r['baseline']==250,'baseline')
    if phase<=1: require(age==0,'pretrial age')
    if phase in (2,3): require(age<10000 and not r['mask']&8,'release state')
    for i in range(3):
        first,last,n,opens,quiet=(r[f'{k}{i}'] for k in LANE)
        require(n<=1000 and opens<=n and n-opens<=1,'edge conservation')
        require(quiet<=250,'settlement range')
        if not n: require(first==last==opens==quiet==0,'absent lane')
        else:
            require(5<=first<=last<=age,'closure times')
            require(n!=1 or first==last,'single closure')
            require(last-first>=(n-1)*10,'closure spacing')
            require(quiet<=age-last and (not quiet or n>opens),'quiet credit')
        if phase<=1: require(n==0,'pretrial evidence')
        if phase in (2,3): require(n-opens==((r['mask']>>i)&1),'occupancy')
        if phase==3: require(quiet==250,'incomplete settlement')
    if phase==2: require(any(r[f'quiet{i}']<250 for i in range(3)),'completed active')
    if r['fault']==5: require(age==10000,'timeout age')

class Session:
    def __init__(self,provenance):
        require(provenance in ('live','replay'),'provenance')
        self.provenance=provenance
        self.connected=provenance=='live'
        self.rows=[]; self.received=None; self.anchor=None
        self.elapsed=0; self.progress=0; self.errors=0; self.warning=''
    def accept(self,r,host=None):
        validate(r)
        host=time.monotonic() if host is None else host
        if self.warning: raise ValueError(self.warning)
        try: self._accept(r,host)
        except ValueError as exc:
            self.warning=str(exc); raise
    def _accept(self,r,host):
        elapsed=self.elapsed
        if self.rows:
            old=self.rows[-1]; delta=(r['now']-old['now'])&0xffffffff
            require(delta<0x80000000,'clock rollback; reconnect')
            if not delta:
                require(r==old,'contradictory duplicate'); return
            require(r['phase']>=old['phase'],'phase rollback')
            require(r['tx_errors']>=old['tx_errors'],'transport rollback')
            growth=r['age']-old['age']
            require(0<=growth<=delta,'trial age')
            if old['phase']==2 and r['phase']==2: require(growth==delta,'active clock')
            if old['phase'] in (3,4):
                require(all(r[k]==old[k] for k in FIELDS if k not in ('now','raw','tx_errors')),'terminal rewrite')
            for i in range(3):
                first,last,n,opens,quiet=(f'{k}{i}' for k in LANE)
                nc=r[n]-old[n]; no=r[opens]-old[opens]
                require(nc>=0 and no>=0,'edge rollback')
                require(nc+no<=(delta+4)//5,'edge rate')
                if old[n]: require(r[first]==old[first],'first closure changed')
                if not nc: require(r[last]==old[last],'last closure changed without edge')
                elif old['phase']==2:
                    require(r[last]>old['age'],'backdated closure')
                    if not old[n]: require(r[first]>old['age'],'backdated first')
                require(r[quiet]<=old[quiet]+growth,'settlement grew too fast')
            elapsed+=delta
            if self.provenance=='live':
                require(host-self.received<=1.5,'host pause; reconnect')
                require(abs((host-self.anchor)*1000-elapsed)<=500,'clock/backlog divergence; reconnect')
            self.progress+=1
        else: self.anchor=host
        self.elapsed=elapsed; self.received=host
        self.rows.append(r.copy()); self.rows=self.rows[-600:]
    def fresh(self,now=None):
        now=time.monotonic() if now is None else now
        return bool(self.connected and not self.warning and self.progress and
                    self.received is not None and now-self.received<=1.5)
    def events(self):
        if not self.rows: return []
        r=self.rows[-1]
        return [dict(lane='ABC'[i],first_ms=r[f'first{i}'] if r[f'closures{i}'] else 'absent',
                     last_ms=r[f'last{i}'] if r[f'closures{i}'] else 'absent',
                     closures=r[f'closures{i}'],reopens=r[f'reopens{i}'],
                     quiet_ms=r[f'quiet{i}']) for i in range(3)]
