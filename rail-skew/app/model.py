# Author: jayis1. SPDX-License-Identifier: MIT
"""RS1 contract, sparse evidence tracking and private exclusive CSV exports."""
from collections import deque
import csv, json, os, time
FIELDS=('time raw levels known state anchor starts opening closing aborted active_ms '
        'bin0 bin1 bin2 bin3 bin4 bin5 peak last_ms last_dir last_peak flags gaps tx').split()
BINS=[f'bin{i}' for i in range(6)]
COUNTERS=['starts','opening','closing','aborted','gaps','tx']+BINS
STATES=['WAIT','READY','ACTIVE','HALTED']
def require(condition,message):
    if not condition: raise ValueError(message)
def skew(levels):
    stages=[(0,1,-1,2)[(levels >> shift)&3] for shift in (0,2)]
    return abs(stages[0]-stages[1])
def validate(r):
    require(set(r)==set(FIELDS),'field names')
    require(all(type(v) is int and 0<=v<=0xffffffff for v in r.values()),'uint32')
    require(all(r[k]<=15 for k in ('raw','levels','known')),'input bits')
    require(r['known'] in (0,3,12,15),'bundled qualification')
    require(r['state']<4 and r['anchor']<=2 and r['last_dir']<=1,'state range')
    require(r['peak']<=2 and r['last_peak']<=2,'peak range')
    require(r['flags']<=31 and bool(r['flags'])==(r['state']==3),'fault state')
    require(not r['gaps'] or r['flags']&1,'gap flag')
    legal=(r['levels']&3)!=2 and (r['levels']>>2)!=2
    require(legal or r['flags']&2,'invalid rail code')
    require(r['starts']==r['opening']+r['closing']+r['aborted']+(r['state']==2),'traverse conservation')
    require(r['active_ms']<60000 and r['last_ms']<60000,'duration bound')
    require(r['state']==2 or (r['active_ms']==0 and r['peak']==0),'inactive duration/peak')
    require(bool(r['anchor'])==(r['state'] in (1,2)),'anchor state')
    if r['state']==1: require(r['levels']==(0 if r['anchor']==1 else 15),'ready endpoint')
    if r['state']==2:
        require(r['levels'] not in (0,15),'active endpoint')
        require(r['peak']>=skew(r['levels']),'peak below current skew')
    if r['state'] in (1,2): require(r['known']==15,'qualification')
    done=r['opening']+r['closing']
    require(bool(done)==bool(r['last_ms']),'last result')
    if done: require(r['closing' if r['last_dir'] else 'opening']>0,'last direction')
    else: require(not r['last_dir'] and not r['last_peak'],'result without completion')
    require(r['starts'] or not any(r[k] for k in BINS),'bins without traverse')

class Session:
    def __init__(self,source='none'):
        self.source=source
        self.connected=False
        self.rows=deque(maxlen=6000)
        self.errors=0
        self.warning=''
        self.progress=None
        self.first_host=None
        self.device_ms=0
        self.advances=0
        self.received=None
        self.dropped=0
    def fail(self,message):
        self.warning=message
        raise ValueError(message)
    def accept(self,row,now=None):
        if now is None: now=time.monotonic()
        if self.warning: raise ValueError(self.warning)
        try:
            validate(row)
            if self.rows:
                p=self.rows[-1]
                dt=(row['time']-p['time']) & 0xffffffff
                require(dt<0x80000000,'time rollback / reset suspected')
                require(not(p['flags'] & ~row['flags']),'fault disappearance')
                require(all(row[k]>=p[k] for k in COUNTERS),'counter rollback / reset suspected')
                if not dt:
                    require(row==p,'contradictory duplicate')
                    return
                require(row['known']|p['known']==row['known'],'qualification disappearance')
                if p['state']==3:
                    immutable=set(FIELDS)-{'time','raw','tx','flags'}
                    require(all(row[k]==p[k] for k in immutable),'halted evidence changed')
                growth=sum(row[k]-p[k] for k in BINS)
                require(growth<=dt,'exposure exceeds elapsed time')
                available=row['starts']-p['starts']+(p['state']==2)
                require(growth<=available*60000,'exposure without active traverse')
                done=row['opening']+row['closing']
                old_done=p['opening']+p['closing']
                if done==old_done:
                    require(all(row[k]==p[k] for k in ('last_ms','last_dir','last_peak')),
                            'last result changed without completion')
                if p['state']==row['state']==2 and row['starts']==p['starts']:
                    require(row['anchor']==p['anchor'] and row['peak']>=p['peak'],'active identity/peak')
                    require(row['active_ms']==p['active_ms']+dt,'active duration')
                if self.source=='live':
                    assert self.first_host is not None
                    require(self.received is not None and now-self.received<=1,'host reception pause; reconnect')
                    self.device_ms+=dt
                    require(abs((now-self.first_host)*1000-self.device_ms)<=750,'device/host progress mismatch; reconnect')
                self.advances+=1
            else:
                self.first_host=now
            self.received=now
            self.progress=now
            if len(self.rows)==self.rows.maxlen: self.dropped+=1
            self.rows.append(dict(row))
        except ValueError as exc:
            self.fail(str(exc))
    def fresh(self,now=None):
        if now is None: now=time.monotonic()
        return (self.source=='live' and self.connected and not self.warning
                and self.advances>0 and self.progress is not None
                and 0<=now-self.progress<=1)
    def export(self,path,now=None):
        if now is None: now=time.monotonic()
        metadata={'author':'jayis1','source':self.source,'connected':self.connected,
                  'fresh':self.fresh(now),'parser_errors':self.errors,'warning':self.warning,
                  'dropped_snapshots':self.dropped,'age_seconds':None if self.progress is None else now-self.progress,
                  'scope':'stage disagreement intervals, includes abandoned traverses; not displacement or force'}
        fd=os.open(path,os.O_WRONLY|os.O_CREAT|os.O_EXCL,0o600)
        with os.fdopen(fd,'w',newline='') as handle:
            writer=csv.writer(handle)
            writer.writerow(['metadata',json.dumps(metadata,sort_keys=True)])
            writer.writerow(FIELDS)
            for row in self.rows: writer.writerow([row[k] for k in FIELDS])
