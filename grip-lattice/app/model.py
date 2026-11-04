# Author: jayis1. SPDX-License-Identifier: MIT
"""GL1 evidence model. Sparse checks are necessary bounds, not history proofs."""
import csv
import os
from collections import deque

FIELDS = ('now samples phase fault raw mask valid starts completed aborted age unknown '
          'baseline tx_errors').split() + [f'{prefix}{i}' for prefix in ('h','r','m') for i in range(8)]
TRIAL = ['age','unknown'] + FIELDS[14:]
PHASES = ('WAIT','READY','ACTIVE','DONE','HALT')
FAULTS = ('none','acquisition gap','counter exhaustion','configuration','watchdog','60 s deadline')
LABELS = ('none','thumb','fingers','thumb + fingers','palm','thumb + palm',
          'fingers + palm','thumb + fingers + palm')

def require(condition, message):
    if not condition: raise ValueError(message)

def validate(r):
    require(set(r)==set(FIELDS), 'field set')
    require(all(type(v) is int and 0<=v<=0xffffffff for v in r.values()), 'uint32 range')
    require(r['phase']<=4 and r['fault']<=5, 'state range')
    require((r['phase']==4)==bool(r['fault']), 'fault state')
    require(all(r[k]<=15 for k in ('raw','mask','valid')), 'input mask')
    require(r['age']<=60000 and r['unknown']<=r['age'] and r['baseline']<=250, 'time bounds')
    require(r['starts']==r['completed']+r['aborted']+(r['phase']==2), 'trial conservation')
    require(r['age']==r['unknown']+sum(r[f'h{i}'] for i in range(8)), 'duration conservation')
    require(r['starts'] or not r['age'], 'trial without start')
    require(r['phase']!=0 or not r['starts'], 'waiting after start')
    require(r['phase']!=3 or r['completed'], 'result without completion')
    require(r['phase']!=2 or (r['mask']&8 and r['age']<60000), 'active endpoint')
    require(r['phase']!=4 or not r['baseline'], 'halt baseline')
    for i in range(8):
        h,n,m=(r[f'{p}{i}'] for p in ('h','r','m'))
        require(n<=h and m<=h, 'bin bounds')
        require((h==0)==(n==0)==(m==0), 'empty bin')
        require(not h or m+n-1<=h<=m*n, 'run length bounds')

class Session:
    def __init__(self, source):
        self.source=source
        self.rows=deque(maxlen=2000)
        self.connected=False
        self.blocked=False
        self.warning=''
        self.errors=0
        self.last_host=None
        self.anchor_host=None
        self.progress=0
        self.device_elapsed=0

    def accept(self, row, host=None):
        require(not self.blocked, 'session blocked; reconnect or reload explicitly')
        try:
            validate(row)
            if self.rows:
                old=self.rows[-1]
                dt=(row['now']-old['now']) & 0xffffffff
                require(dt<0x80000000, 'device clock rollback')
                if not dt:
                    require(row==old, 'contradictory duplicate')
                    return
                for key in ('samples','starts','completed','aborted','tx_errors'):
                    require(row[key]>=old[key], f'{key} rollback')
                require(row['samples']-old['samples']<=dt, 'acquisition rate')
                if row['starts']==old['starts']:
                    if old['phase']!=2:
                        require(all(row[k]==old[k] for k in TRIAL), 'frozen result changed')
                    else:
                        growth=row['age']-old['age']
                        require(0<=growth<=dt, 'trial time growth')
                        if row['phase']==2:
                            require(growth==dt, 'continuing trial lost time')
                        for key in TRIAL:
                            require(row[key]>=old[key], f'{key} decreased within trial')
                        for i in range(8):
                            dh=row[f'h{i}']-old[f'h{i}']
                            require(row[f'r{i}']-old[f'r{i}']<=dh, 'runs without intervals')
                            require(row[f'm{i}']-old[f'm{i}']<=dh, 'maximum without intervals')
                            if not dh:
                                require(row[f'r{i}']==old[f'r{i}'] and row[f'm{i}']==old[f'm{i}'],
                                        'changed empty-growth bin')
                else:
                    require(old['phase']!=4, 'restart after halt')
                    require(row['age']<=dt, 'new trial backdated')
                    require(row['starts']-old['starts']<=1+dt//255, 'impossible trial rate')
                if old['phase']==4:
                    require(all(row[k]==old[k] for k in FIELDS if k not in ('now','tx_errors')),
                            'halt accounting changed')
                if self.source=='live' and host is not None:
                    require(self.last_host is None or host-self.last_host<=2, 'reception pause; reconnect')
                    self.device_elapsed+=dt
                    elapsed=(host-self.anchor_host)*1000
                    require(abs(self.device_elapsed-elapsed)<=1500, 'clock/backlog mismatch; reconnect')
                self.progress+=1
            elif host is not None:
                self.anchor_host=host
            self.rows.append(dict(row))
            if host is not None: self.last_host=host
        except ValueError as exc:
            self.blocked=True
            self.warning=str(exc)
            raise

    def fresh(self, now):
        return (self.source=='live' and self.connected and not self.blocked
                and self.progress>0 and self.last_host is not None
                and 0<=now-self.last_host<=2 and self.rows[-1]['phase']!=4)

    def export(self, path, now, note):
        fd=os.open(path,os.O_WRONLY|os.O_CREAT|os.O_EXCL,0o600)
        with os.fdopen(fd,'w',newline='') as stream:
            writer=csv.writer(stream)
            writer.writerow(['author','jayis1','source',self.source,'connected',self.connected,
                             'fresh',self.fresh(now),'blocked',self.blocked,'parser_errors',self.errors,
                             'warning',self.warning,'note',note,'retained_limit',2000])
            writer.writerow(FIELDS)
            for row in self.rows: writer.writerow([row[k] for k in FIELDS])
