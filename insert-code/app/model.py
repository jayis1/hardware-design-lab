# Author: jayis1. SPDX-License-Identifier: MIT
"""IC1 endpoint invariants and bounded live history, not hidden-history proof."""
from collections import deque
import time
FIELDS = ['now','samples','gaps','missed','tx_errors','raw','qualified','valid',
          'fault','state','identity','changed_here','invalid_here','starts',
          'completed','abandoned','unidentified','changed','invalid','age','latest_age'] + [
          f'seats{i}' for i in range(6)]
CUMULATIVE = ['samples','gaps','missed','tx_errors','starts','completed','abandoned',
              'unidentified','changed','invalid']+[f'seats{i}' for i in range(6)]
LABELS = ['1: AB / 0011','2: AC / 0101','3: BC / 0110',
          '4: AD / 1001','5: BD / 1010','6: CD / 1100']
STATES = ['WAIT EMPTY','READY','ACTIVE','HALTED']
def require(condition,message):
    if not condition: raise ValueError(message)
def validate(r):
    require(set(r)==set(FIELDS),'schema')
    require(all(type(v) is int and 0<=v<=0xffffffff for v in r.values()),'integer bounds')
    require(r['raw']<=15 and r['qualified']<=15,'word bounds')
    require(r['state']<=3 and r['fault']<=5 and r['identity']<=6,'status bounds')
    require(all(r[k]<=1 for k in ('valid','changed_here','invalid_here')),'boolean bounds')
    require((r['state']==3)==bool(r['fault']),'fault state')
    require(not r['fault'] or not r['valid'],'halted qualification')
    require(r['gaps']<=min(r['missed'],r['samples']),'gap bounds')
    active=r['state']==2
    require(r['starts']==r['completed']+r['abandoned']+active,'episode conservation')
    identified=sum(r[f'seats{i}'] for i in range(6))
    require(identified+r['unidentified']==r['starts'],'identity conservation')
    require(r['changed']<=identified and r['unidentified']<=r['invalid']<=r['starts'],'taint bounds')
    if active:
        require(r['valid'] and r['qualified'],'active qualification')
        codes=(3,5,6,9,10,12)
        current=codes.index(r['qualified'])+1 if r['qualified'] in codes else 0
        require(current or r['invalid_here'],'missing invalid taint')
        require(not current or not r['identity'] or current==r['identity'] or r['changed_here'],
                'missing substitution taint')
        if r['identity']: require(r[f"seats{r['identity']-1}"]>0,'unseen identity')
        else: require(r['unidentified'] and r['invalid_here'],'unknown admission')
        require(not r['changed_here'] or r['changed'] and r['identity'],'change evidence')
        require(not r['invalid_here'] or r['invalid'],'invalid evidence')
    else:
        require(not any(r[k] for k in ('age','identity','changed_here','invalid_here')),'inactive payload')
    if r['state']==1: require(r['valid'] and not r['qualified'],'ready baseline')
    require(r['completed'] or not r['latest_age'],'duration without completion')

class Session:
    def __init__(self,source):
        require(source in ('live','replay'),'source')
        self.source=source; self.connected=source=='live'
        self.rows=deque(maxlen=2000); self.error=''; self.parser_errors=0
        self.last_progress=None; self.anchor_host=None; self.elapsed_device=0; self.advances=0
    def accept(self,r,host=None):
        if self.error: raise ValueError(self.error)
        try:
            validate(r)
            now=time.monotonic() if host is None else host
            if self.rows:
                old=self.rows[-1]
                if old==r: return
                require(not old['fault'],'halted evidence changed')
                dt=(r['now']-old['now'])&0xffffffff
                ds=r['samples']-old['samples']
                require(0<dt<0x80000000 or dt==0 and r['fault'],'time regression')
                require(all(r[k]>=old[k] for k in CUMULATIVE),'counter rollback')
                require(0<=ds<=dt and ds+r['missed']-old['missed']==dt,'elapsed accounting')
                starts=r['starts']-old['starts']
                ends=r['completed']+r['abandoned']-old['completed']-old['abandoned']
                require(starts<=ds,'admission growth')
                require(r['changed']-old['changed']<=starts+(old['state']==2 and not old['changed_here']),
                        'substitution growth')
                require(r['invalid']-old['invalid']<=starts+(old['state']==2 and not old['invalid_here']),
                        'invalid growth')
                require(r['abandoned']-old['abandoned']<=r['gaps']-old['gaps']+bool(r['fault']),
                        'abandonment without loss')
                if r['completed']==old['completed']:
                    require(r['latest_age']==old['latest_age'],'latest result changed')
                else:
                    require(r['latest_age']<=old['age']+ds,'completed duration growth')
                if old['state']==2 and not ends:
                    require(r['state']==2 and r['identity']==old['identity'],'active identity changed')
                    require(r['age']==old['age']+ds,'active age')
                    require(r['changed_here']>=old['changed_here'] and r['invalid_here']>=old['invalid_here'],
                            'taint disappeared')
                if not ds:
                    require(all(r[k]==old[k] for k in CUMULATIVE if k not in ('tx_errors','abandoned')),
                            'evidence without acquisition')
                self.elapsed_device+=dt
                if ds: self.last_progress=now; self.advances+=1
                if self.source=='live':
                    assert self.anchor_host is not None
                    require(abs((now-self.anchor_host)-self.elapsed_device/1000)<=1,
                            'host/device progress divergence; reconnect')
            else: self.anchor_host=now
            self.rows.append(r.copy())
        except ValueError as exc:
            self.error=str(exc); raise
    def fresh(self,now=None):
        now=time.monotonic() if now is None else now
        return bool(self.source=='live' and self.connected and not self.error and self.rows
                    and not self.rows[-1]['fault'] and self.advances
                    and self.last_progress is not None and now-self.last_progress<=1)
