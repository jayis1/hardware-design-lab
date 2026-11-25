# Author: jayis1. SPDX-License-Identifier: MIT
"""Pair Cover PC1 model. Sparse checks are necessary bounds, not history proof."""
from itertools import combinations
PAIRS = tuple(combinations(range(4), 2))
FIELDS = ('now samples gaps missed tx_errors raw hold credited seeded fault visits '
          'settings coverage qualified_ms unknown_ms suggestion').split()
LIMIT = 2**32 - 1
ALL = 0xffffff
MASK_PAIRS = tuple(sum(1 << (4*p + 2*((m>>a)&1) + ((m>>b)&1))
                      for p,(a,b) in enumerate(PAIRS)) for m in range(16))
def project(settings):
    result = 0
    for mask in range(16):
        if settings & (1 << mask): result |= MASK_PAIRS[mask]
    return result

def suggest(coverage, raw):
    candidates = [m for m in range(16) if MASK_PAIRS[m] & ~coverage]
    if not candidates: return 16
    return min(candidates, key=lambda m: ((m ^ raw).bit_count(),
               -(MASK_PAIRS[m] & ~coverage).bit_count(), m))

def require(condition, message):
    if not condition: raise ValueError(message)

def validate(r):
    require(set(r) == set(FIELDS), 'schema')
    require(all(type(v) is int and 0 <= v <= LIMIT for v in r.values()), 'uint32 range')
    require(r['raw'] < 16 and r['settings'] < 65536, 'mask range')
    require(r['hold'] <= 100 and r['fault'] <= 4, 'hold/fault range')
    require(r['credited'] in (0,1) and r['seeded'] in (0,1), 'boolean range')
    require(r['coverage'] == project(r['settings']), 'pair projection')
    require(r['credited'] == int(r['hold'] == 100), 'qualification endpoint')
    require(not r['credited'] or r['settings'] & (1 << r['raw']), 'missing active credit')
    require(r['visits'] >= r['settings'].bit_count(), 'settings without visits')
    require(bool(r['visits']) == bool(r['settings']), 'empty visits')
    require(r['gaps'] <= r['missed'], 'gap accounting')
    require(r['suggestion'] == suggest(r['coverage'], r['raw']), 'suggestion')
    if not r['seeded']:
        require(not any(r[f] for f in ('samples','hold','visits','qualified_ms','unknown_ms','gaps','missed')), 'unseeded evidence')
    else:
        require(r['samples'] >= 1, 'seed without sample')
        require(r['unknown_ms'] + r['qualified_ms'] + r['gaps'] == r['samples']-1, 'interval conservation')
        require(r['visits'] * 100 <= r['unknown_ms'], 'visit qualification budget')

def transition(a,b):
    dt = (b['now'] - a['now']) & LIMIT
    require(0 < dt < 2**31, 'timestamp rollback or contradictory duplicate')
    counters = ('samples','gaps','missed','tx_errors','visits','qualified_ms','unknown_ms')
    for key in counters: require(b[key] >= a[key], 'counter rollback: '+key)
    require(b['settings'] | a['settings'] == b['settings'], 'disappearing setting')
    require(not a['fault'] or b['fault'] == a['fault'], 'changed terminal cause')
    ds = b['samples'] - a['samples']
    dg = b['gaps'] - a['gaps']
    dv = b['visits'] - a['visits']
    require(ds + b['missed'] - a['missed'] <= dt, 'sampling faster than time')
    require(dg <= ds, 'gap without acquisition')
    new = b['settings'] & ~a['settings']
    require(new.bit_count() <= dv, 'new settings without visits')
    require(dv * 100 <= ds + a['hold'], 'insufficient qualification time')
    if not ds or a['fault']:
        for key in FIELDS:
            if key not in ('now','tx_errors','fault'):
                require(b[key] == a[key], 'frozen acquisition changed: '+key)
    elif not dg and a['seeded']:
        require(b['hold'] <= min(100,a['hold']+ds), 'hold growth')
        if a['raw'] != b['raw']:
            require(b['hold'] <= ds-1, 'changed word qualified too soon')
    if dv == 0:
        require(b['settings'] == a['settings'], 'credit without visit')
    if not b['fault']:
        require(ds + b['missed']-a['missed'] == dt, 'unreported live acquisition')

class Session:
    def __init__(self, provenance):
        self.provenance = provenance
        self.rows = []
        self.total = 0
        self.error = ''
    def accept(self,row):
        if self.error: raise ValueError(self.error)
        try:
            validate(row)
            if self.rows:
                if row == self.rows[-1]: return False
                transition(self.rows[-1],row)
        except ValueError as exc:
            self.error = str(exc)
            raise
        self.rows.append(dict(row))
        self.rows = self.rows[-2000:]
        self.total += 1
        return True
