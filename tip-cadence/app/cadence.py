#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Linux offline-first Tip Cadence workbench; no third-party dependencies."""
import binascii
from collections import deque
import json
import os
import select
import stat
import termios
import time
import tkinter as tk
from tkinter import ttk, filedialog

MAX_REPLAY=2*1024*1024
LANE_FIELDS=('tips','intervals','even_count','odd_count','even_sum_ms','odd_sum_ms',
             'min_ms','max_ms','jitter_ms','rejected','breaks','phase','anchor','age_ms')
FIELDS=('uptime_ms','raw','gaps','tx_errors','faults')+tuple(
    f'{lane}_{field}' for lane in ('A','B') for field in LANE_FIELDS)

def crc(data):
    return binascii.crc_hqx(data,0xffff)

def parse(line):
    try:
        payload,checksum=line.rstrip(b'\n').split(b'*')
        if len(checksum)!=4 or crc(payload)!=int(checksum,16):
            raise ValueError('CRC')
        fields=payload.split(b',')
        if fields[0]!=b'TC1' or len(fields)!=34:
            raise ValueError('identity or field count')
        if any(not x.isdigit() or len(x)>10 for x in fields[1:]):
            raise ValueError('decimal')
        v=[int(x) for x in fields[1:]]
        if any(x>0xffffffff for x in v) or v[1]>15 or v[4]>15:
            raise ValueError('range')
        for start in (5,19):
            tips,n,a,b,sa,sb,lo,hi,jitter,rejected,breaks,phase,anchor,age=v[start:start+14]
            if a+b!=n or (n and n>=tips) or phase>2 or anchor>1 or age>60001:
                raise ValueError('lane counters')
            if lo>hi or hi>60000 or (n and lo<100) or jitter>hi-lo:
                raise ValueError('interval extrema')
            if not n and (lo or hi or jitter):
                raise ValueError('empty extrema')
            if not a*lo<=sa<=a*hi or not b*lo<=sb<=b*hi:
                raise ValueError('cohort sums')
        return v
    except (ValueError,OverflowError) as exc:
        raise ValueError(f'invalid TC1 record: {exc}') from exc

class Model:
    def __init__(self,source='synthetic'):
        self.source=source
        self.connected=False
        self.rows=deque(maxlen=600)
        self.buffer=bytearray()
        self.errors=0
        self.discarded=0
        self.blocked=False
        self.warning=''
        self.progress=None
        self.origin=None
        self.device_elapsed=0
        self.previous=None

    def accept(self,v,host):
        if self.blocked:
            return
        if self.previous is not None:
            old=self.previous
            delta=(v[0]-old[0])&0xffffffff
            if delta==0:
                if v!=old:
                    self.blocked=True; self.warning='contradictory duplicate'
                return
            monotonic=[2,3]+[s+i for s in (5,19) for i in (0,1,2,3,4,5,7,8,9,10)]
            if delta>=0x80000000 or any(v[i]<old[i] for i in monotonic) or old[4]&~v[4]:
                self.blocked=True; self.warning='reset or counter rollback; reconnect explicitly'
                return
            for s in (5,19):
                if old[s+6] and v[s+6]>old[s+6]:
                    self.blocked=True; self.warning='minimum increased'; return
            self.device_elapsed+=delta
            if self.source=='live' and abs(self.device_elapsed/1000-(host-self.origin))>1:
                self.blocked=True; self.warning='host/device backlog or clock jump; reconnect'; return
        else:
            self.origin=host
        self.previous=v
        self.rows.append(dict(zip(FIELDS,v)))
        self.progress=host

    def feed(self,data,host):
        self.buffer.extend(data)
        # Full identity resynchronization also handles a truncated UART frame.
        while self.buffer:
            marker=self.buffer.find(b'TC1,')
            if marker<0:
                if len(self.buffer)>3:
                    self.discarded+=len(self.buffer)-3
                    del self.buffer[:-3]
                break
            if marker:
                self.discarded+=marker; del self.buffer[:marker]
            second=self.buffer.find(b'TC1,',4)
            newline=self.buffer.find(b'\n')
            if second>=0 and (newline<0 or second<newline):
                self.errors+=1; del self.buffer[:second]; continue
            if newline<0:
                if len(self.buffer)>512:
                    self.errors+=1; self.buffer.clear()
                break
            line=bytes(self.buffer[:newline]); del self.buffer[:newline+1]
            try:
                self.accept(parse(line),host)
            except ValueError:
                self.errors+=1

    def finish(self):
        if self.buffer or not self.rows or self.errors or self.discarded or self.blocked:
            raise ValueError('incomplete, corrupt, reset, or empty replay')

    def fresh(self,now):
        return not self.blocked and self.progress is not None and now-self.progress<1

    def export(self,path,now):
        result={'author':'jayis1','device':'Tip Cadence','source':self.source,
                'fresh':self.fresh(now) if self.source=='live' and self.connected else False,
                'connected':self.connected,
                'errors':self.errors,'discarded_bytes':self.discarded,'blocked':self.blocked,
                'warning':self.warning,'fields':FIELDS,'rows':list(self.rows)}
        fd=os.open(path,os.O_WRONLY|os.O_CREAT|os.O_EXCL,0o600)
        with os.fdopen(fd,'w') as out:
            json.dump(result,out,indent=2); out.write('\n')

def load_replay(path):
    fd=os.open(path,os.O_RDONLY|os.O_NONBLOCK)
    with os.fdopen(fd,'rb') as stream:
        info=os.fstat(stream.fileno())
        if not stat.S_ISREG(info.st_mode) or info.st_size>MAX_REPLAY:
            raise ValueError('replay must be a regular file of at most 2 MiB')
        data=stream.read(MAX_REPLAY+1)
        if len(data)>MAX_REPLAY:
            raise ValueError('replay grew past limit')
    model=Model('replay'); model.feed(data,time.monotonic()); model.finish()
    return model

class Serial:
    def __init__(self,path):
        self.fd=os.open(path,os.O_RDWR|os.O_NOCTTY|os.O_NONBLOCK)
        self.old=None
        try:
            self.old=termios.tcgetattr(self.fd)
            mode=termios.tcgetattr(self.fd)
            mode[0]=0; mode[1]=0; mode[2]=termios.CS8|termios.CREAD|termios.CLOCAL
            mode[3]=0; mode[4]=termios.B115200; mode[5]=termios.B115200
            mode[6][termios.VMIN]=0; mode[6][termios.VTIME]=0
            termios.tcsetattr(self.fd,termios.TCSANOW,mode)
            termios.tcflush(self.fd,termios.TCIFLUSH)
        except BaseException:
            self.close(); raise
        self.poller=select.poll()
        self.poller.register(self.fd,select.POLLIN|select.POLLHUP|select.POLLERR)

    def read(self):
        if self.fd is None:
            raise OSError('serial port is closed')
        events=self.poller.poll(0)
        data=b''; hung=False
        for _,flags in events:
            if flags&select.POLLIN:
                try: data+=os.read(self.fd,4096)
                except BlockingIOError: pass
            hung=bool(flags&(select.POLLHUP|select.POLLERR|select.POLLNVAL))
        return data,hung

    def close(self):
        if self.fd is None: return
        try:
            if self.old is not None: termios.tcsetattr(self.fd,termios.TCSANOW,self.old)
        except (OSError,termios.error): pass
        os.close(self.fd); self.fd=None

class App:
    def __init__(self,root):
        self.root=root; self.serial=None; self.model=Model(); self.closed=False
        root.title('Tip Cadence — jayis1')
        self.path=tk.StringVar(value='/dev/ttyACM0')
        self.status=tk.StringVar(value='Disconnected; no physical measurements')
        self.summary=tk.StringVar(value='No observations')
        tabs=ttk.Notebook(root); tabs.pack(fill='both',expand=True)
        setup=ttk.Frame(tabs); live=ttk.Frame(tabs); evidence=ttk.Frame(tabs)
        for frame,title in ((setup,'Connect'),(live,'Cadence'),(evidence,'Evidence')):
            tabs.add(frame,text=title)
        ttk.Label(setup,text='3.3 V dry contacts only. No medical or control use.').pack()
        ttk.Entry(setup,textvariable=self.path,width=60).pack()
        ttk.Button(setup,text='Connect (new session)',command=self.connect).pack()
        ttk.Button(setup,text='Disconnect',command=self.disconnect).pack()
        ttk.Label(live,textvariable=self.summary,justify='left').pack(padx=20,pady=20)
        ttk.Label(evidence,text='Exports retain the last 600 snapshots and all integrity warnings.').pack()
        ttk.Button(evidence,text='Replay raw TC1 recording',command=self.replay).pack()
        ttk.Button(evidence,text='Export private JSON (new file)',command=self.export).pack()
        ttk.Label(root,textvariable=self.status).pack()
        root.protocol('WM_DELETE_WINDOW',self.close)
        self.timer=root.after(20,self.poll)

    def connect(self):
        if self.serial is not None:
            self.status.set('Already connected; disconnect first'); return
        try:
            serial=Serial(self.path.get())
        except (OSError,termios.error) as exc:
            self.status.set(str(exc)); return
        self.serial=serial; self.model=Model('live'); self.summary.set('Awaiting TC1 identity')
        self.model.connected=True

    def disconnect(self):
        if self.serial is not None: self.serial.close()
        self.model.connected=False
        self.serial=None; self.status.set('disconnected — evidence retained')

    def replay_path(self,path):
        replacement=load_replay(path)
        self.disconnect(); self.model=replacement; self.render()

    def replay(self):
        path=filedialog.askopenfilename()
        if not path: return
        try: self.replay_path(path)
        except (OSError,ValueError) as exc: self.status.set(str(exc))

    def export(self):
        path=filedialog.asksaveasfilename(defaultextension='.json')
        if not path: return
        try:
            self.model.export(path,time.monotonic()); self.status.set('Exported '+path)
        except (OSError,ValueError) as exc: self.status.set(str(exc))

    def render(self):
        m=self.model
        if not m.rows: return
        row=m.rows[-1]; lines=[]
        for lane in ('A','B'):
            v={k:row[lane+'_'+k] for k in LANE_FIELDS}
            means=[v[f'{p}_sum_ms']/v[f'{p}_count'] if v[f'{p}_count'] else None for p in ('even','odd')]
            comparable=not (v['breaks'] or v['rejected'] or row['gaps'] or row['faults'])
            even,odd=means
            contrast=f'{even-odd:.1f} ms' if comparable and even is not None and odd is not None else 'unavailable'
            lines.append(f"{lane}: {v['tips']} tips, {v['intervals']} intervals; phase {v['phase']}\n"
                         f"  range {v['min_ms']}..{v['max_ms']} ms; max adjacent change {v['jitter_ms']} ms\n"
                         f"  relative even-minus-odd: {contrast}; rejected {v['rejected']}, breaks {v['breaks']}")
        self.summary.set('\n'.join(lines))
        state='replay (not live)' if m.source=='replay' else ('live' if self.serial and m.fresh(time.monotonic()) else 'stale/disconnected')
        self.status.set(f'{state}; gaps={row["gaps"]}; faults={row["faults"]}; parse errors={m.errors}; {m.warning}')

    def poll(self):
        if self.closed: return
        if self.serial is not None:
            try:
                data,hung=self.serial.read()
                if data: self.model.feed(data,time.monotonic())
                if hung: self.disconnect()
            except (OSError,termios.error) as exc:
                self.disconnect(); self.model.warning=str(exc)
        self.render(); self.timer=self.root.after(20,self.poll)

    def close(self):
        if self.closed: return
        self.closed=True; self.root.after_cancel(self.timer)
        self.disconnect(); self.root.destroy()

if __name__=='__main__':
    window=tk.Tk(); App(window); window.mainloop()
