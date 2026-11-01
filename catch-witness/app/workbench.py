#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Local Linux desktop: connection, retained outcomes, history and private CSV."""
import csv
import os
import termios
import time
import tkinter as tk
from tkinter import ttk, filedialog, messagebox
from model import FIELDS, STATES, Session
from transport import Decoder, Serial, load_replay

class Workbench(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title('Catch Witness — jayis1'); self.geometry('1080x620')
        self.port=None; self.decoder=Decoder(); self.session=Session('none')
        self.last_rx=None; self.anchor=None; self.progress=0; self.device_elapsed=0
        self.status=tk.StringVar(value='Disconnected; no physical measurements')
        self.path=tk.StringVar(value='/dev/ttyACM0')
        self.summary=tk.StringVar(value='No evidence')
        self.tabs=ttk.Notebook(self); self.tabs.pack(fill='both',expand=True)
        connection=ttk.Frame(self.tabs); live=ttk.Frame(self.tabs); history=ttk.Frame(self.tabs)
        for frame,name in ((connection,'Connection'),(live,'Retention'),(history,'History')):
            self.tabs.add(frame,text=name)
        ttk.Label(connection,text='Voltage-free bench fixture only. Never connect a powered latch.').pack(pady=15)
        ttk.Entry(connection,textvariable=self.path,width=65).pack()
        ttk.Button(connection,text='Connect',command=lambda:self.guard(self.connect,self.path.get())).pack(pady=8)
        ttk.Button(connection,text='Disconnect (retain evidence)',command=self.disconnect).pack()
        ttk.Button(connection,text='Open CW1 recording',command=self.choose_replay).pack(pady=8)
        ttk.Button(connection,text='Export private CSV',command=self.choose_export).pack()
        ttk.Label(live,textvariable=self.summary,justify='left',font=('TkFixedFont',13)).pack(anchor='w',padx=15,pady=20)
        ttk.Label(live,text='Handle/tug associations are not force measurements or causal proof.').pack()
        columns=('now','state','starts','settled','intentional','challenged','unexplained','unseated','timed_out')
        self.history=ttk.Treeview(history,columns=columns,show='headings')
        for name in columns:
            self.history.heading(name,text=name); self.history.column(name,width=110)
        scroll=ttk.Scrollbar(history,orient='vertical',command=self.history.yview)
        self.history.configure(yscrollcommand=scroll.set)
        scroll.pack(side='right',fill='y'); self.history.pack(fill='both',expand=True)
        ttk.Label(self,textvariable=self.status,wraplength=1050).pack(fill='x')
        self.protocol('WM_DELETE_WINDOW',self.close)
        self.after(30,self.loop)
    def guard(self,fn,*args):
        try: fn(*args)
        except (OSError,ValueError,termios.error) as exc:
            messagebox.showerror('Catch Witness',str(exc),parent=self)
    def connect(self,path):
        if self.port: raise ValueError('Disconnect the owned port first')
        port=Serial(path) # Do not erase old evidence if opening fails.
        self.port=port; self.session=Session('serial'); self.decoder=Decoder()
        self.last_rx=time.monotonic(); self.anchor=None; self.progress=0; self.device_elapsed=0
        self.render(); self.status.set('Serial connected; waiting for advancing evidence')
    def disconnect(self):
        if self.port: self.port.close(); self.port=None
        self.status.set(f'Disconnected; retained {self.session.source}; errors={self.decoder.errors}; {self.session.warning}')
    def replay(self,path):
        if self.port: raise ValueError('Disconnect before loading replay')
        session=load_replay(path) # Transactional replacement on fully validated input.
        self.session=session; self.decoder=Decoder(); self.render()
        self.status.set('Replay: synthetic unless independently documented; never live')
    def choose_replay(self):
        path=filedialog.askopenfilename(parent=self)
        if path: self.guard(self.replay,path)
    def choose_export(self):
        path=filedialog.asksaveasfilename(parent=self,defaultextension='.csv')
        if path: self.guard(self.export,path)
    def export(self,path):
        fd=os.open(path,os.O_WRONLY|os.O_CREAT|os.O_EXCL,0o600)
        with os.fdopen(fd,'w',newline='') as handle:
            writer=csv.writer(handle)
            writer.writerow(['author','jayis1','source',self.session.source,'connected',bool(self.port),
                             'blocked',self.session.blocked,'errors',self.decoder.errors,
                             'status',self.status.get(),'warning',self.session.warning,
                             'receive_age_s',None if self.last_rx is None else time.monotonic()-self.last_rx,
                             'acquisition_age','unverified'])
            writer.writerow(FIELDS)
            for row in self.session.rows: writer.writerow([row[k] for k in FIELDS])
    def render(self):
        self.history.delete(*self.history.get_children())
        columns=self.history['columns']
        for row in self.session.rows:
            self.history.insert('', 'end',values=[STATES[row[k]] if k=='state' else row[k] for k in columns])
        if not self.session.rows:
            self.summary.set('No evidence'); return
        row=self.session.rows[-1]
        self.summary.set('\n'.join([f'State: {STATES[row["state"]]}    Contact mask: {row["levels"]:04b}']+
            [f'{name:18s} {row[name]}' for name in ('starts','settled','intentional','challenged','unexplained',
                                                  'unseated','timed_out','aborted','bolt_losses','gaps','faults','age','dwell')]))
    def poll_once(self):
        if not self.port or self.session.blocked: return
        now=time.monotonic()
        if self.last_rx is not None and now-self.last_rx>1.0:
            self.session.blocked=True; self.session.warning='Stale or paused acquisition; reconnect required'
            self.status.set(self.session.warning); return
        try:
            data=self.port.read()
            changed=False
            for row in self.decoder.feed(data):
                old=self.session.rows[-1] if self.session.rows else None
                if self.session.accept(row):
                    changed=True
                    if old:
                        self.device_elapsed+=((row['now']-old['now'])&0xffffffff)/1000
                        self.progress+=1
                    if self.anchor is None: self.anchor=now
                    # Fixed tolerance, not a growing backlog allowance.
                    if abs((now-self.anchor)-self.device_elapsed)>1.5:
                        self.session.fail('Device/host progress mismatch; reconnect required')
                    self.last_rx=now
            if changed: self.render()
            label='Receiving advancing ticks; acquisition age unverified' if self.progress else 'Waiting for device progress'
            if self.session.rows and self.session.rows[-1]['state']==5:
                label='HALTED: acquisition invalid; retained evidence only'
            self.status.set(f'{label}; serial; framing errors={self.decoder.errors}')
        except (OSError,ValueError,termios.error) as exc:
            self.session.blocked=True; self.session.warning=str(exc)
            self.disconnect()
    def loop(self):
        self.poll_once(); self.after(30,self.loop)
    def close(self):
        self.disconnect(); self.destroy()

if __name__=='__main__': Workbench().mainloop()
