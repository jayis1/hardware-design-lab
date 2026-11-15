# Author: jayis1. SPDX-License-Identifier: MIT
"""Strict FB1 contract and session evidence. No dependency outside Python stdlib."""
import csv
import time
from collections import deque
FIELDS=['sequence','now','fault','raw','ref_valid','dut_valid','band','dut',
        'direction','edges']+[f'{side}{i}' for side in ('open','close') for i in range(5)]+[
        'noise','invalid_ticks','skips','reversals','gaps','tx_errors']
BINS=FIELDS[10:20]
COUNTERS=['sequence','edges']+BINS+FIELDS[20:]
LIMIT=1_000_000_000

def validate(r):
    if set(r)!=set(FIELDS) or any(type(v) is not int or not 0<=v<=0xffffffff for v in r.values()):
        raise ValueError('invalid schema or unsigned range')
    if any(r[k]>LIMIT for k in COUNTERS): raise ValueError('counter range')
    for k,maximum in [('fault',15),('raw',15),('ref_valid',1),('dut_valid',1),
                      ('band',4),('dut',1),('direction',2)]:
        if r[k]>maximum: raise ValueError('range: '+k)
    if sum(r[k] for k in BINS)!=r['edges']: raise ValueError('edge conservation')
    opens=sum(r[f'open{i}'] for i in range(5))
    closes=sum(r[f'close{i}'] for i in range(5))
    if abs(opens-closes)>1: raise ValueError('nonalternating edges')
    if closes>opens and r['dut']!=1: raise ValueError('closing endpoint')
    if opens>closes and r['dut']!=0: raise ValueError('opening endpoint')
    if not r['dut_valid'] and r['edges']: raise ValueError('edges before baseline')
    if not r['ref_valid'] and r['band']!=4: raise ValueError('band before baseline')
    if r['fault'] and r['direction']: raise ValueError('halted direction')
    if r['gaps'] and not r['fault']&4: raise ValueError('gap without halt')
    if r['sequence']<5 and (r['ref_valid'] or r['dut_valid']):
        raise ValueError('qualification before five samples')
    if r['edges']>max(0,(r['sequence']-5)//5): raise ValueError('too many edges')
    if any(r[k]>r['sequence'] for k in ('invalid_ticks','noise','skips','reversals')):
        raise ValueError('diagnostic count exceeds acquired samples')

class Session:
    def __init__(self,source):
        self.source=source
        self.rows=deque(maxlen=10000)
        self.last=None
        self.blocked=''
        self.errors=0
        self.connected=False
        self.progress=None
        self.anchor_host=None
        self.elapsed=0
        self.advances=0

    def accept(self,r):
        if self.blocked: raise ValueError(self.blocked)
        try:
            validate(r)
            if self.last is not None:
                p=self.last
                dt=(r['now']-p['now'])&0xffffffff
                if dt==0:
                    if r!=p: raise ValueError('contradictory duplicate')
                    return
                if dt>0x7fffffff: raise ValueError('time rollback; reconnect required')
                if any(r[k]<p[k] for k in COUNTERS): raise ValueError('counter rollback')
                if r['fault']&p['fault']!=p['fault']: raise ValueError('fault cleared')
                if p['ref_valid']>r['ref_valid'] or p['dut_valid']>r['dut_valid']:
                    raise ValueError('qualification vanished')
                ds=r['sequence']-p['sequence']
                if ds>dt: raise ValueError('acquisition exceeds elapsed time')
                if not r['fault'] and ds!=dt: raise ValueError('missing acquisition without halt')
                if ds==0:
                    no_sample=['raw','ref_valid','dut_valid','band','dut','edges']+BINS+FIELDS[20:24]
                    if any(r[k]!=p[k] for k in no_sample):
                        raise ValueError('evidence changed without acquisition')
                if p['fault']:
                    frozen=['sequence','raw','ref_valid','dut_valid','band','dut','direction','edges']+BINS+FIELDS[20:25]
                    if any(r[k]!=p[k] for k in frozen): raise ValueError('halted evidence changed')
                # Every newly qualified binary edge alternates, regardless of band.
                opens=sum(r[f'open{i}']-p[f'open{i}'] for i in range(5))
                closes=sum(r[f'close{i}']-p[f'close{i}'] for i in range(5))
                if p['dut_valid'] and closes-opens!=r['dut']-p['dut']:
                    raise ValueError('edge polarity mismatch')
                if opens+closes>(ds+4)//5: raise ValueError('edges exceed acquired opportunity')
                if any(r[k]-p[k]>ds for k in ('invalid_ticks','noise','skips','reversals')):
                    raise ValueError('diagnostic growth exceeds acquired samples')
                self.elapsed+=dt
                self.advances+=1
            host=time.monotonic()
            if self.source=='live':
                if self.anchor_host is None: self.anchor_host=host
                elif abs((host-self.anchor_host)*1000-self.elapsed)>2000:
                    raise ValueError('backlog or clock drift; reconnect required')
                if self.progress is not None and host-self.progress>2:
                    raise ValueError('reception pause; reconnect required')
            self.progress=host
            self.last=dict(r)
            self.rows.append(dict(r))
        except ValueError as exc:
            self.blocked=str(exc)
            raise

    def status(self):
        if self.blocked: return 'BLOCKED: '+self.blocked
        if self.source=='replay': return 'REPLAY — not live measurements'
        if not self.connected: return 'DISCONNECTED — retained evidence'
        if self.last and self.last['fault']: return 'HALTED — heartbeat is not acquisition'
        if self.advances<2: return 'WAITING for advancing telemetry'
        if self.progress is None or time.monotonic()-self.progress>1: return 'STALE — no recent progress'
        return 'LIVE — physical accuracy unverified'

    def export(self,path):
        # Exclusive creation avoids destroying an earlier laboratory record.
        with open(path,'x',newline='',encoding='utf8') as f:
            writer=csv.writer(f)
            writer.writerow(['author','jayis1','source',self.source,'status',self.status(),
                             'parser_errors',self.errors,'retained_rows',len(self.rows)])
            writer.writerow(FIELDS)
            for r in self.rows: writer.writerow([r[k] for k in FIELDS])
