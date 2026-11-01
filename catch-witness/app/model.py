# Author: jayis1. SPDX-License-Identifier: MIT
"""CW1 conservative snapshot checks; no claim to reconstruct hidden edges."""
FIELDS=('now state levels valid starts settled intentional challenged unexplained '
        'unseated timed_out aborted gaps faults tx_errors age dwell handle_age '
        'tug_age handle_credit tug_credit bolt_losses').split()
COUNTERS=FIELDS[4:13]+['tx_errors','bolt_losses']
STATES=('WAIT','READY','SETTLING','LATCHED','DONE','HALT')

def validate(r):
    if set(r)!=set(FIELDS) or any(type(v) is not int or not 0<=v<=0xffffffff for v in r.values()):
        raise ValueError('field type or range')
    state=r['state']; active=state in (2,3)
    ended=sum(r[k] for k in ('intentional','challenged','unexplained','unseated','timed_out','aborted'))
    if state>5 or r['levels']>15 or r['valid']>15 or r['faults']>15:
        raise ValueError('mask/state')
    if ended+active!=r['starts']: raise ValueError('episode conservation')
    retained=sum(r[k] for k in ('intentional','challenged','unexplained'))
    if retained>r['settled'] or r['settled']+r['unseated']>r['starts']:
        raise ValueError('settlement conservation')
    if r['age']>30000 or r['dwell']>200: raise ValueError('duration range')
    if (state==5)!=bool(r['faults']): raise ValueError('fault/state')
    if r['gaps'] and not r['faults']&3: raise ValueError('gap/fault')
    for name in ('handle','tug'):
        credit=r[name+'_credit']; age=r[name+'_age']
        if credit>1 or age>500 or (not credit and age) or (credit and state!=3):
            raise ValueError('credit state')
    if state==3 and (r['dwell']!=200 or not r['settled']): raise ValueError('unsettled latch')
    if active and (not r['levels']&1 or r['age']>=30000): raise ValueError('active endpoint')

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
            if dt==0:
                if row!=old: self.fail('contradictory duplicate timestamp')
                return False
            if dt>=0x80000000: self.fail('timestamp rollback; reconnect required')
            if any(row[k]<old[k] for k in COUNTERS): self.fail('counter rollback; possible reset')
            if row['faults']&old['faults']!=old['faults']: self.fail('disappearing fault')
            if old['state']==5 and any(row[k]!=old[k] for k in FIELDS if k not in ('now','tx_errors','faults')):
                self.fail('halted evidence changed')
            if row['starts']-old['starts']>dt//5+1: self.fail('impossible start rate')
            if row['settled']-old['settled']>row['starts']-old['starts']+(old['state']==2):
                self.fail('settlement without pending start')
            if row['bolt_losses']-old['bolt_losses']>dt//5+1: self.fail('impossible bolt edge rate')
        self.rows.append(dict(row)); self.rows=self.rows[-4096:]
        return True
