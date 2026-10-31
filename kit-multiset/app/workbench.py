#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Kit Multiset local connection, evidence and export workbench."""
import termios
import tkinter as tk
from tkinter import ttk, filedialog
from model import Session, STATES
from transport import Decoder, Serial, load_replay

class Workbench:
    def __init__(self, root):
        self.root=root
        root.title('Kit Multiset — jayis1')
        root.geometry('900x560')
        self.port=None
        self.decoder=Decoder()
        self.session=Session('replay')
        self.path=tk.StringVar(value='/dev/ttyACM0')
        self.label=tk.StringVar(value='unnamed fixture')
        self.status=tk.StringVar(value='Disconnected; no evidence loaded')
        self.summary=tk.StringVar(value='No samples')
        self.tabs=ttk.Notebook(root); self.tabs.pack(fill='both',expand=True)
        connection=ttk.Frame(self.tabs,padding=16)
        live=ttk.Frame(self.tabs,padding=16)
        settings=ttk.Frame(self.tabs,padding=16)
        for page,title in [(connection,'Connection / replay'),(live,'Live evidence'),(settings,'Recipe / export')]:
            self.tabs.add(page,text=title)
        ttk.Label(connection,text='Linux serial path (115200 baud; receive only)').pack(anchor='w')
        ttk.Entry(connection,textvariable=self.path,width=70).pack(anchor='w')
        for text,command in [('Connect',self.connect),('Disconnect',self.disconnect),('Open strict replay',self.replay)]:
            ttk.Button(connection,text=text,command=command).pack(anchor='w',pady=5)
        ttk.Label(connection,textvariable=self.status,wraplength=800).pack(anchor='w',pady=15)
        ttk.Label(live,textvariable=self.summary,wraplength=800,font=('TkDefaultFont',12)).pack(anchor='w')
        self.history=ttk.Treeview(live,columns=('time','result','counts','duration'),show='headings',height=14)
        for key,title in [('time','Device ms'),('result','Latest disposition'),('counts','Retained A / B / C'),('duration','Retained ms')]:
            self.history.heading(key,text=title); self.history.column(key,width=180)
        self.history.pack(fill='both',expand=True,pady=12)
        ttk.Label(settings,text='Fixed firmware recipe: A = 2 strokes, B = 1 stroke, C = 1 stroke.\n'
                  'Any order; stroke declarations are NOT inspected part quantities.\n'
                  'A 60-second deadline abandons the batch. Reset physical MCU for a new session.\n'
                  'No remote settings or reset commands are supported.',wraplength=800).pack(anchor='w')
        ttk.Label(settings,text='Fixture label (local export metadata only, maximum 120 characters)').pack(anchor='w',pady=12)
        ttk.Entry(settings,textvariable=self.label,width=70).pack(anchor='w')
        ttk.Button(settings,text='Export private CSV (never overwrite)',command=self.export).pack(anchor='w',pady=12)
        ttk.Label(settings,text='Exports retain the last 2000 snapshots, provenance, warnings and corruption count.\n'
                  'No cloud, personal identifiers, or automatic file upload.',wraplength=800).pack(anchor='w')
        root.protocol('WM_DELETE_WINDOW',self.close)
        self.timer=root.after(30,self.poll)

    def connect(self):
        if self.port is not None:
            self.status.set('Already connected; disconnect before selecting another port')
            return
        try:
            port=Serial(self.path.get())
        except (OSError,termios.error,ValueError) as exc:
            self.status.set('Connection failed: '+str(exc)); return
        self.port=port
        self.decoder=Decoder()
        self.session=Session('live')
        self.render()
        self.status.set('Connected; waiting for two advancing valid snapshots')

    def disconnect(self):
        if self.port is not None:
            self.port.close(); self.port=None
            if self.decoder.buffer:
                self.decoder.errors+=1; self.decoder.buffer=b''
        self.session.connected=False
        self.session.errors=self.decoder.errors
        self.status.set('Disconnected; retained evidence is not live')
        self.render()

    def poll(self):
        if self.port is not None:
            try:
                data=self.port.read()
                rows=self.decoder.feed(data)
                self.session.errors=self.decoder.errors
                for row in rows: self.session.accept(row)
                if rows: self.render()
                self.status.set('LIVE' if self.session.fresh() else 'Waiting / STALE; no current progress confirmed')
            except (OSError,ValueError) as exc:
                self.disconnect(); self.status.set(str(exc))
        self.timer=self.root.after(30,self.poll)

    def render(self):
        if not self.session.rows:
            self.summary.set('No samples')
        else:
            r=self.session.rows[-1]
            self.summary.set(f"{STATES[r['state']]} | A {r['a']}/2   B {r['b']}/1   C {r['c']}/1\n"
                             f"Matching {r['good']}   Mismatching {r['bad']}   Abandoned {r['aborted']}\n"
                             f"Batch {r['starts']}   Age {r['age']} ms   Tainted {r['tainted']}   "
                             f"Gaps {r['gaps']}   Faults {r['faults']}\n"
                             'Counts describe qualified paddle strokes, not inspected parts.')
        self.history.delete(*self.history.get_children())
        # Show changes in retained outcomes; sparse telemetry may skip batches.
        previous=None
        for r in self.session.rows:
            key=(r['good'],r['bad'],r['aborted'])
            if key!=previous and r['last_result']:
                self.history.insert('', 'end',values=(r['now'],('none','matching','mismatching','abandoned')[r['last_result']],
                    f"{r['last_a']} / {r['last_b']} / {r['last_c']}",r['last_age']))
            previous=key

    def replay_path(self, path):
        if self.port is not None: raise ValueError('disconnect before replacing evidence')
        session=load_replay(path)
        self.session=session; self.decoder=Decoder()
        self.render(); self.status.set('REPLAY — never live; source may be synthetic')

    def replay(self):
        path=filedialog.askopenfilename(title='Open KM1 recording')
        if not path: return
        try: self.replay_path(path)
        except (OSError,ValueError) as exc: self.status.set('Replay rejected: '+str(exc))

    def export_to(self,path):
        self.session.export(path,self.label.get())
        self.status.set('Exported '+str(path))

    def export(self):
        path=filedialog.asksaveasfilename(title='New CSV file',defaultextension='.csv')
        if not path: return
        try: self.export_to(path)
        except (OSError,ValueError) as exc: self.status.set('Export failed: '+str(exc))

    def close(self):
        self.root.after_cancel(self.timer)
        self.disconnect()
        self.root.destroy()

if __name__=='__main__':
    root=tk.Tk(); Workbench(root); root.mainloop()
