# Author: jayis1. SPDX-License-Identifier: MIT
"""Receive-only Maze Return protocol, serial transport and evidence store."""
import binascii
import json
import os
import re
import select
import stat
import termios
import time

FIELDS = ('now raw mask qualified state fault visits added loops repeats erased gaps '
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

class Decoder:
    def __init__(self):
        self.buffer=b''
        self.errors=0
    def fail(self, message, strict):
        self.errors+=1
        if strict: raise ValueError(message)
    def feed(self, data, strict=False):
        rows=[]
        # Bound both individual frames and callers' chunks; no unbounded buffering.
        if len(data)>65536: raise ValueError('oversized read')
        self.buffer+=data
        while self.buffer:
            marker=self.buffer.find(b'MR1,')
            if marker<0:
                if len(self.buffer)>4:
                    self.fail('unframed input',strict)
                    self.buffer=self.buffer[-3:]
                break
            if marker:
                self.fail('discarded fragment',strict)
                self.buffer=self.buffer[marker:]
            end=self.buffer.find(b'\n')
            next_marker=self.buffer.find(b'MR1,',4)
            if next_marker>=0 and (end<0 or next_marker<end):
                self.fail('truncated frame',strict)
                self.buffer=self.buffer[next_marker:]
                continue
            if end<0:
                if len(self.buffer)>512:
                    self.fail('oversized frame',strict); self.buffer=b''
                break
            line,self.buffer=self.buffer[:end],self.buffer[end+1:]
            try:
                if len(line)>512: raise ValueError('frame too long')
                body,crc=line.split(b'*')
                if not re.fullmatch(rb'[0-9A-F]{4}',crc): raise ValueError('CRC format')
                if binascii.crc_hqx(body,65535)!=int(crc,16): raise ValueError('CRC mismatch')
                fields=body.split(b',')[1:]
                if len(fields)!=len(FIELDS): raise ValueError('field count')
                if any(not re.fullmatch(rb'0|[1-9][0-9]{0,9}',f) for f in fields):
                    raise ValueError('decimal format')
                row=dict(zip(FIELDS,map(int,fields)))
                validate(row); rows.append(row)
            except (ValueError,UnicodeError) as exc:
                self.fail(str(exc),strict)
        return rows
    def finish(self):
        if self.buffer:
            self.errors+=1
            raise ValueError('partial recording tail')

class Session:
    def __init__(self, source):
        self.source=source
        self.connected=False
        self.blocked=False
        self.warning=''
        self.errors=0
        self.rows=[]
        self.first_host=None
        self.last_host=None
        self.progress_ms=0
        self.advances=0
    def reject(self, message):
        self.blocked=True; self.warning=message
        raise ValueError(message)
    def accept(self, row, host=None):
        if self.blocked: raise ValueError(self.warning)
        validate(row)
        host=time.monotonic() if host is None else host
        if self.rows:
            prev=self.rows[-1]
            dt=(row['now']-prev['now']) & 0xffffffff
            if dt==0:
                if row!=prev: self.reject('contradictory duplicate')
                return
            if dt>=0x80000000: self.reject('timestamp rollback; reconnect explicitly')
            if any(row[k]<prev[k] for k in COUNTERS): self.reject('counter rollback')
            if row['fault'] | prev['fault'] != row['fault']: self.reject('fault disappeared')
            if prev['state']==3 and any(row[k]!=prev[k] for k in FIELDS if k not in ('now','raw','tx_errors')):
                self.reject('halted evidence changed')
            if row['visits']==prev['visits'] and any(row[k]!=prev[k] for k in
                    ('added','loops','repeats','erased','depth','route')):
                self.reject('route changed without a completed visit')
            old=[(prev['route']>>(2*i))&3 for i in range(prev['depth'])]
            new=[(row['route']>>(2*i))&3 for i in range(row['depth'])]
            if old and (not new or old[0]!=new[0]):
                self.reject('first checkpoint changed')
            if row['visits']==prev['visits']+1:
                delta=tuple(row[k]-prev[k] for k in ('added','loops','repeats','erased'))
                append=(delta==(1,0,0,0) and len(new)==len(old)+1 and new[:-1]==old)
                repeat=(delta==(0,0,1,0) and new==old)
                truncate=(len(new)<len(old) and new==old[:len(new)] and
                          delta==(0,1,0,len(old)-len(new)))
                if not (append or repeat or truncate):
                    self.reject('impossible single-visit route transition')
            # Every newly completed visit requires at least a 100 ms clearing
            # window. One prior in-flight visit may finish just after attachment.
            if row['visits']-prev['visits'] > dt//100+1:
                self.reject('too many visits for elapsed device time')
            self.progress_ms+=dt
            if self.source=='live':
                assert self.last_host is not None and self.first_host is not None
                if host-self.last_host>1.5: self.reject('host reception pause; reconnect')
                drift=(host-self.first_host)-self.progress_ms/1000
                if abs(drift)>1.0: self.reject('host/device progress disagreement; reconnect')
            self.advances+=1
        else:
            self.first_host=host
        self.last_host=host
        self.rows.append(dict(row))
        # Snapshots, not individual trial logs: latest counters survive truncation.
        self.rows=self.rows[-2000:]
    def fresh(self, host=None):
        host=time.monotonic() if host is None else host
        return (self.source=='live' and self.connected and not self.blocked and
                self.advances>0 and self.last_host is not None and host-self.last_host<=1.0)
    def export(self, path, note=''):
        document={'author':'jayis1','format':'maze-return-evidence-1',
                  'source':self.source,'connected':self.connected,'fresh':bool(self.fresh()),
                  'blocked':self.blocked,'warning':self.warning,'parser_errors':self.errors,
                  'note':note,'snapshot_limit':2000,'snapshots':self.rows}
        fd=os.open(path,os.O_WRONLY|os.O_CREAT|os.O_EXCL,0o600)
        with os.fdopen(fd,'w') as handle:
            json.dump(document,handle,indent=2); handle.write('\n')

class Serial:
    def __init__(self,path):
        self.fd=os.open(path,os.O_RDWR|os.O_NOCTTY|os.O_NONBLOCK)
        self.saved=None
        try:
            self.saved=termios.tcgetattr(self.fd)
            settings=termios.tcgetattr(self.fd)
            settings[0]=0; settings[1]=0; settings[2]=termios.CLOCAL|termios.CREAD|termios.CS8
            settings[3]=0; settings[4]=termios.B115200; settings[5]=termios.B115200
            settings[6][termios.VMIN]=0; settings[6][termios.VTIME]=0
            termios.tcsetattr(self.fd,termios.TCSANOW,settings)
            termios.tcflush(self.fd,termios.TCIFLUSH)
            self.poller=select.poll(); self.poller.register(self.fd,select.POLLIN|select.POLLHUP|select.POLLERR)
        except BaseException:
            self.close(); raise
    def read(self):
        if self.fd is None: raise OSError('serial closed')
        events=self.poller.poll(0)
        if not events: return b''
        flags=events[0][1]
        if flags & select.POLLIN:
            try: data=os.read(self.fd,65536)
            except BlockingIOError: data=b''
            if data: return data
        if flags & (select.POLLHUP|select.POLLERR|select.POLLNVAL):
            raise OSError('serial disconnected')
        return b''
    def close(self):
        if self.fd is None: return
        try:
            if self.saved is not None: termios.tcsetattr(self.fd,termios.TCSANOW,self.saved)
        except (OSError,termios.error): pass
        finally: os.close(self.fd); self.fd=None

def load_replay(path):
    fd=os.open(path,os.O_RDONLY|os.O_NONBLOCK)
    with os.fdopen(fd,'rb') as handle:
        info=os.fstat(handle.fileno())
        if not stat.S_ISREG(info.st_mode) or info.st_size>2*1024*1024:
            raise ValueError('replay must be a regular file at most 2 MiB')
        data=handle.read(2*1024*1024+1)
    if len(data)>2*1024*1024: raise ValueError('recording grew beyond limit')
    session=Session('replay'); decoder=Decoder()
    for offset in range(0,len(data),4096):
        for row in decoder.feed(data[offset:offset+4096],strict=True): session.accept(row)
    decoder.finish()
    if not session.rows: raise ValueError('empty recording')
    return session
