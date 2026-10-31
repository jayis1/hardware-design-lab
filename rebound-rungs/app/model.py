# Author: jayis1. SPDX-License-Identifier: MIT
"""RR1 evidence validation and bounded, receive-only session model."""
from collections import deque
import csv
import os
import time

FIELDS = ('now state raw stable qualified faults age high downmask upmask events '
          'gaps tx_errors clear_ms down0 down1 down2 down3 up0 up1 up2 up3 end_ms').split()
STATES = ('waiting for clear', 'ready', 'descending', 'awaiting rebound',
          'ascending', 'complete', 'invalid')

def require(condition, message):
    if not condition:
        raise ValueError(message)

def validate(row):
    require(set(row) == set(FIELDS), 'fields')
    require(all(type(v) is int and 0 <= v <= 0xffffffff for v in row.values()), 'integer range')
    s=row['state']; dm=row['downmask']; um=row['upmask']; high=row['high']
    require(s <= 6 and row['raw'] <= 15 and row['stable'] <= 15, 'state/input')
    require(row['qualified'] <= 1 and row['faults'] <= 63 and high <= 3, 'flags')
    require(row['age'] <= 5000 and row['clear_ms'] <= 250 and row['gaps'] <= 1, 'bounds')
    if s in (2,3,4,5): require(row['age'] < 5000, 'expired trial')
    require((s == 6) == bool(row['faults']), 'fault state')
    require(bool(row['faults'] & 4) == bool(row['gaps']), 'gap flag')
    require(dm in (0,8,12,14,15) and um in (0,1,3,7,15), 'visit masks')
    require(not um or dm == 15, 'upward without descent')
    require((um == (1 << (high+1))-1) if um else high == 0, 'highest gate')
    expected=dm.bit_count()+um.bit_count()+int(s==5 and high<3)
    require(row['events']==expected, 'event conservation')
    last=None
    for prefix,mask,order in [('down',dm,range(3,-1,-1)),('up',um,range(4))]:
        for i in order:
            value=row[f'{prefix}{i}']
            if not mask & (1<<i):
                require(value == 0, 'absent timestamp')
            else:
                require(value <= row['age'] and (last is None or value >= last+8), 'event order')
                last=value
    require(row['down3']==0, 'start origin')
    if s in (0,1): require(not (dm or um or row['age'] or row['events']), 'idle evidence')
    if s==1: require(row['clear_ms']==250, 'unarmed ready')
    if s==2: require(dm in (8,12,14), 'descent state')
    if s==3: require(dm==15 and not um, 'turn state')
    if s==4: require(bool(um) and high<3, 'ascent state')
    if s==5:
        require(bool(um) and row['end_ms']==row['age'], 'result duration')
        if last is None: raise ValueError('missing result events')
        require(row['end_ms']==last if high==3 else row['end_ms']>=last+8, 'result boundary')
    else: require(row['end_ms']==0, 'premature result')

class Session:
    def __init__(self, provenance):
        require(provenance in ('live','replay'), 'provenance')
        self.provenance=provenance
        self.rows=deque(maxlen=2048)
        self.dropped=0
        self.errors=0
        self.connected=False
        self.blocked=False
        self.warning=''
        self.last_progress=None
        self.anchor=None
        self.device_elapsed=0
        self.advances=0
    def accept(self, row, host=None):
        if self.blocked: raise ValueError(self.warning)
        host=time.monotonic() if host is None else host
        try:
            validate(row)
            if self.rows:
                previous=self.rows[-1]
                delta=(row['now']-previous['now']) & 0xffffffff
                require(delta < 0x80000000, 'clock rollback; reconnect after reset')
                if not delta:
                    require(row==previous,'contradictory duplicate')
                    return
                require(row['state']>=previous['state'], 'phase rollback')
                for key in ('events','gaps','tx_errors'):
                    require(row[key]>=previous[key], 'counter rollback')
                for prefix in ('down','up'):
                    oldmask=previous[prefix+'mask']; newmask=row[prefix+'mask']
                    require((oldmask & newmask)==oldmask,'disappearing gate visit')
                    for i in range(4):
                        if oldmask & (1<<i):
                            require(row[f'{prefix}{i}']==previous[f'{prefix}{i}'], 'changed event time')
                        elif newmask & (1<<i) and previous['state'] in (2,3,4):
                            require(row[f'{prefix}{i}']>previous['age'], 'backdated event')
                if previous['state'] in (0,1) and row['events']:
                    require(row['age']<delta, 'trial predates idle snapshot')
                if previous['state'] in (2,3,4):
                    require(previous['age']<=row['age']<=previous['age']+delta, 'duration progression')
                    if row['state'] in (2,3,4):
                        require(row['age']==previous['age']+delta,'active duration mismatch')
                    if row['state']==5:
                        require(row['end_ms']>previous['age'], 'backdated completion')
                if previous['state'] in (5,6):
                    require(all(row[k]==previous[k] for k in FIELDS if k not in ('now','raw','tx_errors')),
                            'terminal evidence changed')
                if self.provenance=='live':
                    assert self.last_progress is not None and self.anchor is not None
                    require(host-self.last_progress<=1.5,'reception pause; reconnect required')
                    self.device_elapsed+=delta/1000
                    require(abs(self.device_elapsed-(host-self.anchor))<=0.5,
                            'host/device progress mismatch; reconnect required')
                self.advances+=1
            else:
                self.anchor=host
            if len(self.rows)==self.rows.maxlen: self.dropped+=1
            self.rows.append(dict(row))
            self.last_progress=host
        except ValueError as exc:
            self.blocked=True
            self.warning=str(exc)
            raise
    def fresh(self, host=None):
        host=time.monotonic() if host is None else host
        return (self.provenance=='live' and self.connected and not self.blocked and
                self.advances>0 and self.last_progress is not None and host-self.last_progress<=1.5)
    def result(self):
        if not self.rows: return 'No evidence'
        row=self.rows[-1]
        if row['state']==6: return f"Invalid trial; fault mask {row['faults']}"
        if row['state']!=5: return STATES[row['state']]
        rung=row['high']
        return ('Reached upper rung 3; upper bound unknown' if rung==3 else
                f'Rebound returned through rung {rung}; did not visit rung {rung+1}')
    def export(self, path, fixture):
        # Exclusive creation: never silently overwrite an earlier experiment.
        fd=os.open(path,os.O_WRONLY|os.O_CREAT|os.O_EXCL,0o600)
        with os.fdopen(fd,'w',newline='') as handle:
            writer=csv.writer(handle)
            writer.writerow(['author','jayis1'])
            writer.writerow(['provenance',self.provenance,'connected',int(self.connected),
                'fresh',int(self.fresh()),'blocked',int(self.blocked),'parser_errors',self.errors,
                'dropped_rows',self.dropped,'warning',self.warning])
            # Prefix user text to prevent spreadsheet formula interpretation.
            writer.writerow(['fixture',"'"+fixture])
            writer.writerow(FIELDS)
            for row in self.rows: writer.writerow([row[k] for k in FIELDS])
