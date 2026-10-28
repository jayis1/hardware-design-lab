#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Detent Loop: local Linux acquisition, loop plot and evidence export."""
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

STATES=('WAIT','ZERO','READY','OUT','BACK','DONE','INVALID')
FIELDS=('sequence','now','state','faults','acquired','adc0','adc1','adc2','adc3',
        'base0','base1','x_um','force_mn','elapsed','span_um','peak_out_mn',
        'peak_back_mn','out_uj','back_uj')
SUMMARY=('base0','base1','elapsed','span_um','peak_out_mn','peak_back_mn','out_uj','back_uj')
MAX_FILE=2*1024*1024
U32=4294967295

def parse(line):
    if not line.endswith(b'\n') or len(line)>512:
        raise ValueError('oversize or truncated record')
    try:
        body,check=line.rstrip(b'\r\n').rsplit(b',',1)
        if len(check)!=4 or binascii.crc_hqx(body,65535)!=int(check,16):
            raise ValueError('CRC mismatch')
        values=body.decode('ascii').split(',')
        if values[0]!='D1' or len(values)!=20:
            raise ValueError('unsupported wire version or field count')
        for s in values[1:]:
            if not s or len(s)>12 or not s.lstrip('-').isdecimal() or s.count('-')>1 or '-' in s[1:]:
                raise ValueError('invalid decimal field')
        r: dict[str,int]=dict(zip(FIELDS,map(int,values[1:])))
        bounds={
            'sequence':(0,U32),'now':(0,U32),'state':(0,6),'faults':(0,511),
            'acquired':(0,1),'base0':(0,127133),'base1':(0,127133),
            'x_um':(-100000,100000),'force_mn':(-300000,300000),
            'elapsed':(0,120025),'span_um':(0,20000),'peak_out_mn':(0,20000),
            'peak_back_mn':(-20000,0),'out_uj':(-241000000,241000000),
            'back_uj':(-241000000,241000000)}
        bounds.update({f'adc{i}':(0,4095) for i in range(4)})
        if any(not low<=r[key]<=high for key,(low,high) in bounds.items()):
            raise ValueError('field outside instrument limits')
        if bool(r['faults']) != (r['state']==6):
            raise ValueError('fault/state contradiction')
        if r['acquired'] and any(not 80<=r[f'adc{i}']<=2300 for i in range(2)):
            raise ValueError('acquisition/range contradiction')
        if r['state'] in (2,3,4,5):
            if not -500<=r['x_um']<=20000 or abs(r['force_mn'])>20000:
                raise ValueError('valid state outside mechanical limits')
        if r['state']<3 and any(r[k] for k in SUMMARY[2:]):
            raise ValueError('experiment summary before start')
        if r['state'] in (4,5) and r['span_um']<5000:
            raise ValueError('return without outward span')
        if r['state']==5 and (abs(r['x_um'])>500 or abs(r['force_mn'])>2000):
            raise ValueError('completion away from unloaded home')
        if r['state']<2 and (r['base0'] or r['base1']):
            raise ValueError('baseline before qualification')
        return r
    except (UnicodeError,OverflowError,IndexError) as exc:
        raise ValueError('malformed wire record') from exc

def load_replay(path):
    fd=os.open(path,os.O_RDONLY|os.O_NONBLOCK)
    try:
        info=os.fstat(fd)
        if not stat.S_ISREG(info.st_mode) or info.st_size>MAX_FILE:
            raise ValueError('select a regular recording file at most 2 MiB')
        data=b''
        while len(data)<=MAX_FILE:
            part=os.read(fd,min(65536,MAX_FILE+1-len(data)))
            if not part:break
            data+=part
        if not data or len(data)>MAX_FILE:
            raise ValueError('empty or oversized recording')
        return [parse(line) for line in data.splitlines(keepends=True)]
    finally:
        os.close(fd)

class App:
    def __init__(self,root):
        self.root=root
        root.title('Detent Loop — jayis1')
        self.fd=None
        self.saved=None
        self.rows=[]
        self.buffer=b''
        self.errors=0
        self.provenance='none'
        self.blocked=False
        self.last_host=None
        self.first_host=None
        self.device_elapsed=0
        self.connected_at=None
        self.port=tk.StringVar(value='/dev/ttyACM0')
        self.status=tk.StringVar(value='disconnected')
        self.live=tk.StringVar(value='No measurements')
        self.result=tk.StringVar(value='No loop captured')
        self.note=tk.StringVar(value='')
        self.tabs=ttk.Notebook(root)
        self.tabs.pack(fill='both',expand=True)
        live=ttk.Frame(self.tabs)
        loop=ttk.Frame(self.tabs)
        evidence=ttk.Frame(self.tabs)
        for widget,title in [(live,'Connection / live'),(loop,'Force–travel loop'),(evidence,'Evidence')]:
            self.tabs.add(widget,text=title)
        ttk.Entry(live,textvariable=self.port,width=55).pack()
        for label,command in [('Connect',self.connect),('Disconnect',self.disconnect),('Replay recording',self.replay)]:
            ttk.Button(live,text=label,command=command).pack()
        ttk.Label(live,textvariable=self.status).pack(pady=10)
        ttk.Label(live,textvariable=self.live,justify='left').pack(padx=20,pady=20)
        ttk.Label(live,text='Hold ARM to zero; release; press ARM to start.\nMove outward; release ARM; press TURN. Return; release TURN; press ARM at home.').pack()
        self.canvas=tk.Canvas(loop,width=650,height=320,background='white')
        self.canvas.pack()
        ttk.Label(loop,textvariable=self.result,justify='left').pack(pady=10)
        ttk.Label(loop,text='Nominal calibration only. Plot is decimated; firmware integrates at 50 Hz.').pack()
        ttk.Label(evidence,text='Specimen, fixture, calibration and operator notes').pack()
        ttk.Entry(evidence,textvariable=self.note,width=80).pack()
        ttk.Button(evidence,text='Export new private CSV',command=self.export).pack()
        self.log=tk.Text(evidence,width=90,height=15,state='disabled')
        self.log.pack()
        root.protocol('WM_DELETE_WINDOW',self.close)
        self.timer=root.after(30,self.poll)
        self.refresh()

    def reset_capture(self):
        self.rows=[];self.buffer=b'';self.errors=0;self.blocked=False
        self.last_host=None;self.first_host=None;self.device_elapsed=0
        self.refresh()

    def connect(self):
        self.disconnect()
        fd=None;saved=None
        try:
            fd=os.open(self.port.get(),os.O_RDWR|os.O_NOCTTY|os.O_NONBLOCK)
            saved=termios.tcgetattr(fd);cfg=termios.tcgetattr(fd)
            cfg[0]=cfg[1]=cfg[3]=0
            cfg[2]=termios.CS8|termios.CREAD|termios.CLOCAL
            cfg[4]=cfg[5]=termios.B115200
            cfg[6][termios.VMIN]=cfg[6][termios.VTIME]=0
            termios.tcsetattr(fd,termios.TCSANOW,cfg)
            termios.tcflush(fd,termios.TCIFLUSH)
            self.reset_capture()
            self.fd=fd;self.saved=saved;self.provenance='live'
            self.connected_at=time.monotonic()
            self.status.set('connected; waiting for D1 telemetry')
        except (OSError,termios.error) as exc:
            if fd is not None:
                if saved is not None:
                    try:termios.tcsetattr(fd,termios.TCSANOW,saved)
                    except (OSError,termios.error):pass
                os.close(fd)
            self.status.set(f'connection failed; evidence retained: {exc}')

    def disconnect(self):
        if self.fd is not None:
            try:
                if self.saved is not None:
                    termios.tcsetattr(self.fd,termios.TCSANOW,self.saved)
            except (OSError,termios.error):pass
            os.close(self.fd);self.fd=None
        self.status.set('disconnected; retained evidence is not live')

    def accept(self,row):
        if self.blocked:raise ValueError('capture blocked; reconnect')
        dt=0
        if self.rows:
            old=self.rows[-1]
            ds=(row['sequence']-old['sequence'])&U32
            dt=(row['now']-old['now'])&U32
            if ds==0 and row==old:return
            if ds==0 or ds>=0x80000000 or dt==0 or dt>=0x80000000:
                raise ValueError('reset or contradictory sequence/time')
            if row['faults']|old['faults']!=row['faults']:
                raise ValueError('sticky fault disappeared')
            if old['state']>=2:
                if any(row[k]!=old[k] for k in ('base0','base1')):
                    raise ValueError('baseline changed')
                if row['state']<old['state']:
                    raise ValueError('trial state regressed')
            if old['state']>=5 and any(row[k]!=old[k] for k in SUMMARY+('x_um','force_mn')):
                raise ValueError('terminal experiment changed')
            if row['elapsed']<old['elapsed'] or row['elapsed']-old['elapsed']>dt:
                raise ValueError('impossible elapsed time')
            if row['span_um']<old['span_um'] or row['peak_out_mn']<old['peak_out_mn'] or row['peak_back_mn']>old['peak_back_mn']:
                raise ValueError('extrema regressed')
        host=time.monotonic()
        elapsed=self.device_elapsed+dt
        if self.provenance=='live':
            if self.last_host is not None and host-self.last_host>2:
                raise ValueError('host paused; queued data cannot renew freshness')
            if self.first_host is not None and abs((host-self.first_host)*1000-elapsed)>2000:
                raise ValueError('host/device progress diverged')
        if self.first_host is None:self.first_host=host
        self.device_elapsed=elapsed;self.last_host=host
        self.rows.append(row);self.rows=self.rows[-2000:]
        self.status.set(f'{self.provenance}: {len(self.rows)} retained snapshots')
        self.refresh()

    def poll(self):
        if self.fd is not None:
            try:
                poller=select.poll()
                poller.register(self.fd,select.POLLIN|select.POLLHUP|select.POLLERR)
                events=poller.poll(0)
                flags=events[0][1] if events else 0
                if flags&select.POLLIN:
                    self.buffer+=os.read(self.fd,8192)
                    while b'\n' in self.buffer:
                        line,self.buffer=self.buffer.split(b'\n',1)
                        marker=line.rfind(b'D1,')
                        if marker>0:
                            self.errors+=1;line=line[marker:]
                        self.accept(parse(line+b'\n'))
                    if len(self.buffer)>512:raise ValueError('unterminated serial frame')
                if flags&(select.POLLHUP|select.POLLERR):
                    if self.buffer:self.errors+=1
                    self.disconnect()
                elif self.connected_at is not None and time.monotonic()-(self.last_host if self.last_host is not None else self.connected_at)>2:
                    raise ValueError('no fresh telemetry; reconnect explicitly')
            except (OSError,ValueError,termios.error) as exc:
                self.errors+=1;self.blocked=True;self.disconnect()
                self.status.set(f'capture stopped: {exc}')
        self.timer=self.root.after(30,self.poll)

    def refresh(self):
        self.canvas.delete('all')
        self.canvas.create_line(45,280,620,280,fill='black')
        self.canvas.create_line(45,20,45,280,fill='black')
        self.canvas.create_line(45,150,620,150,fill='gray')
        self.canvas.create_text(320,305,text='Travel: 0 to 20 mm; blue OUT, orange BACK')
        self.canvas.create_text(220,12,text='Force: +20 N top / -20 N bottom')
        self.live.set('No measurements');self.result.set('No loop captured')
        for r in self.rows:
            if r['state'] in (3,4,5) and r['acquired']:
                x=45+r['x_um']/20000*575;y=150-r['force_mn']/20000*130
                color='blue' if r['state']==3 else 'darkorange'
                self.canvas.create_oval(x-2,y-2,x+2,y+2,fill=color,outline=color)
        if self.rows:
            r=self.rows[-1]
            measurement=(f"Travel {r['x_um']/1000:.3f} mm; force {r['force_mn']/1000:.3f} N"
                         if r['acquired'] and r['state'] in (2,3,4) else 'No current calibrated acquisition (terminal values retained)')
            self.live.set(f"{STATES[r['state']]} | faults {r['faults']} | ADC valid {r['acquired']}\n{measurement}\nElapsed {r['elapsed']/1000:.2f} s; span {r['span_um']/1000:.3f} mm")
            self.result.set(f"{STATES[r['state']]}: outward peak {r['peak_out_mn']/1000:.3f} N; return minimum {r['peak_back_mn']/1000:.3f} N\nOutward work {r['out_uj']/1000:.3f} mJ; return work {r['back_uj']/1000:.3f} mJ\nSigned loop sum {(r['out_uj']+r['back_uj'])/1000:.3f} mJ — not certified hysteresis energy")
        self.log.configure(state='normal');self.log.delete('1.0','end')
        for r in self.rows[-100:]:
            self.log.insert('end',json.dumps(r)+'\n')
        self.log.configure(state='disabled')

    def replay(self):
        path=filedialog.askopenfilename(title='Select D1 serial recording')
        if not path:return
        try:
            rows=load_replay(path)
            self.disconnect();self.reset_capture();self.provenance='replay'
            for row in rows:self.accept(row)
        except (OSError,ValueError) as exc:
            self.errors+=1;self.blocked=True
            self.status.set(f'replay failed: {exc}')

    def export_to(self,path):
        age=None if self.last_host is None else round(time.monotonic()-self.last_host,3)
        meta=dict(author='jayis1',device='detent-loop',protocol='D1',provenance=self.provenance,
                  connected=self.fd is not None,blocked=self.blocked,errors=self.errors,
                  last_record_age_s=age,status=self.status.get(),notes=self.note.get(),
                  calibration='nominal 2222 ohm/mm, 4000 mN/mm; not physically validated')
        fd=os.open(path,os.O_WRONLY|os.O_CREAT|os.O_EXCL,0o600)
        with os.fdopen(fd,'w',newline='') as stream:
            writer=csv.writer(stream)
            writer.writerow(['metadata',json.dumps(meta)])
            writer.writerow(FIELDS)
            writer.writerows([r[k] for k in FIELDS] for r in self.rows)

    def export(self):
        path=filedialog.asksaveasfilename(defaultextension='.csv')
        if not path:return
        try:self.export_to(path)
        except OSError as exc:messagebox.showerror('Export failed',str(exc))

    def close(self):
        self.disconnect()
        self.root.after_cancel(self.timer)
        self.root.destroy()

if __name__=='__main__':
    root=tk.Tk()
    App(root)
    root.mainloop()
