#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Balance Order Linux/Tk workbench. No network or target commands."""
import termios
import tkinter as tk
from tkinter import ttk, filedialog
from model import Session, STATES
from transport import Decoder, Serial, load_replay

class Workbench:
    def __init__(self,root):
        self.root=root; root.title('Balance Order — jayis1'); root.geometry('1000x650')
        self.port=None; self.decoder=Decoder(); self.session=Session('replay')
        self.path=tk.StringVar(value='/dev/ttyACM0')
        self.label=tk.StringVar(value='coupons A B C; fixed symmetric deadband; uncalibrated')
        self.status=tk.StringVar(value='Disconnected; no evidence')
        self.summary=tk.StringVar(value='No samples')
        self.tabs=ttk.Notebook(root); self.tabs.pack(fill='both',expand=True)
        pages=[ttk.Frame(self.tabs,padding=12) for _ in range(4)]
        for page,title in zip(pages,('Connection / replay','Comparison ledger','History','Setup / export')):
            self.tabs.add(page,text=title)
        connection,live,history,settings=pages
        ttk.Label(connection,text='Linux serial path; 115200 8N1; receive only').pack(anchor='w')
        ttk.Entry(connection,textvariable=self.path,width=70).pack(anchor='w')
        for text,command in [('Connect',self.connect),('Disconnect',self.disconnect),('Open replay',self.replay)]:
            ttk.Button(connection,text=text,command=command).pack(anchor='w',pady=5)
        ttk.Label(connection,text='Load the labelled pair while the key is removed (PARK=0).\n'
                  'Insert pair key AB=1, AC=2 or BC=3 only after loading.\n'
                  'Left-low=4; right-low=8. Check each sensor before using a neutral result.\n'
                  'The fixture does not identify objects or prove that either pan is loaded.').pack(anchor='w',pady=15)
        ttk.Label(root,textvariable=self.status,wraplength=960).pack(fill='x')
        ttk.Label(live,textvariable=self.summary,wraplength=960).pack(anchor='w',pady=10)
        self.table=ttk.Treeview(live,columns=('pair','neutral','left','right'),show='headings',height=4)
        for key,title in [('pair','Pair (left/right)'),('neutral','Neutral band'),('left','Left low'),('right','Right low')]:
            self.table.heading(key,text=title); self.table.column(key,width=200)
        self.table.pack(fill='x')
        ttk.Label(live,text='Neutral is a finite deadband, NOT an equality relation.\n'
                  'A neutral B and B neutral C can coexist with A heavier C.\n'
                  'INCOMPATIBLE means no assignment fits the shared symmetric-deadband model.\n'
                  'Inspect calibration, pan placement, labels and friction; do not average away contrary evidence.',
                  wraplength=960).pack(anchor='w',pady=20)
        self.history=ttk.Treeview(history,columns=('time','accepted','rejected','aborted','conflict'),show='headings')
        for key in ('time','accepted','rejected','aborted','conflict'):
            self.history.heading(key,text=key); self.history.column(key,width=160)
        self.history.pack(fill='both',expand=True)
        ttk.Label(settings,text='BO1 policy — Author: jayis1\n\n'
                  'One stable 500 ms beam classification per pair-key insertion.\n'
                  'Identity changes, ten-second timeout or stable both-low contacts reject the trial.\n'
                  'Remove the key to arm another observation. Acquisition loss permanently halts.\n'
                  'Do not change the sensor spacing, pans or sample labels within a session.\n'
                  'Neutral observations require a calibrated, common, symmetric deadband assumption.\n'
                  'No grams, certified ranking, load detection, authentication or physical safety claim.\n'
                  'Export before reset; reconnect alone does not reset MCU evidence.\n'
                  'History retains at most 2000 snapshots; sparse telemetry is not a complete event log.',
                  wraplength=960).pack(anchor='w')
        ttk.Entry(settings,textvariable=self.label,width=100).pack(anchor='w',pady=15)
        ttk.Button(settings,text='Export private CSV; no overwrite',command=self.export).pack(anchor='w')
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
        self.status.set('Disconnected; retained evidence is historical'); self.render()

    def poll(self):
        if self.port is not None:
            try:
                rows=self.decoder.feed(self.port.read()); self.session.errors=self.decoder.errors
                for row in rows: self.session.accept(row)
                if rows: self.render()
                state='HALTED device' if self.session.rows and self.session.rows[-1]['state']==4 else 'LIVE transport'
                self.status.set(state if self.session.fresh() else 'Waiting / STALE')
            except (OSError,ValueError) as exc:
                self.disconnect(); self.status.set(str(exc))
        self.timer=self.root.after(30,self.poll)

    def render(self):
        self.table.delete(*self.table.get_children()); self.history.delete(*self.history.get_children())
        if not self.session.rows: self.summary.set('No samples'); return
        r=self.session.rows[-1]
        verdict='INCOMPATIBLE — inspect fixture and repeated comparisons' if r['conflict'] else 'Compatible so far — NOT a calibrated mass ranking'
        self.summary.set(f"{STATES[r['state']]} | {verdict}\n"
                         f"Accepted {r['accepted']} | Rejected {r['rejected']} | Aborted {r['aborted']}\n"
                         f"Pair {r['selected']} | age {r['age']} ms | stable {r['dwell']} / 500 ms | "
                         f"contacts {r['levels']} | faults {r['faults']}")
        for p,name in enumerate(('A / B','A / C','B / C')):
            self.table.insert('','end',values=(name,*(r['c'+str(3*p+i)] for i in range(3))))
        previous=None
        for row in self.session.rows:
            key=tuple(row[k] for k in ('accepted','rejected','aborted','conflict'))
            if key!=previous: self.history.insert('','end',values=(row['now'],*key))
            previous=key

    def replay_path(self,path):
        if self.port is not None: raise ValueError('disconnect before replacing evidence')
        session=load_replay(path)
        self.session=session; self.decoder=Decoder(); self.render()
        self.status.set('REPLAY — never live; source may be synthetic')

    def replay(self):
        path=filedialog.askopenfilename(title='Open BO1 recording')
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
