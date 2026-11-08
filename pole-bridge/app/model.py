# Author: jayis1. SPDX-License-Identifier: MIT
"""PB1 validation: intervals are observations, never electrical ratings."""
from collections import deque
import csv
import os
import time
CAPTURE = ['origin','outcome','age'] + ['bin'+str(i) for i in range(8)] + ['unknown0','unknown1','mixed']
FIELDS = ('sequence now state fault raw qualified valid starts completed returned timeouts aborted tx_errors gaps anchor target settle'.split()
          + [p+x for p in ('work_','last_') for x in CAPTURE])
STATES = ('WAIT','READY','ACTIVE','HALT')
OUTCOMES = ('none','transfer','return','timeout')
COUNTERS = 'starts completed returned timeouts aborted tx_errors gaps'.split()

def require(condition,message):
    if not condition:
        raise ValueError(message)

def validate(r):
    require(set(r)==set(FIELDS),'field contract')
    require(all(type(v) is int and 0<=v<=0xffffffff for v in r.values()),'integer range')
    require(r['state'] in range(4) and r['fault'] in (0,1,2,4,8),'state/fault range')
    require((r['state']==3)==bool(r['fault']),'halt/fault mismatch')
    require(all(r[x]<=15 for x in ('raw','qualified','valid')),'contact range')
    require(r['gaps']==int(r['fault']==4),'gap mismatch')
    require(all(r[x]<=1000000 for x in COUNTERS),'counter limit')
    require(r['aborted']<=1 and (not r['aborted'] or r['state']==3),'abandonment')
    require(r['starts']==sum(r[x] for x in ('completed','returned','timeouts','aborted'))+(r['state']==2),'capture conservation')
    require((r['anchor'] in (5,10) and r['valid']==15) if r['state'] in (1,2) else not r['anchor'],'anchor')
    require(r['state']!=1 or r['qualified']==r['anchor'],'READY endpoint')
    require(r['target'] in (0,5,10) and r['settle']<=20,'settlement range')
    require(not r['settle'] or r['target']==r['raw']==r['qualified'],'settlement endpoint')
    require(r['state'] not in (1,3) or not (r['target'] or r['settle']),'inactive settlement')
    for prefix in ('work_','last_'):
        c={x:r[prefix+x] for x in CAPTURE}
        active=prefix=='work_' and r['state']==2
        require(c['origin'] in (0,5,10) and c['outcome'] in range(4) and c['age']<=5000,'capture range')
        require(bool(c['origin']) if active else bool(c['origin'])==bool(c['outcome']),'capture presence')
        if active:
            require(c['origin']==r['anchor'] and not c['outcome'] and c['age']<5000,'active capture')
        elif prefix=='work_':
            require(not c['origin'],'inactive working capture')
        require(c['origin'] or not (c['age'] or c['outcome'] or c['mixed']),'empty capture')
        if c['outcome']==3: require(c['age']==5000,'timeout deadline')
        if c['outcome'] in (1,2): require(20<=c['age']<5000,'completed age')
        for p in range(2):
            require(c['unknown'+str(p)]+sum(c['bin'+str(p*4+k)] for k in range(4))==c['age'],'duration conservation')
            require(c['mixed']<=c['bin'+str(p*4+1)]+c['bin'+str(p*4+2)],'mixed coverage')
            if c['outcome'] in (1,2):
                target=c['origin'] if c['outcome']==2 else 15-c['origin']
                require(c['bin'+str(p*4+((target>>(p*2))&3))]>=20,'final endpoint coverage')
    require(bool(r['last_outcome'])==bool(r['completed']+r['returned']+r['timeouts']),'result presence')
    if r['last_outcome']:
        require(r[('completed','returned','timeouts')[r['last_outcome']-1]]>0,'result count')

class Session:
    def __init__(self, provenance):
        require(provenance in ('live','replay'),'provenance')
        self.provenance=provenance
        self.rows=deque(maxlen=512)
        self.connected=False
        self.warning=''
        self.errors=0
        self.last_progress=None
        self.anchor_host=None
        self.device_elapsed=0
        self.progress=0
    def accept(self,row,host=None):
        if self.warning: raise ValueError(self.warning)
        try: return self._accept(row,time.monotonic() if host is None else host)
        except ValueError as exc:
            self.warning=str(exc)
            raise
    def _accept(self,r,host):
        validate(r)
        if self.rows:
            p=self.rows[-1]
            if r==p: return False
            dt=(r['now']-p['now'])&0xffffffff
            ds=r['sequence']-p['sequence']
            require(0<dt<0x80000000 and 0<=ds<=dt,'reset or contradictory duplicate')
            require(all(r[x]>=p[x] for x in COUNTERS),'counter rollback')
            if p['state']==3:
                require(all(r[x]==p[x] for x in FIELDS if x not in ('now','tx_errors')),'halt evidence changed')
            else:
                require(ds or r['state']==3,'clock without acquisition')
                if r['state']!=3: require(ds==dt,'unreported acquisition gap')
                starts=r['starts']-p['starts']
                ends=sum(r[x]-p[x] for x in ('completed','returned','timeouts'))
                require(starts<=ds and ends<=ds,'events without acquired samples')
                require(starts<=1+ds//20,'start rate')
                require(ends<=starts+(p['state']==2),'completion budget')
                if not ends and p['state'] in (1,2) and r['state'] in (1,2):
                    require(r['anchor']==p['anchor'],'anchor changed without completion')
                if not starts and not ends and p['state']==1:
                    require(r['state'] in (1,3),'READY regressed without capture')
                if not ends:
                    require(all(r['last_'+x]==p['last_'+x] for x in CAPTURE),'retained result changed')
                else:
                    outcome=r['last_outcome']
                    require(r[('completed','returned','timeouts')[outcome-1]]>p[('completed','returned','timeouts')[outcome-1]],'latest outcome not newly counted')
                    require(r['last_age']<=ds+(p['work_age'] if p['state']==2 else 0),'latest duration budget')
                    if not starts and p['state']==2:
                        require(r['last_origin']==p['work_origin'],'result origin changed')
                        for x in CAPTURE[2:]:
                            require(p['work_'+x]<=r['last_'+x]<=p['work_'+x]+ds,'result coverage budget')
                if r['state']==2:
                    if not starts:
                        require(p['state']==2 and not ends,'unstarted capture')
                        require(r['work_origin']==p['work_origin'] and r['work_age']==p['work_age']+ds,'active continuity')
                        for x in CAPTURE[3:]:
                            require(p['work_'+x]<=r['work_'+x]<=p['work_'+x]+ds,'working coverage budget')
                    else:
                        require(r['work_age']<ds,'new capture age')
                if r['target']==p['target'] and r['target']:
                    require(r['settle']<=p['settle']+ds,'settlement growth')
                else:
                    require(r['settle']<ds or not r['settle'],'new settlement age')
            self.device_elapsed+=dt
            if self.provenance=='live':
                require(abs((host-self.anchor_host)*1000-self.device_elapsed)<=2000,'host/device progress mismatch; reconnect')
            if ds:
                self.last_progress=host; self.progress+=1
        else:
            self.anchor_host=host; self.last_progress=host
        self.rows.append(dict(r))
        return True
    def fresh(self,host=None):
        host=time.monotonic() if host is None else host
        return bool(self.provenance=='live' and self.connected and not self.warning and self.progress and
                    self.rows and self.rows[-1]['state']!=3 and self.last_progress is not None and
                    0<=host-self.last_progress<=1)
    def export(self,path):
        fd=os.open(path,os.O_WRONLY|os.O_CREAT|os.O_EXCL,0o600)
        with os.fdopen(fd,'w',newline='') as handle:
            writer=csv.writer(handle)
            writer.writerow(['author','provenance','connected','fresh','parser_errors','warning'])
            writer.writerow(['jayis1',self.provenance,self.connected,self.fresh(),self.errors,self.warning])
            writer.writerow(FIELDS)
            writer.writerows([r[x] for x in FIELDS] for r in self.rows)
