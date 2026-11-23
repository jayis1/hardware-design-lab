# Author: jayis1. SPDX-License-Identifier: MIT
"""RW1 snapshot contract and bounded local evidence sessions."""
import csv
import os
from collections import deque

FIELDS = ('samples now state fault raw qualified valid starts good bad same opposite '
          'still minimum maximum total last last_ms last_input last_output steps output '
          'age input_dir output_dir reason quiet tx_errors').split()
COUNTERS = 'samples starts good bad same opposite still total tx_errors'.split()
FROZEN = [k for k in FIELDS if k not in ('now', 'tx_errors')]

def require(condition, message):
    if not condition:
        raise ValueError(message)

def validate(r):
    require(set(r) == set(FIELDS), 'fields')
    require(all(type(v) is int and 0 <= v <= 0xffffffff for v in r.values()), 'range')
    require(r['samples'] <= 1000000000 and r['state'] <= 2 and r['fault'] <= 31, 'state')
    require((r['state'] == 2) == bool(r['fault']), 'halt/fault')
    require(r['raw'] <= 15 and r['qualified'] <= 15 and r['valid'] <= 3, 'inputs')
    require(r['starts'] == r['good'] + r['bad'] + (r['state'] == 1), 'window conservation')
    require(r['same'] + r['opposite'] + r['still'] == r['good'], 'direction conservation')
    require(r['starts'] <= r['samples'] and 96*r['good'] <= r['samples'], 'sampling budget')
    require(r['total'] <= r['samples'] and r['output'] <= 4096 and r['steps'] < 96, 'counts')
    require(r['input_dir'] <= 2 and r['output_dir'] <= 2 and r['reason'] <= 5, 'direction')
    require(r['age'] < 120000 and r['quiet'] < 100, 'duration')
    require(bool(r['steps']) == bool(r['input_dir']) and
            bool(r['output']) == bool(r['output_dir']), 'pending direction')
    if r['state'] == 1:
        require(r['valid'] == 3 and not r['quiet'], 'active')
    else:
        require(not any(r[k] for k in ('steps','output','age','input_dir','output_dir')), 'idle')
    if not r['good']:
        require(not any(r[k] for k in ('minimum','maximum','total','last','last_ms',
                                       'last_input','last_output')), 'empty result')
        return
    require(r['minimum'] <= r['last'] <= r['maximum'] <= 4096, 'extrema')
    require(r['minimum']*r['good'] <= r['total'] <= r['maximum']*r['good'], 'sum')
    require(r['maximum']+r['minimum']*(r['good']-1) <= r['total'], 'attained maximum')
    require(1 <= r['last_input'] <= 2 and r['last_output'] <= 2, 'result direction')
    require(bool(r['last']) == bool(r['last_output']), 'result output')
    category = 'still' if not r['last_output'] else ('same' if r['last_input']==r['last_output'] else 'opposite')
    require(r[category] > 0, 'latest category absent')
    require(not r['still'] or not r['minimum'], 'zero observation missing from minimum')
    require(r['same']+r['opposite'] <= r['total'], 'nonzero observations without edges')
    require(r['total'] <= r['minimum']+r['maximum']*(r['good']-1), 'attained minimum')
    require(480 <= r['last_ms'] < 120000, 'result duration')

class Session:
    def __init__(self, provenance):
        self.provenance = provenance
        self.connected = False
        self.rows = deque(maxlen=512)
        self.errors = 0
        self.warning = ''
        self.blocked = False
        self.last_host = None
        self.first_host = None
        self.elapsed = 0
        self.advances = 0

    def accept(self, r, host=None):
        if self.blocked:
            return False
        try:
            validate(r)
            if self.rows:
                p = self.rows[-1]
                dt = (r['now'] - p['now']) & 0xffffffff
                if not dt:
                    require(r == p, 'contradictory duplicate')
                    return False
                require(dt < 0x80000000, 'clock regression; reconnect')
                for k in COUNTERS:
                    require(r[k] >= p[k], 'counter regression: '+k)
                ds = r['samples'] - p['samples']
                require(ds <= dt, 'acquisition faster than clock')
                if r['state'] != 2:
                    require(ds == dt, 'unreported acquisition loss')
                if p['state'] == 2:
                    require(all(r[k] == p[k] for k in FROZEN), 'halt mutation')
                dg = r['good'] - p['good']
                db = r['bad'] - p['bad']
                require(dg*96 <= p['steps'] + ds, 'too many completed input turns')
                require(r['total']-p['total'] <= p['output']+ds, 'output credit growth')
                require(r['starts']-p['starts'] <= ds, 'start growth')
                require(db <= ds + (r['state']==2 and p['state']==1), 'rejection growth')
                if not dg:
                    require(all(r[k]==p[k] for k in ('minimum','maximum','last','last_ms',
                            'last_input','last_output','total','same','opposite','still')), 'result mutation')
                else:
                    require(r['last_ms'] <= p['age']+ds, 'result duration exceeds acquisition')
                    if p['good']:
                        require(r['minimum'] <= p['minimum'] and r['maximum'] >= p['maximum'], 'extrema regression')
                    if dg == 1:
                        require(r['total']-p['total'] == r['last'], 'latest output credit')
                        category = 'still' if not r['last_output'] else ('same' if r['last_input']==r['last_output'] else 'opposite')
                        require(all(r[k]-p[k] == (k==category) for k in ('same','opposite','still')), 'latest category')
                        require(r['minimum'] == (min(p['minimum'],r['last']) if p['good'] else r['last']), 'minimum attribution')
                        require(r['maximum'] == max(p['maximum'],r['last']), 'maximum attribution')
                if host is not None:
                    require(self.last_host is None or host-self.last_host <= 1, 'receive pause; reconnect')
                    self.elapsed += dt
                    require(abs((host-self.first_host)*1000-self.elapsed) <= 1000, 'clock/backlog mismatch')
                if ds:
                    self.advances += 1
                elif r['state'] != 2:
                    require(False, 'acquisition stopped')
            elif host is not None:
                self.first_host = host
            self.rows.append(dict(r))
            if host is not None:
                self.last_host = host
            return True
        except ValueError as exc:
            self.warning = str(exc)
            self.blocked = True
            raise

    def status(self, host):
        if self.provenance == 'replay':
            return 'REPLAY - NOT LIVE'
        if not self.connected:
            return 'DISCONNECTED'
        if self.blocked:
            return 'BLOCKED - RECONNECT'
        if self.rows and self.rows[-1]['state'] == 2:
            return 'HALTED ACQUISITION'
        if self.last_host is not None and host-self.last_host > 1:
            self.blocked = True
            self.warning = 'stale acquisition; reconnect'
            return 'STALE - RECONNECT'
        return 'ADVANCING' if self.advances else 'WAITING FOR PROGRESS'

    def export(self, path, host):
        status = self.status(host)
        fd = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
        with os.fdopen(fd, 'w', newline='') as handle:
            writer = csv.writer(handle)
            writer.writerow(['author','provenance','connected','status','errors','warning'])
            writer.writerow(['jayis1',self.provenance,self.connected,status,self.errors,self.warning])
            writer.writerow(FIELDS)
            for row in self.rows:
                writer.writerow([row[k] for k in FIELDS])
