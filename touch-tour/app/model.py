# Author: jayis1. SPDX-License-Identifier: MIT
"""TT1 validation and local session provenance; no network dependencies."""
from collections import deque
import json
import os
import time

FIELDS = ('now samples missed tx_errors fault raw qualified valid armed starts '
          'rounds aborted repeats discarded disruptions timeouts active mask age '
          'order last_age last_order visit_a visit_b visit_c visit_d').split()
COUNTERS = ('samples missed tx_errors starts rounds aborted repeats discarded '
            'disruptions timeouts visit_a visit_b visit_c visit_d').split()
VISITS = FIELDS[-4:]
MAX = 2**32-1

def require(condition, message):
    if not condition:
        raise ValueError(message)

def order_ids(order, count):
    return [(order >> (2*i)) & 3 for i in range(count)]

def valid_order(order, mask):
    count=mask.bit_count()
    ids=order_ids(order,count)
    return not order >> (2*count) and len(set(ids))==count and sum(1<<i for i in ids)==mask

def validate(r):
    require(set(r)==set(FIELDS),'incompatible field set')
    require(all(type(v) is int and 0<=v<=MAX for v in r.values()),'unsigned range')
    require(r['fault']<=31 and r['raw']<=15 and r['qualified']<=15,'input range')
    require(all(r[k]<=1 for k in ('valid','armed','active')) and r['mask']<15,'state range')
    require(not r['valid'] or (r['raw']==r['qualified'] and r['samples']>=6),'qualification')
    require(r['active']==bool(r['mask']) and r['age']<60000,'active state')
    require((r['active'] or not r['age']) and r['age']<=r['samples'],'age')
    require(not r['fault'] or not (r['active'] or r['armed']),'halted state')
    require(valid_order(r['order'],r['mask']),'partial order')
    require(r['starts']==r['rounds']+r['aborted']+r['active'],'round conservation')
    require(r['timeouts']<=r['aborted'],'timeout conservation')
    require(r['aborted']<=r['discarded']<=3*r['aborted'],'discard conservation')
    visits=sum(r[k] for k in VISITS)
    require(visits<=min(MAX,r['samples']//12),'visit rate')
    require(visits==4*r['rounds']+r['mask'].bit_count()+r['discarded']+r['repeats'],'visit conservation')
    for i,k in enumerate(VISITS):
        require(r[k]>=r['rounds']+((r['mask']>>i)&1),'per-pad coverage')
    if not r['rounds']:
        require(not r['last_age'] and not r['last_order'],'unearned result')
    else:
        require(valid_order(r['last_order'],15) and 36<=r['last_age']<60000
                and r['last_age']<=r['samples'],'completed result')

class Session:
    def __init__(self, provenance):
        self.provenance=provenance
        self.rows=deque(maxlen=1000)
        self.blocked=''
        self.connected=False
        self.errors=0
        self.fragments=0
        self.last_progress=None
        self.first_host=0.0
        self.device_elapsed=0
        self.advances=0
    def accept(self,r,host=None):
        if self.blocked:
            raise ValueError(self.blocked)
        try:
            validate(r)
            host=time.monotonic() if host is None else host
            if self.provenance=='live' and self.last_progress is not None:
                require(host-self.last_progress<=1.0,'acquisition stale; reconnect')
            if self.rows:
                p=self.rows[-1]
                dt=(r['now']-p['now']) & MAX
                require(dt<2**31,'timestamp rollback; reconnect')
                if not dt:
                    require(r==p,'contradictory duplicate')
                    return
                require(all(r[k]>=p[k] for k in COUNTERS),'counter rollback; reconnect')
                require(r['fault'] & p['fault']==p['fault'],'fault rollback')
                ds=r['samples']-p['samples']
                require(ds+r['missed']-p['missed']<=dt,'impossible acquisition rate')
                new_visits=sum(r[k]-p[k] for k in VISITS)
                require(new_visits<=(ds+11)//12,'impossible visit growth')
                require(r['starts']-p['starts']<=new_visits,'unearned start')
                if p['fault'] or not ds:
                    frozen=[k for k in FIELDS if k not in ('now','tx_errors','fault')]
                    # First terminal transition may abandon the pending round.
                    if not p['fault'] and r['fault']:
                        frozen=[k for k in frozen if k not in
                                ('aborted','discarded','active','mask','age','order','armed')]
                        require(r['aborted']==p['aborted']+p['active'] and
                                r['discarded']==p['discarded']+p['mask'].bit_count(),
                                'terminal abandonment')
                    require(all(r[k]==p[k] for k in frozen),'frozen accounting changed')
                if r['rounds']==p['rounds']:
                    require(r['last_age']==p['last_age'] and r['last_order']==p['last_order'],
                            'completed result changed')
                else:
                    require(r['last_age']<=p['age']+ds,'impossible completed age')
                if ds:
                    self.last_progress=host
                    self.advances+=1
                self.device_elapsed+=dt/1000
                if self.provenance=='live':
                    require(abs(self.device_elapsed-(host-self.first_host))<=0.75,
                            'clock/backlog drift; reconnect')
            else:
                self.first_host=host
                self.last_progress=host
            self.rows.append(dict(r))
        except ValueError as exc:
            self.blocked=str(exc)
            raise
    def fresh(self,now=None):
        now=time.monotonic() if now is None else now
        if self.provenance=='live' and self.last_progress is not None and now-self.last_progress>1:
            self.blocked=self.blocked or 'acquisition stale; reconnect'
        return bool(self.provenance=='live' and self.connected and self.advances and not self.blocked
                    and self.rows and not self.rows[-1]['fault'])
    def export(self,path):
        # Exclusive create and owner-only permissions; preserve even empty metadata.
        now=time.monotonic()
        live=self.fresh(now)
        content={'author':'jayis1','device':'touch-tour','protocol':'TT1',
                 'provenance':self.provenance,'connected':self.connected,
                 'live':live,'acquisition_age_seconds':None if self.last_progress is None
                 or self.provenance!='live' else max(0,now-self.last_progress),
                 'blocked':self.blocked,'parser_errors':self.errors,
                 'initial_fragments':self.fragments,'retained_limit':1000,
                 'rows':list(self.rows)}
        fd=os.open(path,os.O_WRONLY|os.O_CREAT|os.O_EXCL,0o600)
        with os.fdopen(fd,'w') as handle:
            json.dump(content,handle,indent=2)
            handle.write('\n')
