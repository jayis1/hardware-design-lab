#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Slip Window local Linux USB-serial notebook; standard library only."""
import binascii
import csv
import os
from pathlib import Path
import select
import termios
import time
import tkinter as tk
from tkinter import ttk, filedialog, messagebox
import tty

FIELDS=('seq uptime state reason raw stable qualified cycle_ms residence_ms '
        'drain_ms longest_drain_ms rest_ms interruptions completed rejected gaps tx_loss').split()
STATES=('clear','ready','cast','turn','drain','return','rest','done','reject')
REASONS=('none','open early','contradictory stops','sampling gap','mark repeated',
         'turn timeout','cycle timeout','counter limit','bad input','no drain')

class Model:
    def __init__(self, source):
        self.source=source
        self.buffer=bytearray()
        self.rows=[]
        self.latest=None
        self.errors=0
        self.missing=0
        self.blocked=False
        self.received=None

    def stale(self, now):
        return self.received is None or now-self.received>1.5

    def feed(self, data, now):
        # Byte-wise bounded framing also recovers a valid marker following a
        # partial frame whose terminating newline was lost on UART timeout.
        for byte in data:
            if byte==ord('$'):
                if self.buffer:
                    self.errors+=1
                self.buffer=bytearray(b'$')
            elif self.buffer:
                self.buffer.append(byte)
                if byte==10:
                    self._record(bytes(self.buffer),now)
                    self.buffer.clear()
                elif len(self.buffer)>=256:
                    self.errors+=1
                    self.buffer.clear()

    def _record(self, line, now):
        if self.blocked:
            return
        try:
            payload,crc=line.rstrip(b'\n').rsplit(b'*',1)
            if len(crc)!=4 or binascii.crc_hqx(payload,0xffff)!=int(crc,16):
                raise ValueError('CRC')
            parts=payload.split(b',')
            if parts[0]!=b'$SW1' or len(parts)!=len(FIELDS)+1:
                raise ValueError('Version or field count')
            if any(not part or not part.isdigit() or len(part)>10 for part in parts[1:]):
                raise ValueError('Integer syntax')
            values=[int(part) for part in parts[1:]]
            if any(v>0xffffffff for v in values):
                raise ValueError('Integer range')
            row=dict(zip(FIELDS,values))
            if row['state']>=len(STATES) or row['reason']>=len(REASONS):
                raise ValueError('Enum range')
            if row['raw']>15 or row['stable']>15 or row['qualified']>1:
                raise ValueError('Contact range')
            if row['longest_drain_ms']>row['drain_ms']:
                raise ValueError('Drain invariant')
            if self.latest is not None:
                ds=(row['seq']-self.latest['seq'])&0xffffffff
                dt=(row['uptime']-self.latest['uptime'])&0xffffffff
                rollback=any(row[k]<self.latest[k] for k in ('completed','rejected','gaps','tx_loss'))
                if ds==0 or ds>=0x80000000 or dt==0 or dt>=0x80000000 or rollback:
                    self.blocked=True
                    self.errors+=1
                    return
                self.missing+=ds-1
            self.latest=row
            self.received=now
            self.rows.append((dict(row),now))
            del self.rows[:-1000]
        except (ValueError,UnicodeError):
            self.errors+=1

    def export(self, path, now, connection):
        # Exclusive create and restrictive permissions: never overwrite a
        # previous batch log or leak workshop job metadata by default.
        fd=os.open(path,os.O_WRONLY|os.O_CREAT|os.O_EXCL,0o600)
        with os.fdopen(fd,'w',newline='') as file:
            writer=csv.writer(file)
            writer.writerow(['author','jayis1','source',self.source,'connection',connection,
                             'errors',self.errors,'missing',self.missing,'blocked',self.blocked])
            writer.writerow(FIELDS+['age_seconds','stale_at_export'])
            for row,received in self.rows:
                age=max(0,now-received)
                writer.writerow([row[k] for k in FIELDS]+[f'{age:.3f}',age>1.5])

class Serial:
    def __init__(self,path):
        self.fd=os.open(path,os.O_RDWR|os.O_NOCTTY|os.O_NONBLOCK)
        self.saved=None
        try:
            self.saved=termios.tcgetattr(self.fd)
            tty.setraw(self.fd)
            attrs=termios.tcgetattr(self.fd)
            attrs[4]=termios.B115200
            attrs[5]=termios.B115200
            attrs[2]|=termios.CLOCAL|termios.CREAD
            attrs[2]&=~termios.CRTSCTS
            termios.tcsetattr(self.fd,termios.TCSANOW,attrs)
            termios.tcflush(self.fd,termios.TCIFLUSH)
        except Exception:
            self.close()
            raise

    def read(self):
        if self.fd is None:
            raise OSError('Serial device closed')
        if not select.select([self.fd],[],[],0)[0]:
            return b''
        data=os.read(self.fd,4096)
        if not data:
            raise OSError('Serial device disconnected')
        return data

    def close(self):
        if self.fd is not None:
            try:
                if self.saved is not None:
                    termios.tcsetattr(self.fd,termios.TCSANOW,self.saved)
            except (OSError,termios.error):
                pass
            finally:
                os.close(self.fd)
                self.fd=None

class App:
    def __init__(self,root):
        self.root=root
        self.serial=None
        self.model=Model('none')
        self.connection='disconnected'
        root.title('Slip Window — jayis1')
        root.geometry('1000x620')
        self.tabs=ttk.Notebook(root)
        self.tabs.pack(fill='both',expand=True)
        connect=ttk.Frame(self.tabs,padding=15)
        live=ttk.Frame(self.tabs,padding=15)
        history=ttk.Frame(self.tabs,padding=15)
        for tab,name in ((connect,'Connection'),(live,'Casting notebook'),(history,'History')):
            self.tabs.add(tab,text=name)
        self.path=tk.StringVar(value='/dev/ttyACM0')
        ttk.Label(connect,text='Linux serial device (115200 8N1, receive only)').pack(anchor='w')
        ttk.Entry(connect,textvariable=self.path,width=65).pack(anchor='w')
        ttk.Button(connect,text='Connect new session',command=self.connect).pack(anchor='w',pady=5)
        ttk.Button(connect,text='Disconnect, retain evidence',command=self.disconnect).pack(anchor='w')
        ttk.Button(connect,text='Open synthetic/offline replay',command=self.replay).pack(anchor='w',pady=5)
        ttk.Button(connect,text='Export retained rows as CSV',command=self.export).pack(anchor='w')
        ttk.Label(connect,text='Export before reconnecting: a new session clears retained rows.\n'
                  'Upright + latched + marker released for 1 s arms the recorder.\n'
                  'Press fill marker only after filling. Tip to drain stop; return upright; unlatch.\n'
                  'Times describe contact states, NOT wall thickness, dryness or safe release.\n'
                  'Replay is not a physical measurement. No commands are sent to the MCU.').pack(anchor='w',pady=20)
        self.status=tk.StringVar()
        self.summary=tk.StringVar()
        ttk.Label(live,textvariable=self.status,wraplength=950).pack(anchor='w')
        ttk.Label(live,textvariable=self.summary,font=('monospace',15),justify='left').pack(anchor='w',pady=20)
        self.tree=ttk.Treeview(history,columns=('seq','state','cast','drain','longest','rest'),show='headings')
        for name in self.tree['columns']:
            self.tree.heading(name,text=name)
            self.tree.column(name,width=145)
        self.tree.pack(fill='both',expand=True)
        root.protocol('WM_DELETE_WINDOW',self.close)
        self.render()
        self.timer=root.after(50,self.poll)

    def connect(self):
        self.disconnect()
        try:
            serial=Serial(self.path.get())
        except (OSError,termios.error) as error:
            messagebox.showerror('Connection failed',str(error))
            return
        self.serial=serial
        self.model=Model('live serial: '+self.path.get())
        self.connection='connected'
        self.render()

    def disconnect(self):
        if self.serial:
            self.serial.close()
            self.serial=None
        self.connection='disconnected'
        self.render()

    def load_replay(self,path):
        # Check bounded input before replacing retained evidence.
        with open(path,'rb') as file:
            data=file.read(1024*1024+1)
        if len(data)>1024*1024:
            raise ValueError('Replay exceeds 1 MiB')
        self.disconnect()
        self.model=Model('offline replay: '+str(path))
        self.connection='replay (not live)'
        self.model.feed(data,time.monotonic())
        self.render()

    def replay(self):
        path=filedialog.askopenfilename()
        if path:
            try: self.load_replay(path)
            except (OSError,ValueError) as error: messagebox.showerror('Replay failed',str(error))

    def export(self):
        path=filedialog.asksaveasfilename(defaultextension='.csv')
        if path:
            try: self.model.export(path,time.monotonic(),self.connection)
            except OSError as error: messagebox.showerror('Export failed',str(error))

    def render(self):
        m=self.model
        flag='BLOCKED: rollback/duplicate; reconnect required' if m.blocked else ('STALE' if m.stale(time.monotonic()) else 'recent receipt')
        self.status.set(f'{self.connection} | {m.source} | {flag}\nErrors {m.errors}, missing frames {m.missing}; at most 1000 rows retained')
        row=m.latest
        if row is None:
            self.summary.set('No valid SW1 record received')
        else:
            self.summary.set(f'State: {STATES[row["state"]]} / {REASONS[row["reason"]]}\n'
                f'Raw / qualified contacts: {row["raw"]:04b} / {row["stable"]:04b}\n'
                f'Casting residence: {row["residence_ms"]/1000:.3f} s\n'
                f'Total draining: {row["drain_ms"]/1000:.3f} s\n'
                f'Longest drain segment: {row["longest_drain_ms"]/1000:.3f} s\n'
                f'Upright rest: {row["rest_ms"]/1000:.3f} s\n'
                f'Drain interruptions: {row["interruptions"]}\n'
                f'Complete {row["completed"]}; rejected {row["rejected"]}; gaps {row["gaps"]}; TX loss {row["tx_loss"]}')
        # Incremental insert is unnecessary at 4 Hz; render all bounded rows,
        # including multiple records received in one USB read or replay batch.
        children=self.tree.get_children()
        if children: self.tree.delete(*children)
        for r,_ in m.rows:
            self.tree.insert('', 'end',values=(r['seq'],STATES[r['state']],r['residence_ms'],r['drain_ms'],r['longest_drain_ms'],r['rest_ms']))

    def poll(self):
        if self.serial:
            try:
                data=self.serial.read()
                self.model.feed(data,time.monotonic())
            except OSError as error:
                self.disconnect()
                self.connection='disconnected: '+str(error)
        self.render()
        self.timer=self.root.after(50,self.poll)

    def close(self):
        self.root.after_cancel(self.timer)
        self.disconnect()
        self.root.destroy()

if __name__=='__main__':
    root=tk.Tk()
    App(root)
    root.mainloop()
