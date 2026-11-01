#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Local Shim Bracket workbench: serial, interval evidence, history, CSV."""
import termios
import tkinter as tk
from tkinter import ttk, filedialog
from model import Session, STATES, THICKNESS, bracket
from transport import Decoder, Serial, load_replay

class Workbench:
    def __init__(self,root):
        self.root=root; root.title('Shim Bracket — jayis1'); root.geometry('980x620')
        self.port=None; self.decoder=Decoder(); self.session=Session('replay')
        self.path=tk.StringVar(value='/dev/ttyACM0')
        self.label=tk.StringVar(value='unnamed gap survey')
        self.status=tk.StringVar(value='Disconnected; no evidence')
        self.summary=tk.StringVar(value='No samples')
        self.tabs=ttk.Notebook(root); self.tabs.pack(fill='both',expand=True)
        pages=[ttk.Frame(self.tabs,padding=16) for _ in range(4)]
        for page,title in zip(pages,('Connection / replay','Bracket','History','Policy / export')):
            self.tabs.add(page,text=title)
        connection,live,history,settings=pages
        ttk.Label(connection,text='Linux serial path; 115200 8N1; receive only').pack(anchor='w')
        ttk.Entry(connection,textvariable=self.path,width=70).pack(anchor='w')
        for text,command in [('Connect',self.connect),('Disconnect',self.disconnect),('Open replay',self.replay)]:
            ttk.Button(connection,text=text,command=command).pack(anchor='w',pady=5)
        ttk.Label(root,textvariable=self.status,wraplength=940).pack(fill='x')
        ttk.Label(live,textvariable=self.summary,wraplength=930,font=('TkDefaultFont',12)).pack(anchor='w')
        ttk.Label(live,text='Declarations are not measurements of insertion force or actual clearance.\n'
                  'Conflicting evidence is never overwritten. A stale display is historical.').pack(anchor='w',pady=15)
        self.gauges=ttk.Treeview(live,columns=('thickness','fit','refuse'),show='headings',height=4)
        for c in ('thickness','fit','refuse'): self.gauges.heading(c,text=c)
        self.gauges.pack(fill='x')
        self.history=ttk.Treeview(history,columns=('time','accepted','rejected','aborted','bracket'),show='headings')
        for c in ('time','accepted','rejected','aborted','bracket'):
            self.history.heading(c,text=c); self.history.column(c,width=110 if c!='bracket' else 450)
        self.history.pack(fill='both',expand=True)
        ttk.Label(settings,text='SB1: Gray selector 00,01,11,10 = 0.25,0.50,0.75,1.00 mm nominal.\n'
                  'FIT / REFUSE commits on release, with unchanged selection through the press.\n'
                  'One physical gap per MCU reset. Reconnect alone does NOT clear masks.\n'
                  'Export first, reset the MCU deliberately, then reconnect for a new survey.\n'
                  'No network, remote commands or calibration claims. Author: jayis1.',wraplength=930).pack(anchor='w')
        ttk.Entry(settings,textvariable=self.label,width=70).pack(anchor='w',pady=12)
        ttk.Button(settings,text='Export private CSV; no overwrite',command=self.export).pack(anchor='w')
        ttk.Label(settings,text='Last 2000 snapshots; sparse traffic may omit individual declarations.').pack(anchor='w',pady=12)
        root.protocol('WM_DELETE_WINDOW',self.close); self.timer=root.after(30,self.poll)

    def connect(self):
        if self.port is not None:
            self.status.set('Already connected; disconnect first'); return
        try: port=Serial(self.path.get())
        except (OSError,termios.error,ValueError) as exc:
            self.status.set('Connection failed: '+str(exc)); return
        self.port=port; self.decoder=Decoder(); self.session=Session('live')
        self.render(); self.status.set('Waiting for two advancing snapshots')

    def disconnect(self):
        if self.port is not None:
            self.port.close(); self.port=None
            if self.decoder.buffer:
                self.decoder.errors+=1; self.decoder.buffer=b''
        self.session.connected=False; self.session.errors=self.decoder.errors
        self.status.set('Disconnected; retained evidence is not live'); self.render()

    def poll(self):
        if self.port is not None:
            try:
                rows=self.decoder.feed(self.port.read()); self.session.errors=self.decoder.errors
                for row in rows: self.session.accept(row)
                if rows: self.render()
                self.status.set('LIVE' if self.session.fresh() else 'Waiting / STALE')
            except (OSError,ValueError) as exc:
                self.disconnect(); self.status.set(str(exc))
        self.timer=self.root.after(30,self.poll)

    def render(self):
        self.gauges.delete(*self.gauges.get_children())
        self.history.delete(*self.history.get_children())
        if not self.session.rows: self.summary.set('No samples'); return
        r=self.session.rows[-1]
        self.summary.set(f"{STATES[r['state']]} | {bracket(r)}\n"
                         f"Accepted {r['accepted']} | Rejected {r['rejected']} | Aborted {r['aborted']}\n"
                         f"Contacts {r['levels']} | Gaps {r['gaps']} | Faults {r['faults']}")
        for i,t in enumerate(THICKNESS):
            self.gauges.insert('','end',values=(t+' mm nominal',bool(r['pass']&(1<<i)),bool(r['refuse']&(1<<i))))
        previous=None
        for row in self.session.rows:
            key=tuple(row[k] for k in ('accepted','rejected','aborted'))
            if key!=previous:
                self.history.insert('','end',values=(row['now'],*key,bracket(row)))
            previous=key

    def replay_path(self,path):
        if self.port is not None: raise ValueError('disconnect before replacing evidence')
        session=load_replay(path)
        self.session=session; self.decoder=Decoder(); self.render()
        self.status.set('REPLAY — never live; source may be synthetic')

    def replay(self):
        path=filedialog.askopenfilename(title='Open SB1 recording')
        if path:
            try: self.replay_path(path)
            except (OSError,ValueError) as exc: self.status.set('Replay rejected: '+str(exc))

    def export_to(self,path):
        self.session.export(path,self.label.get()); self.status.set('Exported '+str(path))

    def export(self):
        path=filedialog.asksaveasfilename(title='New CSV',defaultextension='.csv')
        if path:
            try: self.export_to(path)
            except (OSError,ValueError) as exc: self.status.set('Export failed: '+str(exc))

    def close(self):
        self.root.after_cancel(self.timer); self.disconnect(); self.root.destroy()

if __name__=='__main__':
    root=tk.Tk(); Workbench(root); root.mainloop()
