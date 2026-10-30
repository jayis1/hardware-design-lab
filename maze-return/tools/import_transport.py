#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Import reviewed serial framing/session code and specialize the MR1 contract.
Source: punch-register/app/protocol.py at eab7097850e19c41425e605bf3cfe238e1015d74.
One-shot generation; the resulting app is standalone.
"""
from pathlib import Path
root=Path(__file__).resolve().parents[1]
source=(root.parent/'punch-register/app/protocol.py').read_text()
start=source.index('FIELDS =')
end=source.index('class Decoder:')
contract='''FIELDS = ('now raw mask qualified state fault visits added loops repeats erased gaps '
          'tx_errors depth route pending clear_ms elapsed').split()
COUNTERS = 'visits added loops repeats erased gaps tx_errors'.split()
STATES = ['WAIT FOR CLEAR', 'ARMED', 'PROVISIONAL VISIT', 'HALTED']
REASONS = ['configuration', 'watchdog', 'counter capacity', 'sampling gap',
           'ambiguous gates', 'visit timeout']

def reasons(mask):
    return ', '.join(name for bit,name in enumerate(REASONS) if mask & (1<<bit)) or 'none'

def route(row):
    return ' > '.join('ABCD'[(row['route']>>(2*i))&3] for i in range(row['depth'])) or '(empty)'

def validate(r):
    if set(r)!=set(FIELDS) or any(type(v) is not int or not 0<=v<=0xffffffff for v in r.values()):
        raise ValueError('field or integer range')
    if any(r[k]>15 for k in ('raw','mask','qualified')) or r['state']>3 or r['fault']>63:
        raise ValueError('state range')
    if any(r[k]>1000000000 for k in COUNTERS): raise ValueError('counter limit')
    if r['visits']!=r['added']+r['loops']+r['repeats']: raise ValueError('visit conservation')
    if r['depth']>4 or r['added']-r['erased']!=r['depth']: raise ValueError('route conservation')
    if not r['loops']<=r['erased']<=3*r['loops']: raise ValueError('erasure bounds')
    if bool(r['visits'])!=bool(r['depth']): raise ValueError('empty visited route')
    if r['route']>>(2*r['depth']): raise ValueError('unused route bits')
    ids=[(r['route']>>(2*i))&3 for i in range(r['depth'])]
    if len(set(ids))!=len(ids): raise ValueError('repeated route identity')
    if (r['state']==3)!=bool(r['fault']): raise ValueError('halt disagreement')
    if r['pending']>4 or r['clear_ms']>100 or r['elapsed']>30000: raise ValueError('capture range')
    if r['state']==2 and (r['pending']==4 or r['qualified']!=15): raise ValueError('capture state')
    if r['state'] in (0,1) and r['pending']!=4: raise ValueError('idle pending')

'''
text=(source[:start]+contract+source[end:]).replace('PR1','MR1').replace('Punch Register','Maze Return').replace('punch-register','maze-return').replace("prev['state']==4", "prev['state']==3")
needle="            self.progress_ms+=dt"
replacement="""            if row['visits']==prev['visits'] and any(row[k]!=prev[k] for k in
                    ('added','loops','repeats','erased','depth','route')):
                self.reject('route changed without a completed visit')
            # Every newly completed visit requires at least a 100 ms clearing
            # window. One prior in-flight visit may finish just after attachment.
            if row['visits']-prev['visits'] > dt//100+1:
                self.reject('too many visits for elapsed device time')
            self.progress_ms+=dt"""
assert needle in text
text=text.replace(needle,replacement)
dest=root/'app/protocol.py'
if dest.exists(): raise SystemExit('Refusing existing output')
dest.parent.mkdir(exist_ok=True)
dest.write_text(text)
