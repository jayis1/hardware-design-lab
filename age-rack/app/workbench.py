#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Age Rack Linux/Tk notebook; common UI lifecycle adapted from Stroke Weave."""
import termios
import tkinter as tk
from tkinter import ttk, filedialog, messagebox
from model import Session, STATES
from transport import Decoder, Serial, load_replay

class Workbench:
    def __init__(self, root):
        self.root=root
        root.title('Age Rack — jayis1 — retrieval ranks, not cure quality')
        root.geometry('1120x650')
        self.serial=None
        self.decoder=Decoder()
        self.session=Session('replay')
        self.timer=None
        self.summary=tk.StringVar(value='No evidence loaded')
        self.status=tk.StringVar(value='Disconnected')
        self.path=tk.StringVar(value='/dev/ttyACM0')
        bar=ttk.Frame(root); bar.pack(fill='x',padx=8,pady=8)
        ttk.Entry(bar,textvariable=self.path,width=30).pack(side='left')
        for label,action in [('Connect',lambda:self.connect(self.path.get())),
                             ('Disconnect',self.disconnect),('Replay AR1',self.choose_replay),
                             ('Export CSV',self.choose_export)]:
            ttk.Button(bar,text=label,command=lambda f=action:self.guarded(f)).pack(side='left',padx=3)
        ttk.Label(root,textvariable=self.status).pack(fill='x',padx=8)
        self.tabs=ttk.Notebook(root); self.tabs.pack(fill='both',expand=True,padx=8)
        live=ttk.Frame(self.tabs); self.tabs.add(live,text='Retrieval ranks')
        ttk.Label(live,textvariable=self.summary,font=('TkDefaultFont',14),justify='left').pack(anchor='nw',padx=20,pady=20)
        ttk.Label(live,justify='left',text='Start with all pockets empty. Insert carriers in any order.\n'
                  'Rank zero means no strictly older carrier in the pre-removal inventory.\n'
                  'Tied placement cohorts produce a rank interval, not an invented order.\n'
                  'Simultaneous removals are ranked against the same previous inventory.\n'
                  'HALT is terminal: export, inspect the fixture, then reset.').pack(anchor='nw',padx=20)
        history=ttk.Frame(self.tabs); self.tabs.add(history,text='Trace')
        columns='sequence now state raw occupied epoch insertions removals ambiguous rank0 rank1 rank2 rank3'.split()
        self.trace=ttk.Treeview(history,columns=tuple(columns),show='headings')
        for col in columns:
            self.trace.heading(col,text=col); self.trace.column(col,width=75)
        scroll=ttk.Scrollbar(history,command=self.trace.yview)
        self.trace.configure(yscrollcommand=scroll.set)
        self.trace.pack(side='left',fill='both',expand=True); scroll.pack(side='right',fill='y')
        setup=ttk.Frame(self.tabs); self.tabs.add(setup,text='Wiring and limits')
        ttk.Label(setup,justify='left',text='Author: jayis1\n\n'
                  'NUCLEO-F401RE / ST-LINK VCP / 115200 8N1\n'
                  'J1: 1=3V3 from CN6.4; 2=GND from CN6.6\n'
                  'J2: PA0 / PA1 / PA4 / PB0 from CN8.1..4\n'
                  'J3=RACK_A, J4=RACK_B, J5=RACK_C, J6=RACK_D\n'
                  'Sensor connector: pin 1 signal; pin 2 ground. Dry contacts only.\n\n'
                  'Five consecutive samples qualify each independent contact.\n'
                  'No elapsed residence or specimen identity is measured.\n'
                  'Latest 512 snapshots retained; not a complete event log.\n'
                  'CRC detects corruption, not forgery. No network or command receiver.\n'
                  'Two-second cumulative host/device drift budget; reconnect if exceeded.\n'
                  'Unbuilt prototype; no cure, safety, sterility or worker-scoring claims.').pack(anchor='nw',padx=20,pady=20)
        root.protocol('WM_DELETE_WINDOW',self.close)
        self.tick()
    def guarded(self,action):
        try:
            action()
        except (OSError,ValueError,termios.error) as exc:
            messagebox.showerror('Age Rack',str(exc),parent=self.root)
    def connect(self,path):
        if self.serial is not None:
            raise ValueError('Disconnect before opening another serial connection')
        candidate=Serial(path)
        self.serial=candidate
        self.decoder=Decoder()
        self.session=Session('live'); self.session.connected=True
        self.render()
    def disconnect(self):
        if self.serial is not None:
            if self.decoder.buffer:
                self.decoder.errors+=1; self.decoder.buffer=b''
            self.session.errors=self.decoder.errors
            self.serial.close(); self.serial=None
        self.session.connected=False
        self.render()
    def replay(self,path):
        if self.serial is not None:
            raise ValueError('Disconnect before replacing live evidence')
        self.session=load_replay(path)
        self.render()
    def export(self,path):
        self.session.export(path)
    def choose_replay(self):
        path=filedialog.askopenfilename(filetypes=[('AR1 recording','*.ar1'),('All files','*')])
        if path: self.replay(path)
    def choose_export(self):
        path=filedialog.asksaveasfilename(defaultextension='.csv')
        if path: self.export(path)
    def render(self):
        s=self.session
        if s.rows:
            r=s.rows[-1]
            text=[f"{STATES[r['state']]}  fault={r['fault']}  raw={r['raw']:04b} occupied={r['occupied']:04b}",
                  f"Insertions {r['insertions']} / Removals {r['removals']} / abandoned {r['abandoned']}",
                  f"Unique ranks 0..3: {[r['rank'+str(i)] for i in range(4)]} / Ambiguous {r['ambiguous']}"]
            for i in range(4):
                stamp=r['stamp'+str(i)]
                older=sum(0<r['stamp'+str(j)]<stamp for j in range(4)) if stamp else 0
                peers=sum(j!=i and r['stamp'+str(j)]==stamp for j in range(4)) if stamp else 0
                live=f'cohort {stamp}, current rank {older}..{older+peers}' if stamp else 'empty'
                last=f"{r['low'+str(i)]}..{r['high'+str(i)]}" if r['last_mask']&(1<<i) else 'not in last group'
                text.append(f'{chr(65+i)}: {live}; latest removal: {last}')
            self.summary.set('\n'.join(text))
        else:
            self.summary.set('No evidence loaded')
        self.status.set(f'{s.provenance} | '+('advancing acquisition' if s.fresh() else 'NOT LIVE')+
                        f' | connected={s.connected} | parser errors={s.errors} | {s.warning}')
        self.trace.delete(*self.trace.get_children())
        for r in s.rows:
            self.trace.insert('','end',values=[r[x] for x in self.trace['columns']])
    def tick(self):
        if self.serial is not None:
            try:
                data=self.serial.read()
                if data:
                    rows=self.decoder.feed(data)
                    self.session.errors=self.decoder.errors
                    for row in rows: self.session.accept(row)
            except (OSError,ValueError,termios.error) as exc:
                self.session.warning=str(exc)
                self.disconnect()
        self.render()
        self.timer=self.root.after(40,self.tick)
    def close(self):
        if self.timer is not None:
            self.root.after_cancel(self.timer); self.timer=None
        self.disconnect(); self.root.destroy()

if __name__=='__main__':
    root=tk.Tk(); Workbench(root); root.mainloop()
