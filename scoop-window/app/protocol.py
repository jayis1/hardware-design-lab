# Author: jayis1. SPDX-License-Identifier: MIT
"""SW1 receive-only evidence. No authentication or persistent boot identity."""
import binascii
from collections import deque
import os
import re
import stat

CHANNEL='starts completed aborted flagged away_ms foreign_ms'.split()
FIELDS='qualify now levels qualified ready active marked'.split()+[
    side+'_'+key for side in ('a','b') for key in CHANNEL]+['gaps','tx_errors','faults']
COUNTERS=[side+'_'+key for side in ('a','b') for key in CHANNEL]+['gaps','tx_errors']

def encode(row):
    body='SW1,'+','.join(str(row[key]) for key in FIELDS)
    return body+f'*{binascii.crc_hqx(body.encode(),65535):04X}\n'

def require(condition,message):
    if not condition:
        raise ValueError(message)

def validate(row):
    require(set(row)==set(FIELDS),'fields')
    require(all(type(v) is int and 0<=v<=0xffffffff for v in row.values()),'uint32 range')
    require(row['qualify']==5,'incompatible qualification')
    for key,limit in [('levels',15),('qualified',15),('ready',3),('active',3),('marked',3),('faults',7)]:
        require(row[key]<=limit,'range '+key)
    require(not (row['ready']&row['active']),'ready and active')
    require(not (row['marked']&~row['active']),'marked outside active')
    require(not row['faults'] or not (row['ready']|row['active']|row['marked']),'terminal state')
    for i,side in enumerate(('a','b')):
        get=lambda key:row[side+'_'+key]
        bit=1<<i
        active=bool(row['active']&bit)
        marked=bool(row['marked']&bit)
        require(get('starts')==get('completed')+get('aborted')+active,'excursion conservation')
        require(get('flagged')<=get('completed')+get('aborted')+marked,'flag ownership')
        require(get('foreign_ms')<=get('away_ms'),'duration subset')
        require(not get('foreign_ms') or get('flagged'),'foreign without flag')
        require(not get('away_ms') or get('starts'),'away without start')
        require(not marked or get('flagged'),'marked without flag')
        if active or row['ready']&bit:
            require(row['qualified']&(1<<(i*2)),'dock unknown')
            require(bool(row['levels']&(1<<(i*2)))!=active,'dock state')
    return row

def decode(line):
    require(len(line)<=511 and re.fullmatch(r'SW1(?:,[0-9]{1,10}){22}\*[0-9A-F]{4}\n',line),'invalid SW1 framing')
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
            if self.buffer.endswith(b'SW1,') and len(self.buffer)>4:
                self.errors+=1
                self.buffer=b'SW1,'
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
            for side in ('a','b'):
                away=row[side+'_away_ms']-previous[side+'_away_ms']
                foreign=row[side+'_foreign_ms']-previous[side+'_foreign_ms']
                if not 0<=foreign<=away<=dt:
                    self.fail('impossible duration increment')
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
