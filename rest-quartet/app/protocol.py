# Author: jayis1. SPDX-License-Identifier: MIT
"""Bounded RQ1 framing and conservative receive-only session evidence."""
import binascii
from collections import deque

GOAL = 60000
LIMIT = 3600000
MAX = 0xffffffff
NAMES = ('state','starts','early','rested','aborted','age','last','minimum','maximum','total','timeouts')

def encode(fields):
    """Synthetic test utility; production records come from the MCU."""
    body = ('RQ1,' + ','.join(map(str,fields))).encode('ascii')
    return body + f'*{binascii.crc_hqx(body,65535):04X}\n'.encode('ascii')

def decode(line):
    if len(line)>768 or not line.endswith(b'\n'):
        raise ValueError('frame length or terminator')
    body, separator, checksum = line[:-1].rpartition(b'*')
    if not separator or len(checksum)!=4 or any(c not in b'0123456789ABCDEFabcdef' for c in checksum):
        raise ValueError('CRC envelope')
    if binascii.crc_hqx(body,65535)!=int(checksum,16):
        raise ValueError('CRC mismatch')
    items=body.split(b',')
    if items[0]!=b'RQ1' or len(items)!=52:
        raise ValueError('version or fields')
    if any(not v.isdigit() or len(v)>10 for v in items[1:]):
        raise ValueError('decimal fields')
    f=tuple(map(int,items[1:]))
    if any(v>MAX for v in f) or f[3]>7 or f[4:7]!=(8,GOAL,LIMIT):
        raise ValueError('range or incompatible policy')
    for i in range(4):
        state,starts,early,rested,aborted,age,last,minimum,maximum,total,timeouts=f[7+11*i:18+11*i]
        count=early+rested
        if state>2 or starts!=count+aborted+(state==2) or timeouts>aborted:
            raise ValueError('slot conservation')
        if (state!=2 and age) or age>=LIMIT or (f[3] and state):
            raise ValueError('slot state')
        if not count:
            if last or minimum or maximum or total:
                raise ValueError('unearned duration')
            continue
        if not 0<minimum<=last<=maximum<LIMIT:
            raise ValueError('duration order')
        if not minimum*count<=total<=maximum*count:
            raise ValueError('duration sum')
        if total<maximum+minimum*(count-1):
            raise ValueError('maximum missing from sum')
        if bool(early)!=(minimum<GOAL) or bool(rested)!=(maximum>=GOAL):
            raise ValueError('classification extrema')
        if not rested*GOAL+early<=total<=rested*(LIMIT-1)+early*(GOAL-1):
            raise ValueError('classified totals')
    return f

class Parser:
    def __init__(self):
        self.buffer=bytearray()
        self.errors=0

    def feed(self,data):
        records=[]
        # Bytewise cap prevents allocating an arbitrarily large pending line.
        for byte in data:
            self.buffer.append(byte)
            if self.buffer.endswith(b'RQ1,') and len(self.buffer)>4:
                self.errors+=1
                self.buffer=bytearray(b'RQ1,')
            if byte==10:
                try:
                    records.append(decode(bytes(self.buffer)))
                except (ValueError,OverflowError):
                    self.errors+=1
                self.buffer.clear()
            elif len(self.buffer)>=768:
                self.errors+=1
                self.buffer.clear()
        return records

    def finish(self):
        if self.buffer:
            self.errors+=1
            self.buffer.clear()

class Session:
    def __init__(self,source):
        self.source=source
        self.rows=deque(maxlen=500)
        self.evicted=0
        self.blocked=''
        self.last_host=None
        self.anchor_host=None
        self.device_elapsed=0
        self.advances=0

    def accept(self,f,host):
        if self.blocked:
            return False
        if self.source=='live' and self.last_host is not None and host-self.last_host>1:
            self.blocked='Reception stalled; reconnect, do not trust queued snapshots'
            return False
        if self.rows:
            previous=self.rows[-1]
            dt=(f[0]-previous[0]) & MAX
            if dt==0:
                if f!=previous:
                    self.blocked='Contradictory same-time snapshot'
                return False
            if dt>MAX//2 or f[1]<previous[1] or f[2]<previous[2] or previous[3]&~f[3]:
                self.blocked='Reset or counter rollback suspected'
                return False
            for i in range(4):
                old=previous[7+i*11:18+i*11]
                new=f[7+i*11:18+i*11]
                if any(new[j]<old[j] for j in (1,2,3,4,8,9,10)):
                    self.blocked='Pocket cumulative rollback'
                    return False
                old_count=old[2]+old[3]
                new_count=new[2]+new[3]
                if old_count==new_count and new[6:10]!=old[6:10]:
                    self.blocked='Completion evidence changed without completion'
                    return False
                if old_count and new[7]>old[7]:
                    self.blocked='Minimum duration grew'
                    return False
                if previous[3] and new!=old:
                    self.blocked='Terminal evidence changed'
                    return False
            self.device_elapsed+=dt
            self.advances+=1
            if self.source=='live' and abs(self.device_elapsed/1000-(host-self.anchor_host))>.5:
                self.blocked='Device/host progress differs by over 0.5 s; reconnect'
                return False
        else:
            self.anchor_host=host
        if len(self.rows)==self.rows.maxlen:
            self.evicted+=1
        self.rows.append(f)
        self.last_host=host
        return True

    def fresh(self,host,connected):
        return bool(connected and self.source=='live' and self.advances and
                    not self.blocked and self.last_host is not None and
                    0<=host-self.last_host<=1)
