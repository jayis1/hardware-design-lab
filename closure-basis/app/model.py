# Author: jayis1. SPDX-License-Identifier: MIT
"""Closure Basis CB1 model. Sparse checks are necessary bounds, not history proof."""
FIELDS = ('now samples gaps missed tx_errors raw hold credited seeded fault visits '
          'settings coverage qualified_ms unknown_ms suggestion supported rules').split()
LIMIT = 2**32 - 1
CELLS = tuple(1 << m for m in range(16))
def project(settings):
    result = 0
    for mask in range(16):
        if settings & (1 << mask): result |= CELLS[mask]
    return result

def suggest(coverage, raw):
    unknown = [i for i in range(16) if not coverage & (1<<i)]
    if not unknown: return 16
    return min(unknown, key=lambda i: ((i ^ raw).bit_count(), i))

def matching(settings, antecedent):
    return [w for w in range(16) if settings&(1<<w) and w&antecedent==antecedent]

def support(settings, antecedent):
    return len(matching(settings,antecedent))

def closure(settings, antecedent):
    words=matching(settings,antecedent)
    if not words: return 16
    result=15
    for word in words: result &= word
    return result

def witness(settings, antecedent, bit):
    return next((w for w in matching(settings,antecedent) if not w&(1<<bit)),None)

def supported(settings):
    return sum(1<<a for a in range(16) if support(settings,a))

def minimal_for(settings, bit):
    candidates=[a for a in range(16) if not a&(1<<bit) and
                closure(settings,a)!=16 and closure(settings,a)&(1<<bit)]
    return sum(1<<a for a in candidates if not any(b!=a and b&a==b for b in candidates))

def rules(settings):
    return sum(minimal_for(settings,bit).bit_count() for bit in range(4))

def mask_name(mask):
    return '+'.join(name for i,name in enumerate('ABCD') if mask&(1<<i)) or 'empty'

def require(condition, message):
    if not condition: raise ValueError(message)

def validate(r):
    require(set(r) == set(FIELDS), 'schema')
    require(all(type(v) is int and 0 <= v <= LIMIT for v in r.values()), 'uint32 range')
    require(r['raw'] < 16 and r['settings'] < 65536, 'mask range')
    require(r['hold'] <= 100 and r['fault'] <= 4, 'hold/fault range')
    require(r['credited'] in (0,1) and r['seeded'] in (0,1), 'boolean range')
    require(r['coverage'] == project(r['settings']), 'relation projection')
    require(r['credited'] == int(r['hold'] == 100), 'qualification endpoint')
    require(not r['credited'] or r['settings'] & (1 << r['raw']), 'missing active credit')
    require(r['visits'] >= r['settings'].bit_count(), 'settings without visits')
    require(bool(r['visits']) == bool(r['settings']), 'empty visits')
    require(r['gaps'] <= r['missed'], 'gap accounting')
    require(r['suggestion'] == suggest(r['coverage'], r['raw']), 'suggestion')
    require(r['supported'] == supported(r['settings']), 'supported antecedents')
    require(r['rules'] == rules(r['settings']), 'minimal rule count')
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
    if ds == 1 and a['seeded'] and not a['fault']:
        # An adjacent acquired endpoint has no hidden visit to another word.
        changed = a['raw'] != b['raw']
        hold = 0 if dg or changed else min(100,a['hold']+1)
        visit = int(not dg and not changed and a['hold']==99)
        settings = a['settings'] | ((1<<b['raw']) if visit else 0)
        qualified = int(not dg and not changed and a['hold']==100)
        require(b['hold']==hold and b['credited']==int(hold==100), 'exact qualification')
        require(dv==visit and b['settings']==settings, 'exact visit attribution')
        require(b['qualified_ms']-a['qualified_ms']==qualified, 'exact settled interval')
        require(b['unknown_ms']-a['unknown_ms']==(0 if dg else 1-qualified), 'exact unknown interval')
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
