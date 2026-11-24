# Author: jayis1. SPDX-License-Identifier: MIT
"""QC1 semantics. Bounds reject impossible endpoints, not prove hidden history."""
from collections import deque
import time
FIELDS = ['now','samples','intervals','gaps','missed','tx_errors','raw',
          'qualified','valid','fault','voter_known','compare_known'] + [
          f'{key}{i}' for i in range(5) for key in ('total','segments','peak','run')]
CUMULATIVE = ['samples','intervals','gaps','missed','tx_errors','voter_known','compare_known'] + [
             f'{key}{i}' for i in range(5) for key in ('total','segments','peak')]
LABELS = ['A minority','B minority','C minority','Majority closed / reference open',
          'Majority open / reference closed']

def require(condition, message):
    if not condition:
        raise ValueError(message)

def validate(r):
    require(set(r)==set(FIELDS), 'schema mismatch')
    require(all(type(v) is int and 0<=v<=0xffffffff for v in r.values()), 'integer bounds')
    require(max(r[k] for k in ('raw','qualified','valid'))<=15 and r['fault']<=5,'status bounds')
    require(not r['fault'] or r['valid']==0, 'halted qualification')
    require(r['intervals']+r['gaps']+bool(r['samples'])==r['samples'], 'sample conservation')
    require(r['gaps']<=r['missed'], 'missing interval count')
    require(r['compare_known']<=r['voter_known']<=r['intervals'], 'coverage bounds')
    for i in range(5):
        total,count,peak,run=(r[f'{k}{i}'] for k in ('total','segments','peak','run'))
        require(bool(total)==bool(count)==bool(peak), 'empty evidence')
        require(run<=peak<=total and count<=total, 'episode bounds')
        require(not r['fault'] or run==0, 'halted run')
        if run:
            mask=7 if i<3 else 15
            require(r['valid'] & mask == mask and (r['raw'] ^ r['qualified']) & mask == 0,
                    'unqualified run')
            majority=(r['raw'] & 7).bit_count()>=2
            active=(((r['raw']>>i)&1)!=majority) if i<3 else (
                majority and not r['raw']&8 if i==3 else not majority and bool(r['raw']&8))
            require(active, 'run in inactive bin')
        if count:
            require(peak+count-1<=total<=peak*count, 'episode conservation')
    require(sum(r[f'total{i}'] for i in range(3))<=r['voter_known'], 'minority coverage')
    require(r['total3']+r['total4']<=r['compare_known'], 'reference coverage')

class Session:
    def __init__(self, source):
        require(source in ('live','replay'),'source')
        self.source=source
        self.connected=source=='live'
        self.rows=deque(maxlen=2000)
        self.error=''
        self.parser_errors=0
        self.last_progress=None
        self.anchor_host=None
        self.elapsed_device=0
        self.advances=0
    def accept(self,r,host=None):
        if self.error:
            raise ValueError(self.error)
        try:
            validate(r)
            now=time.monotonic() if host is None else host
            if self.rows:
                old=self.rows[-1]
                if old==r:
                    return
                require(not old['fault'], 'halted evidence changed')
                dt=(r['now']-old['now'])&0xffffffff
                ds=r['samples']-old['samples']
                # A first terminal fault may keep the last accepted timestamp.
                require((0<dt<0x80000000) or (dt==0 and r['fault']), 'time rollback or conflicting duplicate')
                require(all(r[k]>=old[k] for k in CUMULATIVE),'counter rollback')
                require(0<=ds<=dt, 'acquisition exceeds elapsed time')
                require(ds+r['missed']-old['missed']==dt,'elapsed accounting')
                require(r['voter_known']-old['voter_known']<=ds,'coverage growth')
                require(r['compare_known']-old['compare_known']<=r['voter_known']-old['voter_known'], 'comparison growth')
                for i in range(5):
                    total=r[f'total{i}']-old[f'total{i}']
                    known='voter_known' if i<3 else 'compare_known'
                    require(total<=r[known]-old[known], 'credit growth')
                    require(r[f'segments{i}']-old[f'segments{i}']<=total,'segment growth')
                    require(r[f'peak{i}']<=old[f'peak{i}']+total,'peak growth')
                    if not total:
                        require(r[f'peak{i}']==old[f'peak{i}'], 'peak without credit')
                if not ds:
                    require(all(r[k]==old[k] for k in CUMULATIVE if k!='tx_errors'),'evidence without samples')
                self.elapsed_device+=dt
                if ds:
                    self.last_progress=now
                    self.advances+=1
                if self.source=='live':
                    # Fixed 1 s budget caps accumulated lag or clock jump;
                    # explicit reconnection is required after violation.
                    require(self.anchor_host is not None, 'missing live origin')
                    assert self.anchor_host is not None
                    require(abs((now-self.anchor_host)-self.elapsed_device/1000)<=1,
                            'host/device progress divergence; reconnect')
            else:
                self.anchor_host=now
            self.rows.append(r.copy())
        except ValueError as exc:
            self.error=str(exc)
            raise
    def fresh(self,now=None):
        now=time.monotonic() if now is None else now
        return bool(self.source=='live' and self.connected and not self.error and self.rows
                    and not self.rows[-1]['fault'] and self.advances
                    and self.last_progress is not None and now-self.last_progress<=1)
