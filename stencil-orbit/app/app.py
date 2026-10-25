#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Stencil Orbit: local Linux desktop, no third-party Python dependencies."""
import csv
import os
import re
import select
import termios
import time
import tkinter as tk
from tkinter import ttk, filedialog, messagebox
from collections import deque
from collections.abc import Sequence

FIELDS: Sequence[str] = ('sequence now flags levels position episode_ms last_duration accepted '
          'rejected repeats skips quad_faults motion_faults registration_faults '
          'gaps transport_losses station0 station1 station2 station3').split()
COUNTERS = FIELDS[7:]

def crc(data):
    value=0xffff
    for byte in data:
        value ^= byte<<8
        for _ in range(8):
            value=((value<<1)^0x1021 if value&0x8000 else value<<1)&0xffff
    return value

def parse(line):
    if not re.fullmatch(rb'SO1(?:,[0-9]{1,10}){20}\*[0-9A-F]{4}\n',line):
        raise ValueError('Unknown protocol or malformed record')
    body, checksum=line[:-1].split(b'*')
    if crc(body)!=int(checksum,16):
        raise ValueError('CRC mismatch')
    numbers=list(map(int,body.split(b',')[1:]))
    if any(n>0xffffffff for n in numbers):
        raise ValueError('Integer overflow')
    record=dict(zip(FIELDS,numbers))
    if record['flags']>31 or record['levels']>15 or record['position']>15:
        raise ValueError('Field outside policy range')
    return record

class Model:
    def __init__(self):
        self.buffer=b''
        self.rows=deque(maxlen=2000)
        self.latest=None
        self.errors=0
        self.missing=0
        self.blocked=False
        self.origin='none'
        self.connected=False
        self.received=None
        self.warning='No evidence received'

    def stale(self, now):
        return self.received is None or now-self.received>2 or not self.connected

    def feed(self, data, now):
        # Process bounded pieces; resynchronize using a full protocol marker.
        # A truncated record followed by SO1 must not swallow the valid record.
        for byte in data:
            self.buffer += bytes([byte])
            if self.buffer.endswith(b'SO1') and len(self.buffer)>3:
                self.errors+=1
                self.buffer=b'SO1'
            if len(self.buffer)>512:
                self.errors+=1
                self.buffer=b''
            if byte!=10:
                continue
            line,self.buffer=self.buffer,b''
            try:
                row=parse(line)
            except ValueError as error:
                self.errors+=1
                self.warning=str(error)
                continue
            if self.blocked:
                continue
            if self.latest:
                seq=(row['sequence']-self.latest['sequence'])&0xffffffff
                elapsed=(row['now']-self.latest['now'])&0xffffffff
                rollback=any(row[key]<self.latest[key] for key in COUNTERS)
                if not seq or seq>=0x80000000 or elapsed>=0x80000000 or rollback:
                    self.blocked=True
                    self.warning='Suspected reset/reordering: export, then start a new session'
                    continue
                self.missing+=seq-1
            self.latest=row
            self.received=now
            self.rows.append((now,row))
            self.warning=('Position unknown: reset at marked home' if not row['flags']&2
                          else 'Observation only: not proof of print quality')

    def export(self, path, now):
        # Exclusive creation avoids accidental destruction of previous evidence.
        fd=os.open(path,os.O_WRONLY|os.O_CREAT|os.O_EXCL,0o600)
        with os.fdopen(fd,'w',newline='') as stream:
            writer=csv.writer(stream)
            writer.writerow(['author','origin','connected','blocked','errors','missing',
                             'age_seconds','warning',*FIELDS])
            for received,row in self.rows:
                writer.writerow(['jayis1',self.origin,self.connected,self.blocked,self.errors,
                                 self.missing,max(0,now-received),self.warning,
                                 *(row[key] for key in FIELDS)])

class Serial:
    def __init__(self, path):
        self.fd=os.open(path,os.O_RDWR|os.O_NOCTTY|os.O_NONBLOCK)
        self.saved=None
        try:
            self.saved=termios.tcgetattr(self.fd)
            attrs=termios.tcgetattr(self.fd)
            attrs[0]=0; attrs[1]=0
            attrs[2]=termios.CS8|termios.CREAD|termios.CLOCAL
            attrs[3]=0
            attrs[4]=termios.B115200; attrs[5]=termios.B115200
            attrs[6][termios.VMIN]=0; attrs[6][termios.VTIME]=0
            termios.tcsetattr(self.fd,termios.TCSANOW,attrs)
            termios.tcflush(self.fd,termios.TCIFLUSH)
        except (OSError,termios.error):
            self.close()
            raise

    def read(self):
        if self.fd is None:
            raise OSError('Serial port is closed')
        poller=select.poll()
        poller.register(self.fd,select.POLLIN|select.POLLHUP|select.POLLERR)
        if any(flags & (select.POLLHUP|select.POLLERR|select.POLLNVAL)
               for _,flags in poller.poll(0)):
            raise OSError('Serial device disconnected')
        try:
            return os.read(self.fd,4096)
        except BlockingIOError:
            return b''

    def close(self):
        if self.fd is None:
            return
        try:
            if self.saved is not None:
                termios.tcsetattr(self.fd,termios.TCSANOW,self.saved)
        except (OSError,termios.error):
            pass
        finally:
            os.close(self.fd)
            self.fd=None

class Application:
    def __init__(self, window):
        self.window=window
        self.model=Model()
        self.serial=None
        self.job=None
        window.title('Stencil Orbit — jayis1')
        window.geometry('1050x650')
        tabs=ttk.Notebook(window); tabs.pack(fill='both',expand=True)
        self.tabs=tabs
        live=ttk.Frame(tabs); history=ttk.Frame(tabs); help_page=ttk.Frame(tabs)
        tabs.add(live,text='Connect / Live')
        tabs.add(history,text='History / Export')
        tabs.add(help_page,text='Setup / Diagnostics')
        self.port=tk.StringVar(value='/dev/ttyACM0')
        ttk.Entry(live,textvariable=self.port,width=45).pack(pady=8)
        for title,command in [('Connect (new session)',self.connect),('Disconnect',self.disconnect),
                              ('Open replay (new session)',self.replay),('Export CSV',self.export)]:
            ttk.Button(live,text=title,command=command).pack(pady=3)
        self.status=tk.StringVar(value='Disconnected')
        ttk.Label(live,textvariable=self.status,wraplength=980).pack(pady=10)
        self.summary=tk.StringVar(value='No records')
        ttk.Label(live,textvariable=self.summary,font=('TkFixedFont',12),justify='left').pack(pady=12)
        self.table=ttk.Treeview(history,columns=('seq','time','station','accepted','rejected','flags'),show='headings')
        for col in self.table['columns']:
            self.table.heading(col,text=col)
            self.table.column(col,width=120)
        self.table.pack(fill='both',expand=True)
        ttk.Button(history,text='Export retained history',command=self.export).pack()
        notes=('Author: jayis1\n\n1. Use an unpowered hand carousel, no motor or heater wiring.\n'
               '2. Mount a 4-cycle/revolution quadrature cam: 16 accepted edges per revolution.\n'
               '3. A/B closures are bits 0/1; registration and tool parked are bits 2/3.\n'
               '4. Align marked station zero, A=B=open, registration closed, tool parked. Reset.\n'
               '5. Confirm known position, rotate one station and check position 4.\n'
               '6. Pickup-to-return episodes need registration throughout; no stroke sensing exists.\n'
               '7. A gap or invalid quadrature loses position until reset at marked home.\n'
               '8. Export before reset/reconnect; opening a session clears retained history.\n'
               '9. Linux: grant serial-device access via your local administrator. No root required.\n'
               '10. USB shared ground: only voltage-free contacts. No isolation or ESD rating.\n\n'
               'Stale after two seconds; history is bounded to 2000 records. Replay is not live data.\n'
               'Counters are boot-local and saturate. Unknown resets cannot always be inferred.\n'
               'No cloud, wireless, commands, automatic printing, or safety interlocks.')
        ttk.Label(help_page,text=notes,justify='left',wraplength=980).pack(padx=15,pady=15,anchor='nw')
        window.protocol('WM_DELETE_WINDOW',self.close)
        self.poll()

    def disconnect(self):
        if self.serial:
            self.serial.close(); self.serial=None
        self.model.connected=False
        self.render()

    def connect(self):
        self.disconnect()
        try:
            serial=Serial(self.port.get())
        except (OSError,termios.error) as error:
            messagebox.showerror('Serial open failed',str(error)); return
        self.model=Model(); self.model.origin='live'; self.model.connected=True
        self.serial=serial
        self.render()

    def load_replay(self,path):
        # Bounded file ingestion avoids unbounded GUI stalls; no port can remain live.
        with open(path,'rb') as stream:
            content=stream.read(1024*1024+1)
        if len(content)>1024*1024:
            raise ValueError('Replay limit is 1 MiB')
        self.disconnect()
        self.model=Model(); self.model.origin='replay'
        self.model.feed(content,time.monotonic())
        self.render()

    def replay(self):
        path=filedialog.askopenfilename(title='Select SO1 telemetry text')
        if path:
            try: self.load_replay(path)
            except (OSError,ValueError) as error: messagebox.showerror('Replay failed',str(error))

    def export(self):
        path=filedialog.asksaveasfilename(defaultextension='.csv',confirmoverwrite=False)
        if path:
            try: self.model.export(path,time.monotonic())
            except OSError as error: messagebox.showerror('Export failed (never overwrites)',str(error))

    def render(self):
        m=self.model
        freshness='STALE / NOT LIVE' if m.stale(time.monotonic()) else 'LIVE'
        self.status.set(f'{m.origin.upper()} | {freshness} | blocked={m.blocked} | '
                        f'errors={m.errors} missing={m.missing}\n{m.warning}')
        row=m.latest
        if row:
            station=str(row['position']//4) if row['flags']&2 and row['position']%4==0 else 'unknown / between'
            self.summary.set(f'Station: {station}   Active: {bool(row["flags"]&4)}\n'
                             f'Accepted uses: {row["accepted"]}   Rejected uses: {row["rejected"]}\n'
                             f'Repeat: {row["repeats"]}   Out-of-order: {row["skips"]}\n'
                             f'Last duration: {row["last_duration"]} ms   Gaps: {row["gaps"]}\n'
                             f'Station counts: {[row[f"station{i}"] for i in range(4)]}')
        else:
            self.summary.set('No records')
        self.table.delete(*self.table.get_children())
        for _,r in m.rows:
            station=r['position']//4 if r['flags']&2 and r['position']%4==0 else '?'
            self.table.insert('', 'end', values=(r['sequence'],r['now'],station,r['accepted'],r['rejected'],r['flags']))

    def poll(self):
        if self.serial:
            try:
                data=self.serial.read()
                if data: self.model.feed(data,time.monotonic())
            except (OSError,termios.error) as error:
                self.disconnect()
                self.model.warning='Serial lost: '+str(error)
        self.render()
        self.job=self.window.after(100,self.poll)

    def close(self):
        if self.job: self.window.after_cancel(self.job)
        self.disconnect()
        self.window.destroy()

if __name__=='__main__':
    window=tk.Tk()
    Application(window)
    window.mainloop()
