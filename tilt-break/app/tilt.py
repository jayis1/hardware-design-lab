#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Local receive-only Linux companion. No simulated readings in live mode."""
import binascii
import csv
import math
import os
from pathlib import Path
import re
import select
import stat
import termios
import time
import tkinter as tk
from tkinter import ttk, filedialog, messagebox

FIELDS = ('seq','ms','state','faults','valid','adc0','adc1','adc2','adc3',
          'baseline','height','elapsed','release','lo','hi','transit_ms')
STATES = ('WAIT','ZERO','READY','RAMP','SLIDE','DONE','INVALID')
LIMIT = 2*1024*1024

def decode(line):
    if len(line)>512 or not line.endswith(b'\n'):
        raise ValueError('Incomplete or oversized frame')
    try:
        body,checksum=line.rstrip(b'\n').rsplit(b',',1)
        if not re.fullmatch(b'[0-9A-F]{4}',checksum):
            raise ValueError('CRC format')
        if binascii.crc_hqx(body,0xffff)!=int(checksum,16):
            raise ValueError('CRC mismatch')
        cells=body.split(b',')
        if cells[0]!=b'TB1' or len(cells)!=17:
            raise ValueError('Unsupported protocol')
        if any(not re.fullmatch(b'-?[0-9]{1,10}',c) for c in cells[1:]):
            raise ValueError('Nondecimal field')
        row: dict[str,int]=dict(zip(FIELDS,map(int,cells[1:])))
    except (UnicodeError,OverflowError) as exc:
        raise ValueError('Malformed frame') from exc
    for key in FIELDS:
        low=-200000 if key in ('height','lo','hi') else 0
        if not low<=row[key]<=0xffffffff:
            raise ValueError('Field range')
    if row['state']>6 or row['faults']>1023 or row['valid']>1 or row['release']>1:
        raise ValueError('Enum range')
    if (row['state']==6)!=(row['faults']!=0):
        raise ValueError('Fault/state disagreement')
    if any(row[f'adc{i}']>4095 for i in range(4)):
        raise ValueError('ADC range')
    if row['baseline']>128000 or row['elapsed']>120025 or row['transit_ms']>10000:
        raise ValueError('Measurement range')
    if row['state'] in (4,5) and not row['release']:
        raise ValueError('Missing release')
    if row['release'] and not -200<=row['lo']<=row['hi']<=88000:
        raise ValueError('Release bracket')
    if not row['release'] and any(row[k] for k in ('lo','hi','transit_ms')):
        raise ValueError('Phantom release')
    return row

class Model:
    def __init__(self):
        self.rows=[]
        self.source='none'
        self.errors=0
        self.blocked=False
        self.warning='No evidence'
        self.last_host=None
        self.anchor_host=None
        self.device_elapsed=0
        self.connected=False

    def check_age(self,now):
        if self.source=='live' and self.last_host is not None and now-self.last_host>2:
            self.blocked=True
            self.warning='Stale evidence; reconnect required'

    def accept(self,line,now=None):
        now=time.monotonic() if now is None else now
        self.check_age(now)
        if self.blocked:
            return
        try:
            row=decode(line)
            if self.rows:
                old=self.rows[-1]
                ds=(row['seq']-old['seq']) & 0xffffffff
                dt=(row['ms']-old['ms']) & 0xffffffff
                if ds==0:
                    if row!=old: raise ValueError('Contradictory duplicate')
                    return
                if not 0<ds<0x80000000 or not 0<dt<0x80000000:
                    raise ValueError('Possible reset or reordered data')
                if row['faults'] & old['faults'] != old['faults']:
                    raise ValueError('Fault rollback')
                if row['state']<old['state'] and (old['state'],row['state'])!=(1,0):
                    raise ValueError('State rollback')
                if old['state']>=2 and row['baseline']!=old['baseline']:
                    raise ValueError('Baseline changed')
                if old['release'] and any(row[k]!=old[k] for k in ('release','lo','hi')):
                    raise ValueError('Release evidence changed')
                if old['state'] in (5,6) and (row['state'] not in (old['state'],6) or
                    any(row[k]!=old[k] for k in ('height','elapsed','transit_ms'))):
                    raise ValueError('Terminal evidence changed')
                self.device_elapsed+=dt/1000
                assert self.anchor_host is not None
                if self.source=='live' and abs(self.device_elapsed-(now-self.anchor_host))>2:
                    raise ValueError('Device/host progress divergence')
            else:
                self.anchor_host=now
            self.rows.append(row)
            self.rows=self.rows[-2000:]
            self.last_host=now
            self.warning='Retained terminal evidence' if row['state'] in (5,6) else 'Uncalibrated prototype'
        except ValueError as exc:
            self.errors+=1
            self.warning=str(exc)
            self.blocked=True
            raise

    def replay(self,path):
        fd=os.open(path,os.O_RDONLY|os.O_NONBLOCK)
        try:
            info=os.fstat(fd)
            if not stat.S_ISREG(info.st_mode) or info.st_size>LIMIT:
                raise ValueError('Replay requires a regular file <=2 MiB')
            with os.fdopen(fd,'rb',closefd=False) as stream:
                data=stream.read(LIMIT+1)
        finally:
            os.close(fd)
        if len(data)>LIMIT or not data or not data.endswith(b'\n'):
            raise ValueError('Empty, oversized or incomplete replay')
        self.source='replay'
        for line in data.splitlines(keepends=True):
            self.accept(line)
        if not self.rows:
            raise ValueError('No snapshots')

    def export(self,path,notes,slope,run):
        # JSON-like text in the first CSV cell prevents formula evaluation.
        import json
        metadata={'author':'jayis1','source':self.source,'errors':self.errors,
                  'blocked':self.blocked,'connected':self.connected,'warning':self.warning,
                  'age_s':None if self.last_host is None else time.monotonic()-self.last_host,
                  'ohms_per_mm':slope,'horizontal_run_mm':run,'notes':notes,
                  'calibration':'operator supplied; physical accuracy unverified'}
        fd=os.open(path,os.O_WRONLY|os.O_CREAT|os.O_EXCL,0o600)
        with os.fdopen(fd,'w',newline='') as stream:
            writer=csv.writer(stream)
            writer.writerow(['metadata',json.dumps(metadata)])
            writer.writerow(FIELDS)
            for row in self.rows:
                writer.writerow([row[k] for k in FIELDS])

class Serial:
    def __init__(self,path):
        self.fd=os.open(path,os.O_RDWR|os.O_NOCTTY|os.O_NONBLOCK)
        self.original=None
        try:
            self.original=termios.tcgetattr(self.fd)
            cfg=termios.tcgetattr(self.fd)
            cfg[0]=cfg[1]=cfg[3]=0
            cfg[2]=termios.CS8|termios.CREAD|termios.CLOCAL
            cfg[4]=cfg[5]=termios.B115200
            cfg[6][termios.VMIN]=0; cfg[6][termios.VTIME]=0
            termios.tcsetattr(self.fd,termios.TCSANOW,cfg)
            termios.tcflush(self.fd,termios.TCIFLUSH)
            self.poller=select.poll()
            self.poller.register(self.fd,select.POLLIN|select.POLLHUP|select.POLLERR)
        except Exception:
            self.close()
            raise

    def read(self):
        if self.fd is None:
            raise OSError('Serial closed')
        result=b''
        for _,event in self.poller.poll(0):
            if event & select.POLLIN:
                try: result=os.read(self.fd,8192)
                except BlockingIOError: pass
            if event & (select.POLLHUP|select.POLLERR):
                if result: return result,True
                raise OSError('Serial disconnected')
        return result,False

    def close(self):
        if self.fd is not None:
            try:
                if self.original is not None:
                    termios.tcsetattr(self.fd,termios.TCSANOW,self.original)
            except (OSError,termios.error): pass
            os.close(self.fd)
            self.fd=None

class App:
    def __init__(self,root):
        self.root=root
        root.title('Tilt Break — jayis1')
        self.model=Model(); self.serial=None; self.buffer=b''; self.closed=False
        self.port=tk.StringVar(value='/dev/ttyACM0')
        self.status=tk.StringVar(value='No evidence')
        self.result=tk.StringVar(value='No release captured')
        self.slope=tk.StringVar(value='2222.222')
        self.run=tk.StringVar(value='100')
        self.notes=tk.StringVar()
        self.tabs=ttk.Notebook(root); self.tabs.pack(fill='both',expand=True)
        live=ttk.Frame(self.tabs); calibration=ttk.Frame(self.tabs); evidence=ttk.Frame(self.tabs)
        for frame,name in ((live,'Connection / live'),(calibration,'Geometry'),(evidence,'Evidence')):
            self.tabs.add(frame,text=name)
        ttk.Entry(live,textvariable=self.port,width=50).pack()
        for label,command in (('Connect',self.connect),('Disconnect',self.disconnect),('Replay file',self.replay)):
            ttk.Button(live,text=label,command=command).pack()
        ttk.Label(live,textvariable=self.status,wraplength=650).pack()
        ttk.Label(live,textvariable=self.result,wraplength=650).pack()
        self.canvas=tk.Canvas(live,width=640,height=160,bg='white'); self.canvas.pack()
        for text,var in (('Measured ohms per millimeter',self.slope),('Fixed horizontal support distance (mm)',self.run)):
            ttk.Label(calibration,text=text).pack(); ttk.Entry(calibration,textvariable=var).pack()
        ttk.Label(calibration,text='Defaults are nominal, NOT calibrated.\nAngle = atan(height / horizontal distance).\nReed departure is not the first microscopic motion.').pack()
        ttk.Label(evidence,text='Operator notes').pack(); ttk.Entry(evidence,textvariable=self.notes,width=70).pack()
        ttk.Button(evidence,text='Export new private CSV',command=self.export).pack()
        self.table=tk.Text(evidence,width=90,height=18); self.table.pack()
        root.protocol('WM_DELETE_WINDOW',self.close)
        self.job=root.after(50,self.poll)

    def connect(self):
        # Never open the same tty twice: closing the old owner would restore
        # termios over the new owner's settings. Require explicit disconnect.
        if self.serial is not None:
            self.status.set('Disconnect the current port before connecting again')
            return
        try:
            candidate=Serial(self.port.get())
        except (OSError,termios.error) as exc:
            self.status.set(str(exc)); return
        self.disconnect()
        self.serial=candidate; self.buffer=b''
        self.model=Model(); self.model.source='live'; self.model.connected=True
        self.render()

    def disconnect(self):
        if self.serial is not None:
            self.serial.close(); self.serial=None
        self.model.connected=False
        self.buffer=b''

    def replay(self):
        path=filedialog.askopenfilename()
        if not path: return
        try:
            candidate=Model(); candidate.replay(path)
        except (OSError,ValueError) as exc:
            messagebox.showerror('Replay rejected',str(exc)); return
        self.disconnect(); self.model=candidate; self.render()

    def geometry(self):
        slope=float(self.slope.get()); run=float(self.run.get())
        if not math.isfinite(slope) or not math.isfinite(run) or slope<=0 or run<=0:
            raise ValueError('Positive finite geometry required')
        return slope,run

    def export(self):
        path=filedialog.asksaveasfilename(defaultextension='.csv')
        if not path: return
        try:
            slope,run=self.geometry(); self.model.check_age(time.monotonic())
            self.model.export(path,self.notes.get(),slope,run)
        except (ValueError,OSError) as exc:
            messagebox.showerror('Export failed',str(exc))

    def render(self):
        row=self.model.rows[-1] if self.model.rows else None
        state=STATES[row['state']] if row else 'NO DATA'
        link='connected' if self.model.connected else 'disconnected'
        self.status.set(f'{self.model.source} / {link} / {state}: {self.model.warning}' +
                        (f"; faults={row['faults']} acquisition_valid={row['valid']}" if row else ''))
        self.result.set('No release captured')
        if row and row['release']:
            try:
                slope,run=self.geometry()
                angles=[math.degrees(math.atan(row[k]/slope/run)) for k in ('lo','hi')]
                self.result.set(f'Nominal departure bracket {angles[0]:.3f} to {angles[1]:.3f} degrees; '
                                f"gate transit {row['transit_ms']} ms (zero means not completed).")
            except ValueError:
                self.result.set('Invalid geometry; raw resistance remains available')
        self.canvas.delete('all')
        points=[]
        for i,r in enumerate(self.model.rows[-200:]):
            points.extend((10+i*3,145-min(130,max(0,r['height']/700))))
        if len(points)>=4: self.canvas.create_line(*points,fill='navy')
        self.canvas.create_text(320,10,text='Retained height-resistance history (not a live validity indicator)')
        self.table.delete('1.0','end')
        self.table.insert('end',' '.join(FIELDS)+'\n')
        for r in self.model.rows[-100:]:
            self.table.insert('end',' '.join(str(r[k]) for k in FIELDS)+'\n')

    def poll(self):
        if self.closed: return
        try:
            if self.serial is not None:
                data,hangup=self.serial.read()
                self.buffer+=data
                if len(self.buffer)>16384:
                    raise ValueError('Serial buffer overflow')
                while b'\n' in self.buffer:
                    line,self.buffer=self.buffer.split(b'\n',1)
                    # Recover full markers after an incomplete UART transmission.
                    start=line.rfind(b'TB1,')
                    if start<0 and not self.model.rows:
                        # A late attachment may start inside a transmitted frame.
                        # Count discarded fragments; validate strictly once aligned.
                        self.model.errors+=1
                        continue
                    if start>0: self.model.errors+=1; line=line[start:]
                    self.model.accept(line+b'\n')
                if hangup: raise OSError('Serial disconnected')
            self.model.check_age(time.monotonic())
        except (OSError,ValueError) as exc:
            self.model.warning=str(exc); self.model.blocked=True; self.disconnect()
        self.render()
        self.job=self.root.after(50,self.poll)

    def close(self):
        self.closed=True
        self.root.after_cancel(self.job)
        self.disconnect(); self.root.destroy()

if __name__=='__main__':
    root=tk.Tk(); App(root); root.mainloop()
