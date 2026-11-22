# Author: jayis1. SPDX-License-Identifier: MIT
"""SV1 evidence validation and bounded local session store."""
import csv
import os
import time
FIELDS='sequence now phase fault raw qualified valid starts completed nested aborted inner_trips violations timeouts gaps tx_errors age maximum last last_age cycle_trips qualify limit'.split()
PHASES=['WAIT','READY','OUTWARD','EXTENDED','INNER_OUT','INNER_FAR','INNER_BACK','OUTER_BACK','HALT']
RESULTS=['none','outer only','nested','ordering rejected','endpoint overlap','sampling gap','timeout','terminal fault']
COUNTERS='sequence starts completed nested aborted inner_trips violations timeouts gaps tx_errors maximum'.split()
def active(r): return 2 <= r['phase'] <= 7

def validate(r):
    if set(r)!=set(FIELDS) or any(type(v) is not int or not 0<=v<=0xffffffff for v in r.values()):
        raise ValueError('unsigned field contract')
    if r['qualify']!=5 or r['limit']!=60000: raise ValueError('incompatible policy')
    if r['phase']>8 or r['fault']>7 or r['raw']>15 or r['qualified']>15 or r['valid']>1:
        raise ValueError('state range')
    if (r['phase']==8)!=(r['fault']!=0): raise ValueError('fault state')
    if r['starts']!=r['completed']+r['aborted']+active(r): raise ValueError('cycle conservation')
    if r['nested']>min(r['completed'],r['inner_trips']): raise ValueError('nested conservation')
    if r['timeouts']>r['aborted'] or r['last']>7: raise ValueError('outcome range')
    if r['age']>=60000 or r['last_age']>60000 or r['maximum']>=60000: raise ValueError('duration range')
    if not active(r) and (r['age'] or r['cycle_trips']): raise ValueError('inactive working evidence')
    if r['cycle_trips']>r['inner_trips']: raise ValueError('inner count')
    expected={1:5,2:4,3:6,4:2,5:10,6:2,7:4}
    if r['phase'] in expected and (not r['valid'] or r['qualified']!=expected[r['phase']]):
        raise ValueError('trajectory topology')

class Session:
    def __init__(self,source):
        self.source=source; self.rows=[]; self.errors=0; self.blocked=''; self.evicted=0
        self.anchor=None; self.progress=None; self.elapsed=0; self.advances=0
    def accept(self,row,host=None):
        host=time.monotonic() if host is None else host
        if self.blocked: raise ValueError(self.blocked)
        try:
            validate(row)
            if self.rows:
                old=self.rows[-1]
                if row==old: return
                dt=(row['now']-old['now']) & 0xffffffff
                if dt==0 or dt>=0x80000000: raise ValueError('time rollback or contradiction')
                if any(row[k]<old[k] for k in COUNTERS): raise ValueError('counter rollback')
                if row['fault'] & old['fault'] != old['fault']:
                    raise ValueError('sticky fault disappeared')
                ds=row['sequence']-old['sequence']
                if ds>dt: raise ValueError('acquisition exceeds elapsed time')
                for k in ('starts','completed','inner_trips','violations','timeouts','gaps'):
                    if row[k]-old[k]>ds: raise ValueError('event growth exceeds samples')
                # A terminal fault can abandon one old active cycle without a new sample.
                if row['aborted']-old['aborted']>ds+int(active(old) and row['phase']==8):
                    raise ValueError('abandonment growth')
                if row['nested']-old['nested']>row['completed']-old['completed']:
                    raise ValueError('nested outcome migration')
                if old['phase']==8:
                    if any(row[k]!=old[k] for k in FIELDS if k not in ('now','tx_errors','fault')):
                        raise ValueError('halted evidence changed')
                if active(row) and row['starts']==old['starts']:
                    if not active(old) or row['age']!=old['age']+ds:
                        raise ValueError('continuing cycle age')
                    if row['cycle_trips']!=old['cycle_trips']+row['inner_trips']-old['inner_trips']:
                        raise ValueError('continuing cycle trip migration')
                if row['completed']==old['completed'] and row['maximum']!=old['maximum']:
                    raise ValueError('maximum without completion')
                if row['completed']>old['completed'] and row['maximum']>max(old['maximum'],old['age']+ds):
                    raise ValueError('completed duration exceeds acquired time')
                if row['completed']+row['aborted']==old['completed']+old['aborted']:
                    if row['last']!=old['last'] or row['last_age']!=old['last_age']:
                        raise ValueError('result changed without outcome')
                elif row['last_age'] > old['age'] + ds:
                    raise ValueError('latest duration exceeds acquired time')
                if self.source=='live':
                    if host-self.progress>2: raise ValueError('reception pause; reconnect')
                    self.elapsed+=dt
                    if abs(self.elapsed/1000-(host-self.anchor))>2:
                        raise ValueError('host/device drift; reconnect')
                if ds:
                    self.progress=host; self.advances+=1
            else:
                self.anchor=host; self.progress=host
            self.rows.append(dict(row))
            if len(self.rows)>2000:
                self.rows.pop(0); self.evicted+=1
        except ValueError as exc:
            self.blocked=str(exc)
            raise
    def fresh(self,connected,host=None):
        host=time.monotonic() if host is None else host
        return bool(connected and self.source=='live' and not self.blocked and self.advances
                    and self.rows[-1]['phase']!=8 and host-self.progress<1)
    def export(self,path,connected):
        fd=os.open(path,os.O_WRONLY|os.O_CREAT|os.O_EXCL,0o600)
        with os.fdopen(fd,'w',newline='') as out:
            w=csv.writer(out)
            w.writerow(['author','source','connected','fresh','errors','blocked','evicted'])
            w.writerow(['jayis1',self.source,connected,self.fresh(connected),self.errors,self.blocked,self.evicted])
            w.writerow(FIELDS)
            for row in self.rows: w.writerow([row[k] for k in FIELDS])
