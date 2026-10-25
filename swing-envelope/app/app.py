#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Swing Envelope: Linux serial receiver, explicitly labelled replay, local CSV."""
import collections
import csv
import os
from pathlib import Path
import re
import stat
import termios
import time
import tkinter as tk
from tkinter import ttk, filedialog

FIELDS=('sequence','uptime_ms','flags','levels','direction','crossings','period_ms',
        'east_ms','west_ms','left_seen','right_seen','left_missed','right_missed',
        'faults','gaps','timeouts','transport_losses')
COUNTERS=('crossings','left_seen','right_seen','left_missed','right_missed',
          'faults','gaps','timeouts','transport_losses')

def crc(data):
    value=0xffff
    for byte in data:
        value ^= byte << 8
        for _ in range(8):
            value=((value << 1)^0x1021 if value & 0x8000 else value << 1)&0xffff
    return value


def parse(raw):
    if len(raw)>255:
        raise ValueError('oversized record')
    match=re.fullmatch(rb'(SE1,(?:[0-9]+,){16}[0-9]+)\*([0-9A-F]{4})',raw)
    if not match or crc(match[1])!=int(match[2],16):
        raise ValueError('unsupported protocol, malformed record, or CRC error')
    values=[int(x) for x in match[1].split(b',')[1:]]
    if any(v>0xffffffff for v in values):
        raise ValueError('integer outside uint32')
    row=dict(zip(FIELDS,values))
    if row['flags']>15 or row['levels']>15 or row['direction']>2:
        raise ValueError('invalid enum')
    if row['period_ms']>60000 or row['east_ms']>10000 or row['west_ms']>10000:
        raise ValueError('interval outside firmware policy')
    return row


class Model:
    def __init__(self):
        self.start('none')
        self.connected=False

    def start(self,source):
        self.source=source
        self.connected=source=='live'
        self.rows=collections.deque(maxlen=5000)
        self.latest=None
        self.buffer=b''
        self.errors=0
        self.missing=0
        self.blocked=False
        self.received=0.0
        self.warning=''

    def disconnect(self):
        self.connected=False
        self.buffer=b''

    def feed(self,data):
        # Bytewise bounded framing resynchronizes on the full marker, even
        # when a device TX timeout leaves a prefix without a terminating LF.
        for byte in data:
            self.buffer+=bytes([byte])
            if self.buffer.endswith(b'SE1,') and len(self.buffer)>4:
                self.errors+=1
                self.buffer=b'SE1,'
            if byte==10:
                raw=self.buffer[:-1]
                self.buffer=b''
                try:
                    row=parse(raw)
                except ValueError:
                    self.errors+=1
                    continue
                self.accept(row)
            elif len(self.buffer)>255:
                self.errors+=1
                self.buffer=b''

    def accept(self,row):
        if self.blocked:
            return
        if self.latest:
            ds=(row['sequence']-self.latest['sequence'])&0xffffffff
            dt=(row['uptime_ms']-self.latest['uptime_ms'])&0xffffffff
            if ds==0:
                self.errors+=1
                return
            if ds>0x7fffffff or dt>0x7fffffff or any(
                    row[k]<self.latest[k] for k in COUNTERS):
                self.blocked=True
                self.warning='suspected reset; reconnect or reload to start a new session'
                return
            self.missing+=ds-1
        self.received=time.monotonic()
        self.latest=row
        self.rows.append((self.received,dict(row)))

    def status(self,now=None):
        now=time.monotonic() if now is None else now
        if self.blocked:
            return self.warning
        if self.source=='synthetic replay':
            return 'REPLAY: not live measurements'
        if not self.connected:
            return 'disconnected; retained evidence only'
        if not self.latest or now-self.received>2:
            return 'stale: no recent valid telemetry'
        return 'live; timing accuracy not calibrated'

    def export(self,path):
        # Exclusive creation refuses silent overwrite; private permissions
        # precede content writing rather than being repaired afterwards.
        fd=os.open(path,os.O_WRONLY|os.O_CREAT|os.O_EXCL,0o600)
        now=time.monotonic()
        with os.fdopen(fd,'w',newline='') as stream:
            writer=csv.writer(stream)
            writer.writerow(('author','source','connection','status','parser_errors',
                             'missing_frames','age_s',*FIELDS))
            for received,row in self.rows:
                writer.writerow(('jayis1',self.source,'connected' if self.connected else 'disconnected',
                                 self.status(now),self.errors,self.missing,
                                 f'{now-received:.3f}',*(row[k] for k in FIELDS)))


class Serial:
    def __init__(self,path):
        self.fd=None
        self.original=None
        fd=os.open(path,os.O_RDWR|os.O_NONBLOCK|os.O_NOCTTY)
        try:
            if not stat.S_ISCHR(os.fstat(fd).st_mode) or not os.isatty(fd):
                raise ValueError('Select a serial character device, not a regular file')
            self.original=termios.tcgetattr(fd)
            settings=termios.tcgetattr(fd)
            settings[0]=0
            settings[1]=0
            settings[2]=termios.CS8|termios.CREAD|termios.CLOCAL
            settings[3]=0
            settings[4]=termios.B115200
            settings[5]=termios.B115200
            settings[6][termios.VMIN]=1
            settings[6][termios.VTIME]=0
            termios.tcsetattr(fd,termios.TCSANOW,settings)
            termios.tcflush(fd,termios.TCIFLUSH)
            self.fd=fd
        except Exception:
            if self.original is not None:
                try:
                    termios.tcsetattr(fd,termios.TCSANOW,self.original)
                except (OSError,termios.error):
                    pass
            os.close(fd)
            raise

    def read(self):
        if self.fd is None:
            raise OSError('Serial device is closed')
        try:
            data=os.read(self.fd,4096)
        except BlockingIOError:
            return None
        if not data:
            raise OSError('Serial device disconnected')
        return data

    def close(self):
        if self.fd is not None:
            try:
                if self.original is not None:
                    termios.tcsetattr(self.fd,termios.TCSANOW,self.original)
            except (OSError,termios.error):
                pass
            os.close(self.fd)
            self.fd=None


class Window:
    def __init__(self,root):
        self.root=root
        self.model=Model()
        self.serial=None
        root.title('Swing Envelope — jayis1')
        root.geometry('1050x620')
        tabs=ttk.Notebook(root)
        tabs.pack(fill='both',expand=True)
        self.tabs=tabs
        connect=ttk.Frame(tabs,padding=15)
        live=ttk.Frame(tabs,padding=15)
        history=ttk.Frame(tabs,padding=15)
        for frame,name in [(connect,'Connection'),(live,'Live envelope'),(history,'History / export')]:
            tabs.add(frame,text=name)
        self.path=tk.StringVar(value='/dev/ttyACM0')
        ttk.Label(connect,text='115200 baud, receive only. Reconnect clears retained evidence.').pack(anchor='w')
        ttk.Entry(connect,textvariable=self.path,width=65).pack(anchor='w',pady=10)
        ttk.Button(connect,text='Connect',command=self.connect).pack(anchor='w')
        ttk.Button(connect,text='Disconnect',command=self.disconnect).pack(anchor='w')
        ttk.Button(connect,text='Load synthetic / recorded replay',command=self.replay).pack(anchor='w',pady=10)
        ttk.Label(connect,text='Replay is never labelled live. Export before replacing a session.\n'
                  'No commands, cloud accounts, or firmware updates are sent.').pack(anchor='w')
        self.state=tk.StringVar()
        ttk.Label(live,textvariable=self.state,wraplength=950).pack(anchor='w')
        self.summary=tk.StringVar(value='No evidence')
        ttk.Label(live,textvariable=self.summary,font=('TkDefaultFont',14),justify='left').pack(anchor='w',pady=20)
        ttk.Label(live,text='Outer visits are binary threshold evidence, not measured angles.\n'
                  'A missed threshold may mean damping, misalignment, or a failed switch.').pack(anchor='w')
        ttk.Button(history,text='Export retained rows to new CSV',command=self.export).pack(anchor='w')
        cols=('uptime_ms','crossings','period_ms','left_seen','right_seen','left_missed','right_missed','flags')
        self.table=ttk.Treeview(history,columns=cols,show='headings',height=20)
        for name in cols:
            self.table.heading(name,text=name)
            self.table.column(name,width=115)
        self.table.pack(fill='both',expand=True)
        self.error=tk.StringVar()
        ttk.Label(root,textvariable=self.error,wraplength=1000).pack(anchor='w')
        self.signature=None
        root.protocol('WM_DELETE_WINDOW',self.close)
        self.timer=root.after(20,self.poll)

    def disconnect(self):
        if self.serial:
            self.serial.close()
            self.serial=None
        self.model.disconnect()
        self.render()

    def connect(self):
        self.disconnect()
        try:
            self.serial=Serial(self.path.get())
        except (OSError,ValueError,termios.error) as exc:
            self.error.set(str(exc))
            return
        self.model.start('live')
        self.error.set('')
        self.render()

    def load_replay(self,path):
        # Bound allocation before reading an untrusted local file.
        with open(path,'rb') as stream:
            data=stream.read(1024*1024+1)
        if len(data)>1024*1024:
            raise ValueError('Replay exceeds 1 MiB')
        self.disconnect()
        self.model.start('synthetic replay')
        self.model.feed(data)
        self.render()

    def replay(self):
        path=filedialog.askopenfilename()
        if not path:
            return
        try:
            self.load_replay(path)
            self.error.set('')
        except (OSError,ValueError) as exc:
            self.error.set(str(exc))

    def export(self):
        path=filedialog.asksaveasfilename(defaultextension='.csv')
        if not path:
            return
        try:
            self.model.export(path)
            self.error.set('Exported to '+path)
        except OSError as exc:
            self.error.set(str(exc))

    def render(self):
        m=self.model
        self.state.set(f'{m.status()} | parser errors {m.errors} | missing frames {m.missing}')
        row=m.latest
        if row is None:
            self.summary.set('No evidence')
        else:
            period=str(row['period_ms'])+' ms' if row['flags']&2 else 'invalid / historical'
            self.summary.set(f'Crossings: {row["crossings"]}    Period: {period}\n'
                f'Gate transit E / W: {row["east_ms"]} / {row["west_ms"]} ms (last values)\n'
                f'Left reached / missed: {row["left_seen"]} / {row["left_missed"]}\n'
                f'Right reached / missed: {row["right_seen"]} / {row["right_missed"]}\n'
                f'Faults / gaps / timeouts: {row["faults"]} / {row["gaps"]} / {row["timeouts"]}')
        signature=(m.received,len(m.rows))
        if signature!=self.signature:
            self.signature=signature
            self.table.delete(*self.table.get_children())
            for _,record in list(m.rows)[-200:]:
                self.table.insert('', 'end',values=[record[k] for k in self.table['columns']])

    def poll(self):
        try:
            if self.serial:
                data=self.serial.read()
                if data:
                    self.model.feed(data)
        except (OSError,termios.error) as exc:
            self.error.set(str(exc))
            self.disconnect()
        self.render()
        self.timer=self.root.after(20,self.poll)

    def close(self):
        self.root.after_cancel(self.timer)
        self.disconnect()
        self.root.destroy()


if __name__=='__main__':
    root=tk.Tk()
    Window(root)
    root.mainloop()
