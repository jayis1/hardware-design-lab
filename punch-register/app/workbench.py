#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Local Linux/Tk evidence notebook. No device commands or cloud connection."""
import tkinter as tk
from tkinter import ttk, filedialog
from protocol import Decoder, Session, Serial, STATES, reasons, load_replay

class Workbench:
    def __init__(self, root):
        self.root=root
        root.title('Punch Register — jayis1')
        root.geometry('1050x650')
        self.port=None
        self.session=Session('none')
        self.decoder=Decoder()
        self.path=tk.StringVar(value='/dev/ttyACM0')
        self.note=tk.StringVar()
        self.status=tk.StringVar(value='Disconnected. Prototype: contact evidence, not hole inspection.')
        self.summary=tk.StringVar(value='No evidence')
        self.contacts=tk.StringVar(value='No input sample')
        self.tabs=ttk.Notebook(root); self.tabs.pack(fill='both',expand=True)
        connection=ttk.Frame(self.tabs,padding=16)
        live=ttk.Frame(self.tabs,padding=16)
        history=ttk.Frame(self.tabs,padding=16)
        self.tabs.add(connection,text='Connection / experiment')
        self.tabs.add(live,text='Live registration')
        self.tabs.add(history,text='Snapshot history')
        ttk.Label(connection,text='Receive-only USB serial: 115200 8N1. No motor or safety output.').pack(anchor='w')
        ttk.Entry(connection,textvariable=self.path,width=65).pack(anchor='w',pady=8)
        buttons=ttk.Frame(connection); buttons.pack(anchor='w')
        for text,command in [('Connect',self.connect),('Disconnect',self.disconnect),
                             ('Open recording',self.replay),('Export evidence',self.save)]:
            ttk.Button(buttons,text=text,command=command).pack(side='left',padx=3)
        ttk.Label(connection,text='Fixture / stock / experiment notes (exported locally):').pack(anchor='w',pady=12)
        ttk.Entry(connection,textvariable=self.note,width=100).pack(anchor='w')
        ttk.Label(connection,text=(
            'Fixed acquisition policy: independent 5 ms qualification; 100 ms seated before departure;\n'
            '100 ms empty rearm; 30 s stroke deadline; >5 ms acquisition gap abandons a stroke.\n\n'
            'Begin with paper removed and handle HOME. Seat both stops, then operate the handle.\n'
            'BOTTOM must qualify before return HOME. Remove the stack before the next trial.\n'
            'A second stroke without removal is an extra departure, not another clean trial.\n\n'
            'Calibration is mechanical: adjust switches with an empty fixture and blunt test stock.\n'
            'Do not connect to powered equipment. This recorder cannot authorize safe punching.\n'
            'Replay is never live. Reset inference is heuristic; start a new connection after reset.'
        ),justify='left').pack(anchor='w',pady=16)
        ttk.Label(live,textvariable=self.summary,font=('TkDefaultFont',14),justify='left').pack(anchor='w',pady=12)
        ttk.Label(live,textvariable=self.contacts,justify='left').pack(anchor='w')
        ttk.Label(live,text='“Good” means the observed contact sequence matched policy, not that holes are correct.').pack(anchor='w',pady=20)
        columns=('ms','state','starts','good','bad','aborted','reason')
        self.history=ttk.Treeview(history,columns=columns,show='headings',height=18)
        for column in columns:
            self.history.heading(column,text=column)
            self.history.column(column,width=110 if column!='reason' else 260)
        self.history.pack(fill='both',expand=True)
        ttk.Label(history,text='Last 2,000 status snapshots; intermediate strokes may be absent. Counters are cumulative.').pack(anchor='w')
        ttk.Label(root,textvariable=self.status,wraplength=1000).pack(fill='x',padx=12,pady=10)
        self.timer=root.after(25,self.poll)
        root.protocol('WM_DELETE_WINDOW',self.close)
    def connect(self):
        if self.port:
            self.status.set('Serial port already connected; disconnect first.'); return
        try:
            port=Serial(self.path.get())
        except Exception as exc:
            self.status.set(f'Connect failed: {exc}'); return
        self.port=port; self.decoder=Decoder(); self.session=Session('live')
        self.session.connected=True
        self.status.set('Connected; waiting for advancing device frames.')
        self.render()
    def disconnect(self):
        if self.port:
            self.port.close(); self.port=None
            try: self.decoder.finish()
            except ValueError: pass
            self.session.errors=self.decoder.errors
        self.session.connected=False
        self.status.set('Disconnected; retained evidence is not live.')
        self.render()
    def replay(self, path=None):
        if self.port:
            self.status.set('Disconnect before replacing evidence with replay.'); return
        path=path or filedialog.askopenfilename(title='Open PR1 serial recording')
        if not path: return
        try: replacement=load_replay(path)
        except Exception as exc:
            self.status.set(f'Replay failed: {exc}'); return
        self.session=replacement
        self.status.set('REPLAY — recorded contact evidence, not live hardware.')
        self.render()
    def save(self, path=None):
        path=path or filedialog.asksaveasfilename(defaultextension='.json',title='Export to a NEW file')
        if not path: return
        try:
            self.session.export(path,self.note.get()); self.status.set(f'Exported evidence to {path}')
        except Exception as exc: self.status.set(f'Export failed: {exc}')
    def render(self):
        self.history.delete(*self.history.get_children())
        for row in self.session.rows:
            self.history.insert('', 'end',values=(row['now'],STATES[row['state']],row['starts'],
                row['good'],row['bad'],row['aborted'],reasons(row['last_reason'])))
        if not self.session.rows:
            self.summary.set('No evidence'); self.contacts.set('No input sample'); return
        r=self.session.rows[-1]
        self.summary.set(f"{STATES[r['state']]}\nStarts {r['starts']}    Good {r['good']}    Bad {r['bad']}    Aborted {r['aborted']}\n"
                         f"Extra strokes {r['extra']}    Gaps {r['gaps']}    Transport errors {r['tx_errors']}\n"
                         f"Last duration {r['last_duration']} ms; last reasons: {reasons(r['last_reason'])}")
        inputs='\n'.join(f"{name}: {'CLOSED' if r['mask']&(1<<i) else 'open'}"
                         f" {'qualified' if r['qualified']&(1<<i) else 'UNKNOWN'}"
                         for i,name in enumerate(('Left stop','Right stop','Bottom','Home')))
        self.contacts.set(inputs+f"\nCurrent reasons: {reasons(r['reason'])}\nSeated qualification: {r['seat_ms']} / 100 ms")
    def poll(self):
        if self.port:
            try:
                data=self.port.read()
                rows=self.decoder.feed(data)
                self.session.errors=self.decoder.errors
                for row in rows: self.session.accept(row)
                if rows: self.render()
                if self.session.fresh(): self.status.set(f'LIVE contact evidence; parser errors {self.session.errors}')
                else: self.status.set('WAITING / STALE — no confirmed fresh advancing stream.')
            except Exception as exc:
                self.disconnect()
                self.session.warning=str(exc)
                self.status.set(f'disconnected: {exc}. Retained evidence only.')
        self.timer=self.root.after(25,self.poll)
    def close(self):
        self.root.after_cancel(self.timer)
        self.disconnect(); self.root.destroy()

if __name__=='__main__':
    window=tk.Tk(); Workbench(window); window.mainloop()
