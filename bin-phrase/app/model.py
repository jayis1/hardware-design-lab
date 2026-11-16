# Author: jayis1. SPDX-License-Identifier: MIT
"""BQ1 endpoint checks; necessary sparse bounds, not hidden-history proof."""
import csv, os, time
FIELDS=('sequence now phase fault raw qualified valid starts completed aborted age '
        'last_age gaps tx_errors reason count last_count').split()+[
            f'{prefix}{i}' for prefix in ('word','last') for i in range(16)]
LIMIT=1000000000

def validate(r):
    if set(r)!=set(FIELDS) or any(type(v)!=int or v<0 or v>0xffffffff for v in r.values()):
        raise ValueError('record shape or unsigned range')
    if r['phase']>3 or r['fault']>15 or bool(r['fault'])!=(r['phase']==3):
        raise ValueError('phase/fault')
    for key in ('raw','qualified','valid'):
        if r[key]>15: raise ValueError('contact range')
    for key in ('sequence','starts','completed','aborted','gaps','tx_errors'):
        if r[key]>LIMIT: raise ValueError('counter range')
    if r['starts']!=r['completed']+r['aborted']+(r['phase']==2):
        raise ValueError('trial conservation')
    if r['age']>600000 or r['last_age']>=600000 or r['reason']>4:
        raise ValueError('age/reason')
    if r['phase']==2 and (r['age']>=600000 or not r['qualified']&8):
        raise ValueError('active without tray or beyond deadline')
    if not r['completed'] and (r['last_count'] or r['last_age']):
        raise ValueError('result without completion')
    for prefix,count in (('word',r['count']),('last',r['last_count'])):
        if count>16: raise ValueError('word capacity')
        for i in range(16):
            value=r[f'{prefix}{i}']
            if (i<count and not 1<=value<=7) or (i>=count and value):
                raise ValueError('word encoding')

def phrase(row, retained=False):
    prefix,count=('last','last_count') if retained else ('word','count')
    groups=[]
    for i in range(row[count]):
        letters=[c for bit,c in enumerate('ABC') if row[f'{prefix}{i}']&(1<<bit)]
        groups.append(letters[0] if len(letters)==1 else '{'+'+'.join(letters)+'}')
    return ' → '.join(groups) or '(no qualified openings)'

class Session:
    def __init__(self,source):
        if source not in ('live','replay'): raise ValueError('source')
        self.source=source; self.rows=[]; self.errors=0; self.blocked=''
        self.last_progress=None; self.first_host=None; self.device_elapsed=0
        self.last_sequence_progress=None
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
                for key in ('sequence','starts','completed','aborted','gaps','tx_errors'):
                    if row[key]<old[key]: raise ValueError('counter rollback')
                ds=row['sequence']-old['sequence']
                if ds>dt: raise ValueError('acquisition exceeds elapsed time')
                if row['starts']-old['starts']>ds or row['completed']-old['completed']>ds:
                    raise ValueError('trial growth exceeds samples')
                if old['fault'] & ~row['fault']: raise ValueError('fault cleared')
                if row['completed']==old['completed']:
                    if any(row[k]!=old[k] for k in ['last_age','last_count']+[f'last{i}' for i in range(16)]):
                        raise ValueError('retained result changed without completion')
                if old['phase']==3:
                    if any(row[k]!=old[k] for k in FIELDS if k not in ('now','fault','tx_errors')):
                        raise ValueError('halted evidence changed')
                if row['starts']==old['starts'] and old['phase']==2:
                    if row['count']<old['count'] or row['count']-old['count']>ds:
                        raise ValueError('word growth')
                    if any(row[f'word{i}']!=old[f'word{i}'] for i in range(old['count'])):
                        raise ValueError('word prefix mutated')
                if self.source=='live':
                    if self.last_progress is None or self.first_host is None:
                        raise ValueError('missing live time anchor')
                    if host-self.last_progress>2: raise ValueError('host pause: reconnect required')
                    self.device_elapsed+=dt
                    if abs(self.device_elapsed/1000-(host-self.first_host))>2:
                        raise ValueError('host/device progress drift: reconnect required')
                if ds: self.last_sequence_progress=host
            else:
                self.first_host=host; self.last_sequence_progress=host
            self.last_progress=host
            self.rows.append(dict(row)); self.rows=self.rows[-2000:]
        except ValueError as exc:
            self.blocked=str(exc)
            raise
    def fresh(self,connected,host=None):
        host=time.monotonic() if host is None else host
        return bool(connected and self.source=='live' and not self.blocked and len(self.rows)>1
                    and self.rows[-1]['phase']!=3 and self.last_sequence_progress is not None
                    and host-self.last_sequence_progress<1)
    def export(self,path,connected):
        # Exclusive creation and restrictive permissions; never overwrite recordings.
        fd=os.open(path,os.O_WRONLY|os.O_CREAT|os.O_EXCL,0o600)
        with os.fdopen(fd,'w',newline='') as handle:
            writer=csv.writer(handle)
            writer.writerow(['author','source','connected','fresh','errors','blocked'])
            writer.writerow(['jayis1',self.source,connected,self.fresh(connected),self.errors,self.blocked])
            writer.writerow(FIELDS)
            for row in self.rows: writer.writerow([row[k] for k in FIELDS])
