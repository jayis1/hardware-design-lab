# Author: jayis1. SPDX-License-Identifier: MIT
"""FR1 measurements and conservative sparse-history/freshness validation."""
import time
FIELDS = ['now','samples','missed','unknown','tx_errors','fault','raw','qualified',
          'valid','anchor','settle','seeds','diagonal','adjacent']
FIELDS += [f'dwell{i}' for i in range(6)]
FIELDS += [f'visits{i}' for i in range(4)]
FIELDS += [f'edges{i}' for i in range(16)]
CUMULATIVE = ['samples','missed','unknown','tx_errors','seeds','diagonal','adjacent'] + FIELDS[14:]
NAMES = ['NW','NE','SW','SE']

def support_class(mask):
    return {14:0,13:1,11:2,7:3,15:4}.get(mask,5)

def validate(r):
    if set(r) != set(FIELDS):
        raise ValueError('schema mismatch')
    if any(type(v) is not int or not 0 <= v <= 0xffffffff for v in r.values()):
        raise ValueError('not uint32')
    if r['raw']>15 or r['qualified']>15 or r['valid']>1 or r['fault']>31 or r['anchor']>4:
        raise ValueError('state range')
    kind=support_class(r['raw'])
    if r['valid'] and (r['raw']!=r['qualified'] or r['samples']<6):
        raise ValueError('qualification mismatch')
    if r['settle']>1000 or (r['settle'] and (not r['valid'] or r['raw']!=15)):
        raise ValueError('invalid settlement')
    if r['settle']==1000 and r['anchor']!=4:
        raise ValueError('settled anchor')
    if kind==5 and r['anchor']!=4:
        raise ValueError('unsupported anchor')
    if r['valid'] and kind<4 and r['anchor']!=kind:
        raise ValueError('qualified foot differs from anchor')
    if r['anchor']<4 and not r[f"visits{r['anchor']}"]:
        raise ValueError('unvisited anchor')
    if sum(r[f'dwell{i}'] for i in range(6))+r['unknown']!=r['samples']:
        raise ValueError('interval conservation')
    if r['settle']>r['dwell4'] or r['unknown']<min(r['samples'],6):
        raise ValueError('startup/settlement duration')
    diagonal=adjacent=0
    for i in range(4):
        incoming=sum(r[f'edges{j*4+i}'] for j in range(4))
        outgoing=sum(r[f'edges{i*4+j}'] for j in range(4))
        if incoming>r[f'visits{i}'] or outgoing+int(r['anchor']==i)>r[f'visits{i}']:
            raise ValueError('edge source/destination conservation')
        for j in range(4):
            count=r[f'edges{i*4+j}']
            if i==j and count:
                raise ValueError('self edge')
            if i^j==3: diagonal+=count
            else: adjacent+=count
    visits=sum(r[f'visits{i}'] for i in range(4))
    if diagonal!=r['diagonal'] or adjacent!=r['adjacent']:
        raise ValueError('edge classification')
    if visits!=diagonal+adjacent+r['seeds'] or visits>r['samples'] or (visits and not r['seeds']):
        raise ValueError('visit conservation')

def transition(old,r):
    dt=(r['now']-old['now'])&0xffffffff
    if dt==0:
        if r!=old: raise ValueError('contradictory duplicate')
        return 0,0
    if dt>=0x80000000: raise ValueError('device time rollback; reconnect')
    if any(r[k]<old[k] for k in CUMULATIVE): raise ValueError('counter rollback; reconnect')
    if old['fault'] & ~r['fault']: raise ValueError('fault disappeared')
    ds=r['samples']-old['samples']; dm=r['missed']-old['missed']
    if ds+dm>dt: raise ValueError('acquisition too fast')
    if old['samples'] and not r['fault'] and ds+dm!=dt:
        raise ValueError('unaccounted time')
    if old['fault'] or ds==0:
        if any(r[k]!=old[k] for k in FIELDS if k not in ('now','fault','tx_errors')):
            raise ValueError('frozen evidence changed')
    dv=sum(r[f'visits{i}']-old[f'visits{i}'] for i in range(4))
    if dv>ds: raise ValueError('visits without samples')
    for i in range(4):
        incoming=sum(r[f'edges{j*4+i}']-old[f'edges{j*4+i}'] for j in range(4))
        outgoing=sum(r[f'edges{i*4+j}']-old[f'edges{i*4+j}'] for j in range(4))
        change=r[f'visits{i}']-old[f'visits{i}']
        if incoming>change or outgoing+int(r['anchor']==i)>change+int(old['anchor']==i):
            raise ValueError('sparse edge credit migration')
    if r['settle']>min(1000,ds+(old['settle'] if old['raw']==r['raw'] else 0)):
        raise ValueError('settlement grew too fast')
    if ds==1 and dm==0:
        clean=bool(old['valid'] and r['valid'] and old['raw']==r['raw'])
        kind=support_class(r['raw'])
        for i in range(6):
            if r[f'dwell{i}']-old[f'dwell{i}']!=int(clean and kind==i):
                raise ValueError('single interval attribution')
        settle=min(1000,old['settle']+1) if clean and kind==4 else 0
        if r['settle']!=settle: raise ValueError('single interval settlement')
        anchor=old['anchor']
        if kind==5 or settle==1000: anchor=4
        visit=int(bool(r['valid']) and kind<4 and anchor!=kind)
        for i in range(4):
            if r[f'visits{i}']-old[f'visits{i}']!=int(visit and kind==i):
                raise ValueError('single visit attribution')
        for i in range(16):
            expected=int(visit and anchor<4 and i==anchor*4+kind)
            if r[f'edges{i}']-old[f'edges{i}']!=expected:
                raise ValueError('single edge attribution')
        if r['seeds']-old['seeds']!=int(visit and anchor==4):
            raise ValueError('single seed attribution')
        if visit: anchor=kind
        if r['anchor']!=anchor: raise ValueError('anchor transition')
    return dt,ds

class Session:
    def __init__(self,source):
        self.source=source
        self.rows=[]
        self.error=''
        self.first_host=self.last_host=self.last_advance=None
        self.elapsed=self.advances=0
    def accept(self,row,host=None):
        if self.error: raise ValueError(self.error)
        host=time.monotonic() if host is None else host
        try:
            validate(row)
            if self.rows:
                assert self.last_host is not None and self.first_host is not None
                dt,ds=transition(self.rows[-1],row)
                if not dt: return
                self.elapsed+=dt
                if self.source=='live':
                    if host-self.last_host>1 or abs(self.elapsed/1000-(host-self.first_host))>1:
                        raise ValueError('host/device progress diverged; reconnect')
                if ds:
                    self.advances+=1
                    self.last_advance=host
            else: self.first_host=host
            self.last_host=host
            self.rows.append(row.copy())
            if len(self.rows)>1000: del self.rows[:-1000]
        except ValueError as exc:
            self.error=str(exc)
            raise
    def fresh(self,host=None,connected=False):
        host=time.monotonic() if host is None else host
        return bool(connected and self.source=='live' and not self.error and self.rows
                    and not self.rows[-1]['fault'] and self.advances>=2
                    and self.first_host is not None
                    and self.last_advance is not None and host-self.last_advance<.5
                    and abs(self.elapsed/1000-(host-self.first_host))<=1)
