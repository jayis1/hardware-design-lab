# Author: jayis1. SPDX-License-Identifier: MIT
"""Bounded SQ1 framing, POSIX serial and strict replay; stdlib only."""
import binascii, os, re, select, stat, termios
from model import FIELDS, validate, Session

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
            marker=self.buffer.find(b'SQ1,')
            if marker<0:
                if len(self.buffer)>4:
                    self.fail('unframed input',strict)
                    self.buffer=self.buffer[-3:]
                break
            if marker:
                self.fail('discarded fragment',strict)
                self.buffer=self.buffer[marker:]
            end=self.buffer.find(b'\n')
            next_marker=self.buffer.find(b'SQ1,',4)
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
