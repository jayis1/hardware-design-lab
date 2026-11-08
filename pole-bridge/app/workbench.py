#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Pole Bridge Linux/Tk companion. Lifecycle reuses jayis1 carrier conventions."""
import termios
import tkinter as tk
from tkinter import ttk, filedialog, messagebox
from model import Session, STATES, OUTCOMES
from transport import Decoder, Serial, load_replay

class Workbench:
    def __init__(self,root):
        self.root=root
        root.title('Pole Bridge — jayis1 — disconnected switches only')
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
                             ('Disconnect',self.disconnect),('Replay PB1',self.choose_replay),
                             ('Export CSV',self.choose_export)]:
            ttk.Button(bar,text=label,command=lambda f=action:self.guarded(f)).pack(side='left',padx=3)
        ttk.Label(root,textvariable=self.status).pack(fill='x',padx=8)
        self.tabs=ttk.Notebook(root); self.tabs.pack(fill='both',expand=True,padx=8)
        live=ttk.Frame(self.tabs); self.tabs.add(live,text='Pole topology')
        ttk.Label(live,textvariable=self.summary,font=('TkDefaultFont',14),justify='left').pack(anchor='nw',padx=20,pady=20)
        ttk.Label(live,justify='left',text='Start with both poles on A or both on B. Move manually to the other end.\n'
                  'Each row partitions acquired transfer intervals: open / A / B / overlap / unknown.\n'
                  'Mixed means opposite sources on different poles, NOT a short circuit.\n'
                  'Sub-five-sample contact pulses are not resolved. No electrical ratings inferred.\n'
                  'HALT is terminal: export, inspect, reset and start a new session.').pack(anchor='nw',padx=20)
        history=ttk.Frame(self.tabs); self.tabs.add(history,text='Trace')
        columns='sequence now state raw starts completed returned timeouts last_age last_mixed'.split()
        self.trace=ttk.Treeview(history,columns=tuple(columns),show='headings')
        for col in columns:
            self.trace.heading(col,text=col); self.trace.column(col,width=95)
        scroll=ttk.Scrollbar(history,command=self.trace.yview)
        self.trace.configure(yscrollcommand=scroll.set)
        self.trace.pack(side='left',fill='both',expand=True); scroll.pack(side='right',fill='y')
        setup=ttk.Frame(self.tabs); self.tabs.add(setup,text='Wiring and limits')
        ttk.Label(setup,justify='left',text='Author: jayis1\n\n'
                  'NUCLEO-F401RE / ST-LINK VCP / 115200 8N1\n'
                  'J1: 1=3V3 from CN6.4; 2=GND from CN6.6\n'
                  'J2: PA0 / PA1 / PA4 / PB0 from CN8.1..4\n'
                  'J3=P1_A, J4=P1_B, J5=P2_A, J6=P2_B\n'
                  'Both switch commons join ground. All original wiring must be removed.\n\n'
                  'Five samples qualify each contact; 20 clean intervals settle an endpoint.\n'
                  '5000 acquired intervals is the rejecting deadline. No remote commands.\n'
                  'Latest 512 snapshots retained; not a complete event archive.\n'
                  'CRC detects corruption, not forgery. USB and switch share ground.\n'
                  'Two-second cumulative host/device drift budget; reconnect if exceeded.\n'
                  'Unbuilt prototype; no mains, installed relays, powered coils or live circuits.').pack(anchor='nw',padx=20,pady=20)
        root.protocol('WM_DELETE_WINDOW',self.close)
        self.tick()
    def guarded(self,action):
        try: action()
        except (OSError,ValueError,termios.error) as exc:
            messagebox.showerror('Pole Bridge',str(exc),parent=self.root)
    def connect(self,path):
        if self.serial is not None: raise ValueError('Disconnect before opening another port')
        candidate=Serial(path)
        self.serial=candidate; self.decoder=Decoder()
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
        if self.serial is not None: raise ValueError('Disconnect before replay')
        self.session=load_replay(path)
        self.render()
    def export(self,path):
        self.session.export(path)
    def choose_replay(self):
        path=filedialog.askopenfilename(filetypes=[('PB1 recording','*.pb1'),('All files','*')])
        if path: self.replay(path)
    def choose_export(self):
        path=filedialog.asksaveasfilename(defaultextension='.csv')
        if path: self.export(path)
    def render(self):
        s=self.session
        if s.rows:
            r=s.rows[-1]
            text=[f"{STATES[r['state']]}  fault={r['fault']} raw={r['raw']:04b} qualified={r['qualified']:04b}",
                  f"Starts {r['starts']} / transfers {r['completed']} / returns {r['returned']} / timeouts {r['timeouts']}",
                  f"Last: {OUTCOMES[r['last_outcome']]} from {r['last_origin']:04b}; age {r['last_age']} ms; mixed {r['last_mixed']} ms"]
            for prefix in ('work_','last_'):
                for p in range(2):
                    bins=[r[prefix+'bin'+str(p*4+k)] for k in range(4)]
                    text.append(f"{prefix[:-1]} pole {p+1}: {bins} / unknown {r[prefix+'unknown'+str(p)]} ms")
            self.summary.set('\n'.join(text))
        else: self.summary.set('No evidence loaded')
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
                    rows=self.decoder.feed(data); self.session.errors=self.decoder.errors
                    for row in rows: self.session.accept(row)
            except (OSError,ValueError,termios.error) as exc:
                self.session.warning=str(exc); self.disconnect()
        self.render()
        self.timer=self.root.after(40,self.tick)
    def close(self):
        if self.timer is not None:
            self.root.after_cancel(self.timer); self.timer=None
        self.disconnect(); self.root.destroy()

if __name__=='__main__':
    root=tk.Tk(); Workbench(root); root.mainloop()
