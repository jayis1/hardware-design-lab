# Author: jayis1. SPDX-License-Identifier: MIT
"""AY1 necessary endpoint and sparse-transition checks, not history proof."""
import csv
import os
import time
FIELDS=('sequence now phase fault raw qualified valid intervals unknown invalid '
        'episodes run maximum gaps tx_errors').split()+[f'bin{i}' for i in range(9)]
BINS=[f'bin{i}' for i in range(9)]
CUMULATIVE=['sequence','intervals','unknown','invalid','episodes','maximum','gaps','tx_errors']+BINS
LIMIT=1000000000

def cell(word):
    stages={0:0,1:1,3:2}
    a,b=stages.get(word&3),stages.get((word>>2)&3)
    return None if a is None or b is None else 3*a+b

def shared(r):
    return sum(r[f'bin{i}'] for i in (4,5,7,8))

def validate(r):
    if set(r)!=set(FIELDS) or any(type(v)!=int or not 0<=v<=0xffffffff for v in r.values()):
        raise ValueError('record shape or unsigned range')
    if r['phase'] not in (0,1,3) or r['fault']>15 or bool(r['fault'])!=(r['phase']==3):
        raise ValueError('phase/fault')
    if r['raw']>15 or r['qualified']>15 or r['valid']>3:
        raise ValueError('contact range')
    if r['phase']==1 and r['valid']!=3 or r['phase']==0 and r['valid']==3:
        raise ValueError('qualification phase')
    if any(r[k]>LIMIT for k in CUMULATIVE) or r['gaps']>1:
        raise ValueError('counter limit')
    if r['intervals']!=max(0,r['sequence']-1):
        raise ValueError('interval conservation')
    if sum(r[k] for k in BINS)+r['unknown']!=r['intervals']:
        raise ValueError('matrix conservation')
    if r['unknown']<min(r['intervals'],4) or r['invalid']>max(0,r['sequence']-4):
        raise ValueError('startup qualification budget')
    if r['invalid']>r['sequence'] or r['sequence']<5 and (r['valid'] or r['invalid']):
        raise ValueError('qualification before acquisition')
    total=shared(r)
    if not 0<=r['run']<=r['maximum']<=total or r['episodes']>total:
        raise ValueError('shared run bounds')
    if bool(total)!=bool(r['maximum']) or bool(total)!=bool(r['episodes']):
        raise ValueError('shared evidence without episodes')
    if total and r['maximum']+r['episodes']-1>total or total>r['maximum']*r['episodes']:
        raise ValueError('shared segment bounds')
    if r['run'] and (r['valid']!=3 or r['raw']!=r['qualified'] or cell(r['qualified']) not in (4,5,7,8)):
        raise ValueError('current run lacks clean shared cell')
    if r['phase']==3 and r['run']:
        raise ValueError('halted run')
    if bool(r['gaps'])!=bool(r['fault']&4):
        raise ValueError('gap fault mismatch')

def describe(r):
    current=cell(r['qualified'])
    if r['valid']!=3 or current is None or r['raw']!=r['qualified']:
        return 'Unknown / qualifying / forbidden code'
    return f'A stage {current//3}, B stage {current%3}'

class Session:
    def __init__(self,source):
        if source not in ('live','replay'): raise ValueError('source')
        self.source=source
        self.rows=[]
        self.errors=0
        self.blocked=''
        self.last_progress=None
        self.first_host=None
        self.last_sequence_progress=None
        self.device_elapsed=0
    def accept(self,row,host=None):
        if self.blocked: raise ValueError(self.blocked)
        try:
            validate(row)
            host=time.monotonic() if host is None else host
            if self.rows:
                old=self.rows[-1]
                dt=(row['now']-old['now'])&0xffffffff
                if dt>=0x80000000: raise ValueError('clock rollback')
                if not dt:
                    if row!=old: raise ValueError('contradictory duplicate')
                    return
                if any(row[k]<old[k] for k in CUMULATIVE):
                    raise ValueError('counter rollback')
                ds=row['sequence']-old['sequence']
                if ds>dt: raise ValueError('acquisition exceeds time')
                if old['sequence'] and row['phase']!=3 and ds!=dt:
                    raise ValueError('missing acquisition without halt')
                if old['fault'] & ~row['fault']: raise ValueError('fault cleared')
                if old['valid'] & ~row['valid']: raise ValueError('qualification disappeared')
                if row['invalid']-old['invalid']>ds: raise ValueError('invalid growth')
                delta_shared=shared(row)-shared(old)
                if row['episodes']-old['episodes']>delta_shared:
                    raise ValueError('episodes without shared intervals')
                if row['run']>old['run']+delta_shared:
                    raise ValueError('run exceeds acquisition')
                if ds and not delta_shared and row['run']:
                    raise ValueError('run survived without shared intervals')
                if row['maximum']>max(old['maximum'],old['run']+delta_shared):
                    raise ValueError('maximum exceeds possible run')
                if ds==dt==1 and row['phase']!=3:
                    # Adjacent observations leave no hidden interval to invent.
                    position=cell(row['qualified'])
                    attributable=(old['valid']==row['valid']==3 and position is not None
                                  and old['raw']==old['qualified']==row['qualified']==row['raw'])
                    for i in range(9):
                        expected=int(attributable and i==position)
                        if row[f'bin{i}']-old[f'bin{i}']!=expected:
                            raise ValueError('adjacent matrix attribution')
                    if row['unknown']-old['unknown']!=int(not attributable):
                        raise ValueError('adjacent unknown attribution')
                    exposed=attributable and position in (4,5,7,8)
                    expected_run=old['run']+1 if exposed else 0
                    if row['run']!=expected_run:
                        raise ValueError('adjacent shared run')
                    if row['episodes']-old['episodes']!=int(exposed and not old['run']):
                        raise ValueError('adjacent segment start')
                    if row['maximum']!=max(old['maximum'],expected_run):
                        raise ValueError('adjacent maximum')
                frozen=[k for k in FIELDS if k not in ('now','fault','tx_errors')]
                if old['phase']==3 and any(row[k]!=old[k] for k in frozen):
                    raise ValueError('halted evidence changed')
                if not ds:
                    # The first gap/fault may clear a run, never add evidence.
                    permitted=('now','fault','tx_errors','gaps','phase','run')
                    if any(row[k]!=old[k] for k in FIELDS if k not in permitted):
                        raise ValueError('evidence changed without acquisition')
                    if row['run']!=old['run'] and not (row['phase']==3 and row['run']==0):
                        raise ValueError('run changed without acquisition')
                if self.source=='live':
                    if self.last_progress is None or self.first_host is None:
                        raise ValueError('missing time anchor')
                    if host-self.last_progress>2:
                        raise ValueError('host pause: reconnect required')
                    self.device_elapsed+=dt
                    if abs(self.device_elapsed/1000-(host-self.first_host))>2:
                        raise ValueError('host/device drift: reconnect required')
                if ds: self.last_sequence_progress=host
            else:
                self.first_host=host
                self.last_sequence_progress=host
            self.last_progress=host
            self.rows.append(dict(row))
            self.rows=self.rows[-2000:]
        except ValueError as exc:
            self.blocked=str(exc)
            raise
    def fresh(self,connected,host=None):
        host=time.monotonic() if host is None else host
        return bool(connected and self.source=='live' and not self.blocked and len(self.rows)>1
                    and self.rows[-1]['phase']!=3 and self.last_sequence_progress is not None
                    and host-self.last_sequence_progress<1)
    def export(self,path,connected):
        fd=os.open(path,os.O_WRONLY|os.O_CREAT|os.O_EXCL,0o600)
        with os.fdopen(fd,'w',newline='') as handle:
            writer=csv.writer(handle)
            writer.writerow(['author','source','connected','fresh','errors','blocked'])
            writer.writerow(['jayis1',self.source,connected,self.fresh(connected),self.errors,self.blocked])
            writer.writerow(FIELDS)
            for row in self.rows:
                writer.writerow([row[k] for k in FIELDS])
