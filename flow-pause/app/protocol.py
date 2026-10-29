# Author: jayis1. SPDX-License-Identifier: MIT
"""Strict FP1 decoding and conservative, bounded local evidence sessions."""
import binascii
import collections
import re
FIELDS = ('time gaps tx fault qualify silence_limit pulse_limit state mask known '
          'runs finished aborted pulses bad_pulses pauses recovered assisted abandoned '
          'taps silent pause_age pause_total pause_max episode_taps').split()
CUMULATIVE = ('gaps tx runs finished aborted pulses bad_pulses pauses recovered assisted '
              'abandoned taps pause_total pause_max').split()

def validate(row):
    if len(row) != len(FIELDS) or any(x < 0 or x > 0xffffffff for x in row):
        raise ValueError('Wrong field count or integer range')
    f = dict(zip(FIELDS,row))
    if (f['qualify'],f['silence_limit'],f['pulse_limit']) != (8,5000,2000):
        raise ValueError('Incompatible measurement policy')
    state=f['state']; active=state in (2,3)
    if state > 4 or f['mask'] > 15 or f['known'] > 15 or f['fault'] > 7:
        raise ValueError('Invalid state bits')
    if (state == 4) != bool(f['fault']):
        raise ValueError('Fault/state contradiction')
    if f['runs'] != f['finished']+f['aborted']+active:
        raise ValueError('Run conservation')
    if f['pauses'] != f['recovered']+f['abandoned']+(state == 3):
        raise ValueError('Pause conservation')
    if f['assisted'] > min(f['recovered'],f['taps']) or f['recovered'] > f['pulses']:
        raise ValueError('Recovery attribution')
    if f['episode_taps'] > f['taps'] or not f['pause_age'] <= f['pause_max'] <= f['pause_total']:
        raise ValueError('Duration/episode bounds')
    if f['bad_pulses'] > f['aborted'] or (f['pauses'] and not f['runs']):
        raise ValueError('Episode without run')
    if f['pauses'] > f['pulses']+f['runs'] or f['pause_total'] > f['pauses']*f['pause_max']:
        raise ValueError('Impossible pause aggregate')
    if not f['pauses'] and (f['pause_total'] or f['taps']):
        raise ValueError('Evidence without pause')
    if f['silent'] > 5000 or (state == 3 and f['silent'] != 5000):
        raise ValueError('Silence bounds')
    if state != 3 and (f['pause_age'] or f['episode_taps']):
        raise ValueError('Inactive pause')
    if not active and f['silent']:
        raise ValueError('Inactive silence')
    if active and (f['known'] != 15 or f['mask'] & 10 != 10):
        raise ValueError('Active without supply/enable')
    return tuple(row)

class Parser:
    def __init__(self):
        self.buffer=bytearray()
        self.errors=0
        self.discarded=0
    def feed(self,data):
        rows=[]
        # Process bytewise to bound memory even with a malicious huge chunk.
        for byte in data:
            self.buffer.append(byte)
            if self.buffer.endswith(b'FP1') and len(self.buffer)>3:
                self.discarded += len(self.buffer)-3
                self.buffer=bytearray(b'FP1')
            if byte == 10:
                line=bytes(self.buffer); self.buffer.clear()
                try:
                    match=re.fullmatch(rb'FP1((?:,[0-9]{1,10}){25})\*([0-9A-F]{4})\n',line)
                    if not match: raise ValueError('Framing')
                    body=line[:line.index(b'*')]
                    if binascii.crc_hqx(body,65535) != int(match[2],16):
                        raise ValueError('CRC')
                    rows.append(validate([int(x) for x in match[1][1:].split(b',')]))
                except ValueError:
                    self.errors+=1
            if len(self.buffer)>767:
                self.errors+=1
                # Retain possible partial marker at the boundary.
                self.buffer=self.buffer[-2:]
        return rows
    def finish(self):
        if self.buffer:
            self.errors+=1; self.buffer.clear()

class Session:
    def __init__(self,source):
        self.source=source
        self.rows=collections.deque(maxlen=500)
        self.evicted=0
        self.blocked=''
        self.last_host=None
        self.anchor_host=None
        self.progress=0
        self.advances=0
    def accept(self,row,host):
        if self.blocked: return
        try: row=validate(row)
        except ValueError as error:
            self.blocked=str(error); return
        if self.rows:
            old=self.rows[-1]
            dt=(row[0]-old[0]) & 0xffffffff
            if dt == 0:
                if row != old: self.blocked='Contradictory duplicate timestamp'
                return
            if dt >= 0x80000000:
                self.blocked='Device clock rollback'; return
            a=dict(zip(FIELDS,old)); b=dict(zip(FIELDS,row))
            if any(b[k]<a[k] for k in CUMULATIVE) or a['fault'] & ~b['fault']:
                self.blocked='Counter rollback or disappearing fault'; return
            if b['pause_total']-a['pause_total'] > dt:
                self.blocked='Impossible pause duration growth'; return
            if a['state']==4 and any(b[k]!=a[k] for k in FIELDS[10:]):
                self.blocked='Halted evidence changed'; return
            if self.source=='live':
                if host-self.last_host > 1 or host < self.last_host:
                    self.blocked='Reception pause; reconnect explicitly'; return
                self.progress += dt
                # A bounded 500 ms budget covers startup buffering and modest
                # oscillator drift. It never grows to admit unlimited backlog.
                if abs(self.progress/1000-(host-self.anchor_host)) > .5:
                    self.blocked='Device/host progress diverged; reconnect'; return
            self.advances+=1
        else:
            self.anchor_host=host
        self.last_host=host
        if len(self.rows)==self.rows.maxlen: self.evicted+=1
        self.rows.append(row)
    def fresh(self,host,connected):
        return (self.source=='live' and connected and not self.blocked and
                self.advances>0 and self.last_host is not None and
                0 <= host-self.last_host < 1)
