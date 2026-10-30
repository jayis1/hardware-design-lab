#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Receive-only manual sieve notebook; Python stdlib + Tk on Linux."""
import tkinter as tk
from tkinter import ttk, filedialog
from model import Session, STATES, ZONES, reasons
from transport import Decoder, Serial, load_replay

class Workbench:
    def __init__(self,root):
        self.root=root
        root.title('Sieve Shuttle — jayis1'); root.geometry('1100x720')
        self.port=None; self.session=Session('none'); self.decoder=Decoder()
        self.path=tk.StringVar(value='/dev/ttyACM0'); self.note=tk.StringVar()
        self.status=tk.StringVar(value='Disconnected. Motion evidence, not sieving efficiency.')
        self.summary=tk.StringVar(value='No evidence'); self.contacts=tk.StringVar()
        tabs=ttk.Notebook(root); tabs.pack(fill='both',expand=True)
        connection=ttk.Frame(tabs,padding=16); live=ttk.Frame(tabs,padding=16)
        history=ttk.Frame(tabs,padding=16)
        for tab,title in [(connection,'Connection / experiment'),(live,'Motion balance'),(history,'History')]:
            tabs.add(tab,text=title)
        ttk.Label(connection,text='USB serial 115200 8N1. Receive-only; no actuator or cloud.').pack(anchor='w')
        ttk.Entry(connection,textvariable=self.path,width=65).pack(anchor='w',pady=8)
        buttons=ttk.Frame(connection); buttons.pack(anchor='w')
        for text,command in [('Connect',self.connect),('Disconnect',self.disconnect),
                             ('Open recording',self.replay),('Export evidence',self.save)]:
            ttk.Button(buttons,text=text,command=command).pack(side='left',padx=3)
        ttk.Label(connection,text='Mesh, sample, zone spacing, mounting and run notes:').pack(anchor='w',pady=12)
        ttk.Entry(connection,textvariable=self.note,width=100).pack(anchor='w')
        ttk.Label(connection,text=(
            'Move one captive sieve tray within four disjoint zones: West, East, North, South.\n'
            'Start clear for 100 ms. Dwell in a zone for at least 20 ms and clear for 150 ms.\n'
            'A visit commits only after a 100 ms fully qualified clear window.\n'
            'West ↔ East counts X; North ↔ South counts Y; adjacent zones count axis turns.\n'
            'A repeated zone counts a return. Returns and turns break opposite-traversal runs.\n\n'
            'Do not infer particle size, applied force, acceleration, mixing or process completion.\n'
            'Overlapping zones, sampling loss and a 30 s visit halt the notebook.\n'
            'Export, center the tray, reset the board and reconnect to begin a new run.\n'
            'Replay is never live. Reconnect does not reset the MCU.'),justify='left').pack(anchor='w',pady=18)
        ttk.Label(live,textvariable=self.summary,font=('TkDefaultFont',13),justify='left').pack(anchor='w')
        ttk.Label(live,textvariable=self.contacts).pack(anchor='w',pady=8)
        self.matrix=ttk.Treeview(live,columns=('from','West','East','North','South'),show='headings',height=4)
        for name in self.matrix['columns']: self.matrix.heading(name,text=name)
        self.matrix.pack(fill='x')
        ttk.Label(live,text='Rows: previous zone. Columns: new zone. Counts, not travel distance.').pack(anchor='w')
        columns=('ms','state','visits','X','Y','returns','turns','last')
        self.history=ttk.Treeview(history,columns=columns,show='headings',height=20)
        for column in columns:
            self.history.heading(column,text=column); self.history.column(column,width=110)
        self.history.pack(fill='both',expand=True)
        ttk.Label(history,text='Latest 2,000 snapshots, not a complete event log. Export retains session warnings.').pack(anchor='w')
        ttk.Label(root,textvariable=self.status,wraplength=1050).pack(fill='x',padx=12,pady=10)
        self.timer=root.after(25,self.poll); root.protocol('WM_DELETE_WINDOW',self.close)
    def connect(self):
        if self.port:
            self.status.set('Already connected; disconnect first.'); return
        try: port=Serial(self.path.get())
        except Exception as exc:
            self.status.set(f'Connect failed: {exc}'); return
        self.port=port; self.decoder=Decoder(); self.session=Session('live')
        self.session.connected=True
        self.status.set('Connected; awaiting advancing evidence.'); self.render()
    def disconnect(self):
        if self.port:
            self.port.close(); self.port=None
            try: self.decoder.finish()
            except ValueError: pass
            self.session.errors=self.decoder.errors
        self.session.connected=False
        self.status.set('Disconnected; evidence retained, not live.'); self.render()
    def replay(self,path=None):
        if self.port:
            self.status.set('Disconnect before replay.'); return
        path=path or filedialog.askopenfilename(title='Open SS1 recording')
        if not path: return
        try: replacement=load_replay(path)
        except Exception as exc:
            self.status.set(f'Replay failed: {exc}'); return
        self.session=replacement; self.status.set('REPLAY — never live hardware.'); self.render()
    def save(self,path=None):
        path=path or filedialog.asksaveasfilename(defaultextension='.json',title='Export to a NEW file')
        if not path: return
        try:
            self.session.export(path,self.note.get()); self.status.set(f'Exported: {path}')
        except Exception as exc: self.status.set(f'Export failed: {exc}')
    def render(self):
        self.history.delete(*self.history.get_children()); self.matrix.delete(*self.matrix.get_children())
        for r in self.session.rows:
            self.history.insert('','end',values=(r['now'],STATES[r['state']],r['visits'],r['x'],r['y'],
                                                r['returns'],r['turns'],ZONES[r['last']]))
        if not self.session.rows:
            self.summary.set('No evidence'); self.contacts.set('No input sample'); return
        r=self.session.rows[-1]
        self.summary.set(f"{STATES[r['state']]} — {reasons(r['fault'])}\n"
                         f"Visits: {r['visits']}   X traversals: {r['x']}   Y traversals: {r['y']}\n"
                         f"Same-zone returns: {r['returns']}   Axis turns: {r['turns']}\n"
                         f"Longest X run: {r['peak_x']}   Longest Y run: {r['peak_y']}\n"
                         f"Current run: {r['run']}   Last zone: {ZONES[r['last']]}\n"
                         f"Pending: {ZONES[r['pending']]}   Clear: {r['clear_ms']} ms")
        self.contacts.set(f"Raw mask {r['raw']:04b}; qualified mask {r['mask']:04b}; valid bits {r['qualified']:04b}. "
                          f"Gaps {r['gaps']}; TX errors {r['tx_errors']}")
        for a,name in enumerate(ZONES[:4]):
            self.matrix.insert('','end',values=[name]+[r[f'm{4*a+b}'] for b in range(4)])
    def poll(self):
        if self.port:
            try:
                rows=self.decoder.feed(self.port.read()); self.session.errors=self.decoder.errors
                for row in rows: self.session.accept(row)
                if rows: self.render()
                if self.session.fresh(): self.status.set(f'LIVE stream; parser errors {self.session.errors}; check device fault separately.')
                else: self.status.set('WAITING / STALE — no fresh advancing stream.')
            except Exception as exc:
                self.disconnect(); self.session.warning=str(exc)
                self.status.set(f'Disconnected: {exc}. Evidence retained.')
        self.timer=self.root.after(25,self.poll)
    def close(self):
        self.root.after_cancel(self.timer); self.disconnect(); self.root.destroy()
if __name__=='__main__':
    window=tk.Tk(); Workbench(window); window.mainloop()
