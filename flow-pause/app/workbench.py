#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Flow Pause: Linux serial/replay workbench, no network dependencies."""
import copy
import csv
import os
import select
import stat
import termios
import time
import tkinter as tk
from tkinter import ttk, filedialog, messagebox
from protocol import Parser, Session, FIELDS

class Serial:
    def __init__(self,path):
        self.fd=os.open(path,os.O_RDWR|os.O_NOCTTY|os.O_NONBLOCK)
        self.old=None
        try:
            self.old=termios.tcgetattr(self.fd)
            settings=copy.deepcopy(self.old)
            settings[0]=settings[1]=settings[3]=0
            settings[2]=termios.CS8|termios.CREAD|termios.CLOCAL
            settings[4]=settings[5]=termios.B115200
            settings[6][termios.VMIN]=settings[6][termios.VTIME]=0
            termios.tcsetattr(self.fd,termios.TCSANOW,settings)
            termios.tcflush(self.fd,termios.TCIFLUSH)
            self.poller=select.poll()
            self.poller.register(self.fd,select.POLLIN|select.POLLHUP|select.POLLERR)
        except BaseException:
            self.close()
            raise
    def read(self):
        if self.fd is None:
            return b'',True
        data=bytearray(); lost=False
        for _,event in self.poller.poll(0):
            if event & select.POLLIN:
                try: data.extend(os.read(self.fd,8192))
                except BlockingIOError: pass
            lost=bool(event & (select.POLLHUP|select.POLLERR|select.POLLNVAL))
        return bytes(data),lost
    def close(self):
        if self.fd is None: return
        if self.old is not None:
            try: termios.tcsetattr(self.fd,termios.TCSANOW,self.old)
            except (OSError,termios.error): pass
        os.close(self.fd); self.fd=None

def replay(path):
    fd=os.open(path,os.O_RDONLY|os.O_NONBLOCK)
    try:
        info=os.fstat(fd)
        if not stat.S_ISREG(info.st_mode) or info.st_size>1048576:
            raise ValueError('Replay must be a regular file no larger than 1 MiB')
        with os.fdopen(fd,'rb',closefd=False) as stream: data=stream.read(1048577)
        if len(data)>1048576: raise ValueError('Replay grew past limit')
    finally: os.close(fd)
    parser=Parser(); session=Session('replay')
    for row in parser.feed(data): session.accept(row,time.monotonic())
    parser.finish()
    if parser.errors or parser.discarded or session.blocked or not session.rows:
        raise ValueError('Replay empty, malformed, truncated, or inconsistent')
    return session,parser

def export(path,session,parser,connected):
    fd=os.open(path,os.O_CREAT|os.O_EXCL|os.O_WRONLY,0o600)
    with os.fdopen(fd,'w',newline='') as stream:
        writer=csv.writer(stream); now=time.monotonic()
        age='' if session.last_host is None else now-session.last_host
        writer.writerow(['author','jayis1','source',session.source,'connected',connected,
                         'fresh',session.fresh(now,connected),'age_s',age,'blocked',session.blocked,
                         'parser_errors',parser.errors,'discarded_bytes',parser.discarded,
                         'evicted',session.evicted])
        writer.writerow(FIELDS); writer.writerows(session.rows)

class App:
    def __init__(self,root):
        self.root=root; root.title('Flow Pause — jayis1'); root.geometry('1000x700')
        self.serial=None; self.session=Session('none'); self.parser=Parser()
        self.port=tk.StringVar(value='/dev/ttyACM0'); self.status=tk.StringVar()
        bar=ttk.Frame(root); bar.pack(fill='x')
        ttk.Label(bar,text='Linux serial port').pack(side='left')
        ttk.Entry(bar,textvariable=self.port,width=30).pack(side='left')
        for title,callback in [('Connect',self.connect),('Disconnect',self.disconnect),
                               ('Replay',self.load),('Export CSV',self.save)]:
            ttk.Button(bar,text=title,command=callback).pack(side='left')
        ttk.Label(root,textvariable=self.status).pack(fill='x')
        self.tabs=ttk.Notebook(root); self.tabs.pack(fill='both',expand=True)
        live=ttk.Frame(self.tabs); history=ttk.Frame(self.tabs); setup=ttk.Frame(self.tabs)
        self.tabs.add(live,text='Pause evidence'); self.tabs.add(history,text='Snapshot history')
        self.tabs.add(setup,text='Setup / limits')
        self.table=ttk.Treeview(live,columns=('field','value'),show='headings',height=25)
        for name in ('field','value'): self.table.heading(name,text=name)
        self.table.pack(fill='both',expand=True)
        ttk.Label(live,text='Assisted means a declared tap preceded recovery. It does NOT mean the tap caused flow.').pack()
        self.history=ttk.Treeview(history,columns=('time','runs','pulses','pauses','recovered','assisted','aborted'),show='headings')
        for name in self.history['columns']:
            self.history.heading(name,text=name); self.history.column(name,width=120)
        self.history.pack(fill='both',expand=True)
        ttk.Label(setup,justify='left',wraplength=930,text=(
            'Author: jayis1\n\nJ3 discharge reed; J4 declared supply; J5 intervention button; J6 enable.\n'
            'Use voltage-free contacts only. USB ground is common. No powered industrial signals.\n'
            'Disable first, declare supply, ensure discharge and intervention are low, then enable.\n'
            '8 ms independent qualification, 5 s silence criterion, 2 s maximum discharge pulse.\n'
            'A complete discharge pulse is a qualified closure followed by opening, not a volume.\n'
            'A pause may be emptying, sensor failure, slow collection or blockage; inspect visually.\n'
            'Acquisition gaps or supply withdrawal abandon the run. Disable/re-enable to start again.\n'
            'Replay is offline evidence. CSV retains 500 snapshots and explicit provenance.\n'
            'Export before reconnect; successful reconnect replaces the in-memory session.\n'
            'No physical hardware, grain handling, sensor calibration or safety validation has occurred.\n'
            'Supervised tabletop nonhazardous beads only. No motor, mains, silo or dust cloud.')).pack(padx=15,pady=20,anchor='nw')
        root.protocol('WM_DELETE_WINDOW',self.close)
        self.render(); self.job=root.after(25,self.poll)
    def connect(self):
        if self.serial is not None: return
        try: serial=Serial(self.port.get())
        except (OSError,termios.error) as error:
            messagebox.showerror('Connection failed',str(error)); return
        self.serial=serial; self.session=Session('live'); self.parser=Parser(); self.render()
    def disconnect(self):
        if self.serial: self.serial.close()
        self.serial=None; self.render()
    def load(self):
        if self.serial:
            messagebox.showerror('Replay','Disconnect before replacing live evidence'); return
        path=filedialog.askopenfilename()
        if not path: return
        try: session,parser=replay(path)
        except (OSError,ValueError) as error:
            messagebox.showerror('Replay rejected',str(error)); return
        self.session,self.parser=session,parser; self.render()
    def save(self):
        path=filedialog.asksaveasfilename(defaultextension='.csv',confirmoverwrite=False)
        if not path: return
        try: export(path,self.session,self.parser,self.serial is not None)
        except OSError as error: messagebox.showerror('Export failed',str(error))
    def poll(self):
        if self.serial:
            try:
                data,lost=self.serial.read()
                for row in self.parser.feed(data): self.session.accept(row,time.monotonic())
                if lost: self.disconnect()
            except (OSError,termios.error): self.disconnect()
        self.render(); self.job=self.root.after(25,self.poll)
    def render(self):
        connected=self.serial is not None
        label='LIVE' if self.session.fresh(time.monotonic(),connected) else ('WAIT/STALE' if connected else 'DISCONNECTED')
        self.status.set(f'{label} | source={self.session.source} | errors={self.parser.errors} | '
                        f'discarded={self.parser.discarded} | evicted={self.session.evicted} | {self.session.blocked}')
        for table in (self.table,self.history):
            children=table.get_children()
            if children: table.delete(*children)
        if self.session.rows:
            f=dict(zip(FIELDS,self.session.rows[-1]))
            f['state']=('WAIT','READY','WATCH','PAUSED','HALT')[f['state']]
            for name in FIELDS: self.table.insert('','end',values=(name,f[name]))
            for row in self.session.rows:
                f=dict(zip(FIELDS,row))
                self.history.insert('','end',values=[f[k] for k in self.history['columns']])
    def close(self):
        if self.serial: self.serial.close(); self.serial=None
        self.root.after_cancel(self.job); self.root.destroy()

if __name__=='__main__':
    root=tk.Tk(); App(root); root.mainloop()
