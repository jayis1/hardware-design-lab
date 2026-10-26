#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Local Linux serial notebook. No server, radio, package installation or telemetry."""
import binascii
import csv
import json
import os
from pathlib import Path
import select
import stat
import termios
import time
import tkinter as tk
from tkinter import ttk, filedialog, messagebox

NEVER=4294967295
STATES=('BASELINE','READY','LOADED','RECOVERY','DONE','INVALID')
MAX_FILE=2*1024*1024

def ohms(n):
    return max(0,100000*n//(4095-n)-1000) if n<4095 else NEVER

def parse(line):
    if len(line)>512 or not line.endswith(b'\n'):
        raise ValueError('oversize or truncated frame')
    try:
        body,crc=line.rstrip(b'\r\n').rsplit(b',',1)
        if len(crc)!=4 or binascii.crc_hqx(body,65535)!=int(crc,16):
            raise ValueError('CRC mismatch')
        fields=body.decode('ascii').split(',')
        if len(fields)!=37 or fields[0]!='C1':
            raise ValueError('unsupported protocol')
        if any(not p.isascii() or not p.isdecimal() or len(p)>10 for p in fields[1:]):
            raise ValueError('invalid integer')
        v=list(map(int,fields[1:]))
        if any(n>NEVER for n in v):
            raise ValueError('integer overflow')
        seq,now,fault,acquired=v[:4]; adc=v[4:8]
        if fault>31 or acquired>1 or any(n>4095 for n in adc):
            raise ValueError('invalid status')
        if acquired and any(not 80<=n<=2300 for n in adc[:2]):
            raise ValueError('invalid acquisition')
        lanes=[v[8:22],v[22:36]]
        for p in lanes:
            state,contact,base,hold,recover,marks,rmarks,residual=p[:8]
            if state>5 or contact>1 or base>ohms(2300) or hold>180025 or recover>60000:
                raise ValueError('invalid lane')
            if fault and state!=5:
                raise ValueError('fault without invalid state')
            if state==1 and contact:
                raise ValueError('ready while loaded')
            for elapsed,mask,values in [(hold,marks,p[8:11]),(recover,rmarks,p[11:14])]:
                if mask not in (0,1,3,7):
                    raise ValueError('non-prefix milestones')
                for i,deadline in enumerate((1000,10000,60000)):
                    if mask&(1<<i):
                        if elapsed<deadline or values[i]>ohms(2300):
                            raise ValueError('impossible milestone')
                    elif values[i]!=NEVER:
                        raise ValueError('unearned milestone')
            if state in (3,4) and (marks!=7 or hold>180000):
                raise ValueError('recovery without load history')
            if state==4 and (recover!=60000 or rmarks!=7 or residual!=p[13]):
                raise ValueError('invalid final residual')
        return dict(sequence=seq,now=now,faults=fault,acquired=acquired,adc=adc,lanes=lanes)
    except (UnicodeError,IndexError,OverflowError) as exc:
        raise ValueError('malformed frame') from exc

def load_replay(path):
    fd=os.open(path,os.O_RDONLY|os.O_NONBLOCK)
    try:
        s=os.fstat(fd)
        if not stat.S_ISREG(s.st_mode) or s.st_size>MAX_FILE:
            raise ValueError('Replay must be a regular file at most 2 MiB')
        data=b''
        while len(data)<=MAX_FILE:
            chunk=os.read(fd,min(65536,MAX_FILE+1-len(data)))
            if not chunk: break
            data+=chunk
        if len(data)>MAX_FILE or not data:
            raise ValueError('empty or oversized replay')
        return [parse(line) for line in data.splitlines(keepends=True)]
    finally:
        os.close(fd)

class App:
    def __init__(self,root):
        self.root=root; root.title('Creep Pair — jayis1')
        self.fd=None; self.saved=None; self.buffer=b''; self.rows=[]
        self.provenance='none'; self.errors=0; self.last_host=None
        self.first_host=None; self.device_elapsed=0; self.blocked=False
        self.port=tk.StringVar(value='/dev/ttyACM0')
        self.status=tk.StringVar(value='disconnected — no capture')
        self.live=tk.StringVar(value='No measurements')
        self.compare=tk.StringVar(value='No completed specimens')
        self.scale=tk.StringVar(value='10000')
        self.note=tk.StringVar(value='')
        self.tabs=ttk.Notebook(root); self.tabs.pack(fill='both',expand=True)
        live=ttk.Frame(self.tabs); comparison=ttk.Frame(self.tabs); evidence=ttk.Frame(self.tabs)
        for widget,title in [(live,'Connection / live'),(comparison,'Comparison'),(evidence,'Evidence')]:
            self.tabs.add(widget,text=title)
        ttk.Entry(live,textvariable=self.port,width=45).pack()
        for label,command in [('Connect',self.connect),('Disconnect',self.disconnect),('Replay',self.replay)]:
            ttk.Button(live,text=label,command=command).pack()
        ttk.Label(live,textvariable=self.status).pack()
        ttk.Label(live,textvariable=self.live,justify='left').pack(padx=15,pady=15)
        ttk.Label(comparison,text='Calibration ohms/mm (signed, nonzero); same for both lanes').pack()
        ttk.Entry(comparison,textvariable=self.scale).pack()
        ttk.Button(comparison,text='Recompute',command=self.refresh).pack()
        ttk.Label(comparison,textvariable=self.compare,justify='left').pack(padx=15,pady=15)
        ttk.Label(evidence,text='Specimen / mass / conditioning notes (exported locally)').pack()
        ttk.Entry(evidence,textvariable=self.note,width=65).pack()
        ttk.Button(evidence,text='Export new CSV',command=self.export).pack()
        ttk.Button(evidence,text='Clear retained evidence',command=self.clear).pack()
        self.log=tk.Text(evidence,height=15,width=85,state='disabled'); self.log.pack()
        root.protocol('WM_DELETE_WINDOW',self.close)
        root.after(30,self.poll)

    def clear(self):
        self.disconnect(); self.rows=[]; self.errors=0; self.buffer=b''
        self.last_host=None; self.first_host=None; self.device_elapsed=0; self.blocked=False
        self.provenance='none'; self.live.set('No measurements'); self.compare.set('No completed specimens')
        self.refresh_log()

    def connect(self):
        self.disconnect()
        fd=None; saved=None
        try:
            fd=os.open(self.port.get(),os.O_RDWR|os.O_NOCTTY|os.O_NONBLOCK)
            saved=termios.tcgetattr(fd); config=termios.tcgetattr(fd)
            config[0]=0; config[1]=0; config[2]=termios.CS8|termios.CREAD|termios.CLOCAL
            config[3]=0; config[4]=termios.B115200; config[5]=termios.B115200
            config[6][termios.VMIN]=0; config[6][termios.VTIME]=0
            termios.tcsetattr(fd,termios.TCSANOW,config); termios.tcflush(fd,termios.TCIFLUSH)
            self.clear()
            self.fd=fd; self.saved=saved; self.provenance='live'
            self.status.set('connected — waiting for C1')
        except (OSError,termios.error) as exc:
            if fd is not None:
                if saved is not None:
                    try: termios.tcsetattr(fd,termios.TCSANOW,saved)
                    except (OSError,termios.error): pass
                os.close(fd)
            self.status.set(f'disconnected: {exc}')

    def disconnect(self):
        if self.fd is not None:
            try:
                if self.saved is not None: termios.tcsetattr(self.fd,termios.TCSANOW,self.saved)
            except (OSError,termios.error): pass
            os.close(self.fd); self.fd=None
        self.status.set('disconnected — retained evidence is not live')

    def accept(self,row):
        if self.blocked: raise ValueError('capture blocked; reconnect explicitly')
        if self.rows:
            old=self.rows[-1]
            delta=(row['sequence']-old['sequence'])&NEVER
            dt=(row['now']-old['now'])&NEVER
            if delta==0 and row==old: return
            if delta==0 or delta>=2147483648 or dt==0 or dt>=2147483648:
                raise ValueError('reset, duplicate contradiction or time rollback')
            if row['faults']|old['faults']!=row['faults']:
                raise ValueError('sticky fault disappeared')
            for a,b in zip(old['lanes'],row['lanes']):
                if a[0]!=0 and a[2]!=b[2]:
                    raise ValueError('established baseline changed')
                if b[0]<a[0] or b[3]<a[3] or b[4]<a[4]:
                    raise ValueError('lane history regressed')
                for maskindex,start in [(5,8),(6,11)]:
                    if a[maskindex]|b[maskindex]!=b[maskindex]:
                        raise ValueError('milestone disappeared')
                    for j in range(3):
                        if a[maskindex]&(1<<j) and a[start+j]!=b[start+j]:
                            raise ValueError('milestone changed')
            self.device_elapsed+=dt
        now=time.monotonic()
        if self.provenance=='live':
            if self.last_host is not None and now-self.last_host>2:
                raise ValueError('host reception pause; reconnect and reset experiment')
            if self.first_host is not None and abs((now-self.first_host)*1000-self.device_elapsed)>2000:
                raise ValueError('device/host backlog or clock discontinuity')
        if self.first_host is None: self.first_host=now
        self.last_host=now; self.rows.append(row); self.rows=self.rows[-2000:]
        self.refresh()

    def poll(self):
        if self.fd is not None:
            try:
                p=select.poll(); p.register(self.fd,select.POLLIN|select.POLLHUP|select.POLLERR)
                events=p.poll(0)
                flags=events[0][1] if events else 0
                if flags&select.POLLIN:
                    self.buffer+=os.read(self.fd,8192)
                    while b'\n' in self.buffer:
                        line,self.buffer=self.buffer.split(b'\n',1)
                        # Recover from a truncated predecessor only at the complete marker.
                        index=line.rfind(b'C1,')
                        if index>0: self.errors+=1; line=line[index:]
                        self.accept(parse(line+b'\n'))
                    if len(self.buffer)>512: raise ValueError('unterminated frame')
                if flags&(select.POLLHUP|select.POLLERR): self.disconnect()
                elif self.last_host is not None and time.monotonic()-self.last_host>2:
                    raise ValueError('stale stream; reconnect explicitly')
            except (OSError,ValueError,termios.error) as exc:
                self.errors+=1; self.blocked=True; self.disconnect()
                self.status.set(f'disconnected: {exc}')
        self.root.after(30,self.poll)

    def refresh(self):
        if not self.rows: return
        r=self.rows[-1]
        lines=[f"{self.provenance} | faults={r['faults']} | ADC valid={r['acquired']}"]
        for i,p in enumerate(r['lanes']):
            raw=str(r['adc'][i]) if r['acquired'] else 'INVALID'
            lines.append(f"{'AB'[i]}: {STATES[p[0]]} raw={raw} hold={p[3]} ms recovery={p[4]} ms")
        self.live.set('\n'.join(lines))
        try:
            scale=float(self.scale.get())
            if not -1e9<=scale<=1e9 or abs(scale)<.001: raise ValueError()
            results=[]
            for i,p in enumerate(r['lanes']):
                if p[0]==4:
                    results.append((i,(p[7]-p[2])/scale))
            text='\n'.join(f"{'AB'[i]} residual relative to baseline: {v:.4f} mm" for i,v in results)
            if len(results)==2: text+=f'\nA minus B: {results[0][1]-results[1][1]:.4f} mm'
            self.compare.set((text or 'No completed specimens')+'\nAssumed calibration; not a certified material property.')
        except ValueError: self.compare.set('Invalid calibration: enter finite signed ohms/mm')
        self.refresh_log()

    def refresh_log(self):
        self.log.configure(state='normal'); self.log.delete('1.0','end')
        for r in self.rows[-100:]:
            self.log.insert('end',f"{r['sequence']} {r['now']} ms | A {STATES[r['lanes'][0][0]]} B {STATES[r['lanes'][1][0]]}\n")
        self.log.configure(state='disabled')

    def replay(self):
        path=filedialog.askopenfilename()
        if not path: return
        try:
            records=load_replay(path); self.clear(); self.provenance='replay'
            for row in records: self.accept(row)
            self.status.set('replay — synthetic or recorded file; not live hardware')
        except (ValueError,OSError) as exc:
            self.blocked=True; self.errors+=1; self.status.set(f'replay failed: {exc}')

    def export_to(self,path):
        fd=os.open(path,os.O_WRONLY|os.O_CREAT|os.O_EXCL,0o600)
        with os.fdopen(fd,'w',newline='') as f:
            w=csv.writer(f)
            w.writerow(['author','jayis1','provenance',self.provenance,'errors',self.errors,'blocked',self.blocked,
                        'status',self.status.get(),'age_seconds',None if self.last_host is None else time.monotonic()-self.last_host,
                        'ohms_per_mm',self.scale.get(),'notes',self.note.get()])
            w.writerow(['sequence','device_ms','faults','acquired','adc','lanes'])
            for r in self.rows:
                w.writerow([r['sequence'],r['now'],r['faults'],r['acquired'],json.dumps(r['adc']),json.dumps(r['lanes'])])

    def export(self):
        path=filedialog.asksaveasfilename(defaultextension='.csv')
        if not path: return
        try: self.export_to(path)
        except OSError as exc: messagebox.showerror('Export failed',str(exc))

    def close(self):
        self.disconnect(); self.root.destroy()

if __name__=='__main__':
    root=tk.Tk(); App(root); root.mainloop()
