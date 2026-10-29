# Author: jayis1. SPDX-License-Identifier: MIT
"""PP1 receive-only evidence. No authentication or persistent boot identity."""
import binascii
from collections import deque
import os
import re
import stat

FIELDS='qualify timeout now levels qualified phase starts completed aborted elapsed last_ms last_reason gaps tx_errors faults'.split()
COUNTERS='starts completed aborted gaps tx_errors'.split()
PHASES=['Wait for dry home','Ready','Approach first stop','First stop: declare source immersion','Release in source','Aspirated: remove from source','Dispense: reach second stop','Blowout: return home','Removed from source: press to dispense']
REASONS=['none','wrong stop order','source declaration ordering','contradictory contacts','acquisition gap','30 s timeout','hardware fault']

def encode(row):
    body='PP1,'+','.join(str(row[key]) for key in FIELDS)
    return body+f'*{binascii.crc_hqx(body.encode(),65535):04X}\n'

def require(condition,message):
    if not condition:
        raise ValueError(message)

def validate(row):
    require(set(row)==set(FIELDS),'fields')
    require(all(type(v) is int and 0<=v<=0xffffffff for v in row.values()),'uint32 range')
    require(row['qualify']==5 and row['timeout']==30000,'incompatible policy')
    for key,limit in [('levels',15),('qualified',15),('phase',8),('last_reason',6),('faults',7)]:
        require(row[key]<=limit,'range '+key)
    active=row['phase']>=2
    require(row['starts']==row['completed']+row['aborted']+active,'cycle conservation')
    require(row['elapsed']<30000 and row['last_ms']<=30000,'duration bounds')
    require(active or not row['elapsed'],'idle elapsed')
    require(not row['faults'] or row['phase']==0,'terminal state')
    require(row['phase']==0 or row['qualified']==15,'unknown active contacts')
    allowed={1:(1,),2:(0,),3:(2,),4:(8,10),5:(9,),6:(0,2),7:(0,2,6),8:(0,1)}
    require(row['phase']==0 or row['levels'] in allowed[row['phase']],'phase/contact inconsistency')
    require(row['completed'] or row['aborted'] or not(row['last_ms'] or row['last_reason']),'result without trial')
    require(not row['last_reason'] or row['aborted'],'error without abort')
    return row

def decode(line):
    require(len(line)<=511 and re.fullmatch(r'PP1(?:,[0-9]{1,10}){15}\*[0-9A-F]{4}\n',line),'invalid PP1 framing')
    body,check=line.rstrip('\n').split('*')
    require(binascii.crc_hqx(body.encode(),65535)==int(check,16),'CRC mismatch')
    return validate(dict(zip(FIELDS,map(int,body.split(',')[1:]))))

class Stream:
    """Bounded buffer; resynchronize on a full marker after truncated TX."""
    def __init__(self):
        self.buffer=b''
        self.errors=0
    def feed(self,data):
        rows=[]
        for byte in data:
            self.buffer+=bytes([byte])
            if self.buffer.endswith(b'PP1,') and len(self.buffer)>4:
                self.errors+=1
                self.buffer=b'PP1,'
            if byte==10:
                try: rows.append(decode(self.buffer.decode('ascii')))
                except (ValueError,UnicodeError): self.errors+=1
                self.buffer=b''
            elif len(self.buffer)>511:
                self.errors+=1
                self.buffer=b''
        return rows
    def finish(self):
        require(not self.buffer,'truncated recording tail')

def read_replay(path):
    fd=os.open(path,os.O_RDONLY|os.O_NONBLOCK)
    try:
        info=os.fstat(fd)
        require(stat.S_ISREG(info.st_mode) and info.st_size<=2_000_000,'replay must be regular and <= 2 MB')
        with os.fdopen(fd,'rb',closefd=False) as source:
            data=source.read(2_000_001)
        require(len(data)<=2_000_000,'replay grew beyond limit')
    finally:
        os.close(fd)
    stream=Stream()
    rows=stream.feed(data)
    stream.finish()
    require(not stream.errors and rows,'invalid or empty replay')
    return rows

class Model:
    def __init__(self):
        self.rows=deque(maxlen=2000)
        self.received=deque(maxlen=2000)
        self.last=None
        self.last_host=0.0
        self.origin_host=0.0
        self.elapsed=0
        self.progress=0
        self.blocked=False
        self.warning='waiting for two advancing records'
    def fail(self,message):
        self.blocked=True
        self.warning=message
        raise ValueError(message)
    def accept(self,row,host,live=True):
        if self.blocked:
            raise ValueError(self.warning)
        try:
            validate(row)
        except ValueError as error:
            self.fail(str(error))
        if self.last is not None:
            previous=self.last
            dt=(row['now']-previous['now'])&0xffffffff
            if dt==0:
                if row!=previous: self.fail('contradictory duplicate timestamp')
                return
            if dt>=0x80000000: self.fail('device time rollback; reconnect required')
            if any(row[k]<previous[k] for k in COUNTERS):
                self.fail('counter rollback; reconnect required')
            if row['faults']|previous['faults']!=row['faults']:
                self.fail('sticky fault disappeared')
            if previous['faults'] and any(row[k]!=previous[k] for k in FIELDS if k not in ('now','tx_errors','faults')):
                self.fail('terminal evidence changed')
            if row['starts']-previous['starts']>dt:
                self.fail('impossible cycle increment')
            if row['completed']==previous['completed'] and row['aborted']==previous['aborted']:
                if any(row[k]!=previous[k] for k in ('last_ms','last_reason')):
                    self.fail('last result changed without disposition')
            if row['starts']==previous['starts'] and row['phase']>=2 and previous['phase']>=2:
                if row['elapsed']-previous['elapsed']!=dt:
                    self.fail('active duration discontinuity')
            self.elapsed+=dt
            if live:
                host_elapsed=host-self.origin_host
                tolerance=.5+min(1.0,max(0,host_elapsed)*.02)
                if not 0<=host-self.last_host<=1 or abs(self.elapsed/1000-host_elapsed)>tolerance:
                    self.fail('timing continuity lost; reconnect and flush')
            self.progress+=1
        else:
            self.origin_host=host
        self.last=row.copy()
        self.last_host=host
        self.rows.append(row.copy())
        self.received.append(host)
        self.warning=''
    def fresh(self,host):
        return not self.blocked and self.progress>0 and 0<=host-self.last_host<=1
