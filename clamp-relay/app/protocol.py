# Author: jayis1. SPDX-License-Identifier: MIT
"""Receive-only CR1 evidence contract. CRC is not authentication."""
import binascii
from collections import deque
import os
import re
import stat

FIELDS='qualify settle limit now levels qualified grips owner result direction starts completed aborted elapsed_ms gap_ms overlap_ms max_gap_ms direct gaps tx_errors faults'.split()
COUNTERS='starts completed aborted gaps tx_errors'.split()
METRICS='elapsed_ms gap_ms overlap_ms max_gap_ms'.split()

def encode(row):
    body='CR1,'+','.join(str(row[k]) for k in FIELDS)
    return body+f'*{binascii.crc_hqx(body.encode(),65535):04X}\n'

def decode(line):
    if len(line)>511 or not re.fullmatch(r'CR1(?:,[0-9]{1,10}){21}\*[0-9A-F]{4}\n',line):
        raise ValueError('invalid CR1 framing')
    body,check=line.rstrip('\n').split('*')
    if binascii.crc_hqx(body.encode(),65535)!=int(check,16):
        raise ValueError('CRC mismatch')
    row: dict[str, int]=dict(zip(FIELDS,map(int,body.split(',')[1:])))
    if any(v>0xffffffff for v in row.values()):
        raise ValueError('uint32 range')
    def require(test,message):
        if not test: raise ValueError(message)
    require([row[k] for k in FIELDS[:3]]==[5,50,10000],'incompatible policy')
    for key,limit in [('levels',15),('qualified',15),('grips',3),('owner',2),('result',6),('direction',2),('direct',1),('faults',7),('elapsed_ms',10000)]:
        require(row[key]<=limit,'range '+key)
    grips=int(row['levels']&3==3)+2*int(row['levels']&12==12)
    require(row['grips']==grips,'grip mask')
    require(bool(row['faults'])==(row['result']==6),'fault state')
    require(row['starts']==row['completed']+row['aborted']+(row['result']==1),'trial conservation')
    require(row['gap_ms']+row['overlap_ms']<=row['elapsed_ms'],'duration sum')
    require(row['max_gap_ms']<=row['gap_ms'],'gap maximum')
    require(bool(row['max_gap_ms'])==bool(row['gap_ms']),'gap presence')
    require(not row['starts'] or row['direction']!=0,'direction missing')
    require(row['starts'] or not any(row[k] for k in METRICS+['direction']),'trial missing')
    require(row['result']!=1 or (row['owner'] and row['qualified']==15),'active baseline')
    require(row['result']!=4 or row['elapsed_ms']==10000,'timeout duration')
    return row

class Stream:
    """Bounded resynchronizing framing, including truncated-TX recovery."""
    def __init__(self):
        self.buffer=b''
        self.errors=0
    def feed(self,data):
        rows=[]
        for byte in data:
            self.buffer+=bytes([byte])
            if self.buffer.endswith(b'CR1,') and len(self.buffer)>4:
                self.errors+=1
                self.buffer=b'CR1,'
            if byte==10:
                try: rows.append(decode(self.buffer.decode('ascii')))
                except (ValueError,UnicodeError): self.errors+=1
                self.buffer=b''
            elif len(self.buffer)>511:
                self.errors+=1
                self.buffer=b''
        return rows
    def finish(self):
        if self.buffer:
            raise ValueError('truncated recording tail')

def read_replay(path):
    fd=os.open(path,os.O_RDONLY|os.O_NONBLOCK)
    try:
        info=os.fstat(fd)
        if not stat.S_ISREG(info.st_mode) or info.st_size>2_000_000:
            raise ValueError('replay must be a regular file <= 2 MB')
        with os.fdopen(fd,'rb',closefd=False) as f: data=f.read(2_000_001)
        if len(data)>2_000_000: raise ValueError('replay grew beyond limit')
    finally: os.close(fd)
    stream=Stream(); rows=stream.feed(data); stream.finish()
    if stream.errors or not rows: raise ValueError('invalid or empty replay')
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
        if self.blocked: raise ValueError(self.warning)
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
            if row['starts']==previous['starts']:
                if any(row[k]<previous[k] for k in METRICS): self.fail('trial metric rollback')
                if row['direction']!=previous['direction']: self.fail('direction changed')
                if previous['result'] not in (0,1) and any(row[k]!=previous[k] for k in METRICS):
                    self.fail('finished trial changed')
            self.elapsed+=dt
            if live:
                host_elapsed=host-self.origin_host
                # Capped drift budget prevents a slow backlog growing forever.
                tolerance=.5+min(1.0,host_elapsed*.02)
                if host-self.last_host>1 or abs(self.elapsed/1000-host_elapsed)>tolerance:
                    self.fail('timing continuity lost; reconnect and flush')
            self.progress+=1
        else:
            self.origin_host=host
        self.last=row.copy(); self.last_host=host
        self.rows.append(row.copy())
        self.received.append(host)
        self.warning=''
    def fresh(self,host):
        return not self.blocked and self.progress>0 and 0<=host-self.last_host<=1
