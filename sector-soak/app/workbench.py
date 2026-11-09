#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Sector Soak desktop: live serial, sector evidence, replay and CSV."""
import termios
import tkinter as tk
from tkinter import ttk, filedialog, messagebox
from model import Session, STATES
from transport import Decoder, Serial, load_replay

class Workbench:
    def __init__(self,root):
        self.root=root
        root.title('Sector Soak — jayis1 — orientation, not water dose')
        root.geometry('1120x740')
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
                             ('Disconnect',self.disconnect),('Replay SS1',self.choose_replay),
                             ('Export CSV',self.choose_export)]:
            ttk.Button(bar,text=label,command=lambda f=action:self.guarded(f)).pack(side='left',padx=3)
        ttk.Label(root,textvariable=self.status).pack(fill='x',padx=8)
        self.tabs=ttk.Notebook(root); self.tabs.pack(fill='both',expand=True,padx=8)
        live=ttk.Frame(self.tabs); self.tabs.add(live,text='Sector dwell')
        ttk.Label(live,textvariable=self.summary,font=('TkDefaultFont',12),justify='left').pack(anchor='nw',padx=10,pady=10)
        cols=('sector','angle','dwell_ms','longest_ms','share')
        self.bins=ttk.Treeview(live,columns=cols,show='headings',height=16)
        for col in cols:
            self.bins.heading(col,text=col); self.bins.column(col,width=160)
        self.bins.pack(fill='both',expand=True)
        history=ttk.Frame(self.tabs); self.tabs.add(history,text='Trace')
        columns='sequence now state raw sector forward backward skipped reversals unknown'.split()
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
                  'J3=GRAY0, J4=GRAY1, J5=GRAY2, J6=GRAY3 (least to most significant)\n'
                  'Each contact: pin 1 signal, pin 2 ground; isolated dry contacts only.\n\n'
                  '16 reflected Gray sectors; five consecutive equal samples qualify a word.\n'
                  'Dwell needs clean endpoints; transition intervals remain unknown.\n'
                  'Skipped sectors clear the direction anchor. Absolute dwell continues.\n'
                  'A sampling gap halts measurement until MCU reset.\n'
                  'No water flow, wetting, delivered volume or watering uniformity measured.\n'
                  'All-open wiring is valid sector zero: disconnects cannot be diagnosed.\n'
                  'Latest 512 snapshots retained; CSV includes session provenance.\n'
                  'Two-second cumulative host/device drift allowance; reconnect if exceeded.\n'
                  'Unbuilt prototype. Keep USB equipment dry; no mains or powered sensor inputs.').pack(anchor='nw',padx=20,pady=20)
        root.protocol('WM_DELETE_WINDOW',self.close)
        self.tick()
    def guarded(self,action):
        try: action()
        except (OSError,ValueError,termios.error) as exc:
            messagebox.showerror('Sector Soak',str(exc),parent=self.root)
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
    def export(self,path): self.session.export(path)
    def choose_replay(self):
        path=filedialog.askopenfilename(filetypes=[('SS1 recording','*.ss1'),('All files','*')])
        if path: self.replay(path)
    def choose_export(self):
        path=filedialog.asksaveasfilename(defaultextension='.csv')
        if path: self.export(path)
    def render(self):
        s=self.session
        self.bins.delete(*self.bins.get_children())
        if s.rows:
            r=s.rows[-1]
            self.summary.set(f"{STATES[r['state']]}  fault={r['fault']} raw={r['raw']:04b} qualified={r['qualified']:04b}\n"
                             f"Sector {r['sector']} (retained); forward {r['forward']} / backward {r['backward']}\n"
                             f"Skipped {r['skipped']} / reversals {r['reversals']} / unknown {r['unknown']} ms")
            total=sum(r[f'dwell{i}'] for i in range(16))
            for i in range(16):
                dwell=r[f'dwell{i}']
                share=f'{100*dwell/total:.1f}%' if total else 'n/a'
                self.bins.insert('','end',values=(i,f'{i*22.5:.1f}–{(i+1)*22.5:.1f}°',dwell,r[f'longest{i}'],share))
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
