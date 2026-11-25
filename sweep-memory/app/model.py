# Author: jayis1. SPDX-License-Identifier: MIT
"""SM1 evidence validation. Necessary sparse bounds are not history proofs."""
FIELDS = 'now samples gaps missed tx_errors raw qualified valid fault state reason position direction starts completed abandoned age outward_x outward_y return_x return_y compared mismatch_x mismatch_y'.split()
STATES = ['WAIT_BASELINE','READY','OUTWARD','RETURNING','DONE','REJECTED','HALT']
REASONS = ['none','phase jump','early reversal','overshoot / return reversal','acquisition gap','deadline','terminal fault']
EVIDENCE = 'starts completed abandoned age outward_x outward_y return_x return_y compared mismatch_x mismatch_y position direction'.split()

def require(condition, message):
    if not condition: raise ValueError(message)

def validate(r):
    require(set(r)==set(FIELDS),'schema')
    require(all(type(v) is int and 0<=v<=0xffffffff for v in r.values()),'integer range')
    for key,limit in [('raw',15),('qualified',15),('valid',1),('fault',5),('state',6),('reason',6),('position',16),('direction',2),('starts',1),('completed',1),('abandoned',1),('age',60000),('outward_x',131071),('outward_y',131071),('compared',65535),('return_x',65535),('return_y',65535)]:
        require(r[key]<=limit,key+' range')
    active=r['state'] in (2,3)
    require((r['state']==6)==bool(r['fault']),'fault state')
    require(r['starts']==r['completed']+r['abandoned']+active,'trial conservation')
    require((r['state']==4)==bool(r['completed'] and not r['fault']),'completion state')
    require(not r['starts'] or r['direction'] in (1,2),'direction missing')
    require(not active or r['age']<60000,'deadline')
    require(r['state']!=2 or 0<r['position']<16,'outward position')
    require(r['state']!=3 or r['position']>0,'return position')
    require(not r['completed'] or (r['position']==0 and r['compared']==65535),'completion coverage')
    for axis in ('x','y'):
        require(not (r['return_'+axis]&~r['compared']),'uncaptured return')
        require(r['mismatch_'+axis]==((r['outward_'+axis]^r['return_'+axis])&r['compared']),'mismatch plane')
    if r['state'] in (0,1):
        require(not any(r[x] for x in EVIDENCE),'premature evidence')
    if r['state']==2:
        require(r['compared']==0,'premature comparisons')
        require((r['outward_x']|r['outward_y']) < (1<<(r['position']+1)),'future outward bit')
    if r['state']==3:
        require(r['compared']==(65535 & ~((1<<r['position'])-1)),'return coverage')

class Session:
    def __init__(self, provenance):
        self.provenance=provenance
        self.rows=[]
        self.blocked=False
    def accept(self,r):
        require(not self.blocked,'session blocked; reconnect explicitly')
        try:
            validate(r)
            if self.rows:
                old=self.rows[-1]
                if r==old: return False
                dt=(r['now']-old['now'])&0xffffffff
                ds=r['samples']-old['samples']
                require(0<dt<0x80000000 and ds>=0,'clock / sample rollback')
                require(ds>0 or old['fault'] or r['fault'],'no acquisition advance')
                for key in ['gaps','missed','tx_errors','starts','completed','abandoned']:
                    require(r[key]>=old[key],key+' rollback')
                require(ds+r['missed']-old['missed']<=dt,'acquisition budget')
                if old['fault']:
                    require(r['fault']==old['fault'],'fault changed')
                if old['state'] in (4,5,6):
                    require(all(r[k]==old[k] for k in EVIDENCE),'terminal evidence changed')
                else:
                    allowed={0:{0,1,2,3,4,5,6},1:{1,2,3,4,5,6},2:{2,3,4,5,6},3:{3,4,5,6}}
                    require(r['state'] in allowed[old['state']],'phase regression')
                    require(r['age']>=old['age'] and r['age']-old['age']<=ds,'age budget')
                    require((r['compared']|old['compared'])==r['compared'],'coverage rollback')
                    if old['starts']:
                        require(r['direction']==old['direction'],'direction changed')
                        # Every captured outward cell is immutable. In outward
                        # phase capture extends only beyond the prior position.
                        mask=(1<<(old['position']+1))-1 if old['state']==2 else 131071
                        for axis in ('x','y'):
                            require((r['outward_'+axis]&mask)==(old['outward_'+axis]&mask),'reference changed')
                            mask_return=old['compared']
                            require((r['return_'+axis]&mask_return)==old['return_'+axis],'return changed')
            self.rows.append(dict(r))
            self.rows=self.rows[-2000:]
            return True
        except ValueError:
            self.blocked=True
            raise
