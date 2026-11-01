# Author: jayis1. SPDX-License-Identifier: MIT
"""BA1 snapshot validation; sparse endpoints cannot prove all hidden motion."""
FIELDS=('now state levels valid faults gaps tx_errors a_phase a_age a_cycles '
        'b_phase b_age b_cycles windows zero one many assigned completed_hits '
        'pending first last period last_hits last_first last_last').split()
COUNTERS='gaps tx_errors a_cycles b_cycles windows zero one many assigned completed_hits'.split()
STATES=('WAIT','RUN','HALT')
def offsets(count,first,last,bound):
    if not count: return first==last==0
    if not 0<first<=last<=bound: return False
    if count==1: return first==last
    return (count-1)*10<=last-first

def validate(r):
    if set(r)!=set(FIELDS) or any(type(v) is not int or not 0<=v<=0xffffffff for v in r.values()):
        raise ValueError('field type or range')
    if r['state']>2 or r['levels']>15 or r['valid']>15 or r['faults']>63:
        raise ValueError('mask/state')
    if (r['state']==2)!=bool(r['faults']): raise ValueError('fault/state')
    if r['gaps'] and not r['faults']&3: raise ValueError('gap/fault')
    for side in ('a','b'):
        phase,age,cycles=(r[side+'_'+k] for k in ('phase','age','cycles'))
        if phase>2 or age>30000 or (not phase and (age or cycles)):
            raise ValueError('oscillator state')
        if r['state']!=2 and age>=30000: raise ValueError('expired oscillator')
    if r['zero']+r['one']+r['many']!=r['windows'] or r['windows']!=r['a_cycles']:
        raise ValueError('window conservation')
    if r['completed_hits']+r['pending']!=r['assigned'] or r['assigned']>r['b_cycles']:
        raise ValueError('assignment conservation')
    if r['one']+2*r['many']>r['completed_hits']:
        raise ValueError('hit lower bound')
    if not r['many'] and r['completed_hits']!=r['one']: raise ValueError('single counts')
    if not offsets(r['pending'],r['first'],r['last'],r['a_age']): raise ValueError('pending offsets')
    if not offsets(r['last_hits'],r['last_first'],r['last_last'],r['period']):
        raise ValueError('retained offsets')
    if r['windows']:
        if not 10<=r['period']<30000: raise ValueError('period range')
        name='zero' if not r['last_hits'] else ('one' if r['last_hits']==1 else 'many')
        if not r[name] or r['last_hits']>r['completed_hits']: raise ValueError('last class')
    elif r['period'] or r['last_hits'] or r['completed_hits']: raise ValueError('no completed window')
    if r['state']==0 and (r['windows'] or r['a_phase'] or r['b_phase']):
        raise ValueError('startup evidence')

class Session:
    def __init__(self,source):
        self.source=source; self.rows=[]; self.blocked=False; self.warning=''
    def fail(self,message):
        self.blocked=True; self.warning=message
        raise ValueError(message)
    def accept(self,row):
        if self.blocked: raise ValueError(self.warning)
        validate(row)
        if self.rows:
            old=self.rows[-1]; dt=(row['now']-old['now'])&0xffffffff
            if not dt:
                if old!=row: self.fail('contradictory duplicate timestamp')
                return False
            if dt>=0x80000000: self.fail('timestamp rollback; reconnect required')
            if any(row[k]<old[k] for k in COUNTERS): self.fail('counter rollback; possible reset')
            if row['faults']&old['faults']!=old['faults']: self.fail('disappearing fault')
            if old['state']==2 and any(row[k]!=old[k] for k in FIELDS if k not in ('now','faults','tx_errors')):
                self.fail('halted evidence changed')
            if row['windows']==old['windows']:
                if any(row[k]!=old[k] for k in ('period','last_hits','last_first','last_last','completed_hits')):
                    self.fail('retained result changed without a window')
                if row['pending']<old['pending']: self.fail('pending returns disappeared')
                if old['pending'] and row['first']!=old['first']: self.fail('first return changed')
                if row['assigned']==old['assigned'] and row['last']!=old['last']:
                    self.fail('last pending return changed without a new hit')
            da=row['a_cycles']-old['a_cycles']; db=row['b_cycles']-old['b_cycles']
            if da>(dt-1)//10+1 or db>(dt-1)//10+1: self.fail('impossible cycle rate')
            if row['assigned']-old['assigned']>db: self.fail('assigned nonexistent B returns')
            if da==1:
                name='zero' if not row['last_hits'] else ('one' if row['last_hits']==1 else 'many')
                if row[name]!=old[name]+1: self.fail('new result disagrees with bin')
                if row['completed_hits']-old['completed_hits']!=row['last_hits']:
                    self.fail('new result disagrees with completed hits')
            if old['state']!=2 and row['state']!=2:
                for side,delta in (('a',da),('b',db)):
                    if old[side+'_phase'] and not row[side+'_phase']:
                        self.fail('anchor disappeared')
                    if old[side+'_phase']==2 and row[side+'_phase']==1 and not delta:
                        self.fail('return phase changed without a completed cycle')
                    if old[side+'_phase'] and not delta and row[side+'_age']!=old[side+'_age']+dt:
                        self.fail('continuing cycle age mismatch')
        self.rows.append(dict(row)); self.rows=self.rows[-4096:]
        return True
