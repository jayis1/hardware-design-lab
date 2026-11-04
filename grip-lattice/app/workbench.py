#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Local Linux/Tk workbench; serial reception never controls the target."""
import termios
import time
import tkinter as tk
from tkinter import ttk, filedialog
from model import Session, PHASES, FAULTS, LABELS
from transport import Decoder, Serial, load_replay

class Workbench:
    def __init__(self, root):
        self.root=root
        root.title('Grip Lattice — jayis1')
        root.geometry('1050x620')
        self.serial=None
        self.decoder=Decoder()
        self.session=Session('none')
        self.port=tk.StringVar(value='/dev/ttyACM0')
        self.status=tk.StringVar(value='Disconnected — no measurements')
        self.summary=tk.StringVar(value='No evidence')
        self.note=tk.StringVar()
        controls=ttk.Frame(root); controls.pack(fill='x',padx=8,pady=8)
        ttk.Entry(controls,textvariable=self.port,width=28).pack(side='left')
        for label,command in [('Connect',self.connect),('Disconnect',self.disconnect),
                              ('Replay',self.choose_replay),('Export CSV',self.choose_export)]:
            ttk.Button(controls,text=label,command=command).pack(side='left',padx=4)
        ttk.Label(root,textvariable=self.status).pack(anchor='w',padx=8)
        ttk.Label(root,textvariable=self.summary).pack(anchor='w',padx=8,pady=8)
        self.tabs=ttk.Notebook(root); self.tabs.pack(fill='both',expand=True,padx=8)
        live=ttk.Frame(self.tabs); history=ttk.Frame(self.tabs); setup=ttk.Frame(self.tabs)
        self.tabs.add(live,text='Contact lattice'); self.tabs.add(history,text='Snapshot history')
        self.tabs.add(setup,text='Setup and interpretation')
        self.lattice=ttk.Treeview(live,columns=('combination','time','runs','longest'),show='headings')
        for k,label in zip(self.lattice['columns'],('Combination','Observed ms','Segments','Longest ms')):
            self.lattice.heading(k,text=label)
        self.lattice.pack(fill='both',expand=True)
        self.history=ttk.Treeview(history,columns=('time','trial','phase','age','unknown'),show='headings')
        for k in self.history['columns']: self.history.heading(k,text=k)
        self.history.pack(fill='both',expand=True)
        instructions=(
            'Author: jayis1\n\n'
            'Use an unpowered dummy handle, three low-force contact paddles and a footswitch.\n'
            'J3 = thumb; J4 = fingers; J5 = palm; J6 = RECORD. Never attach powered tool wiring.\n'
            'Release RECORD for at least 250 ms after qualification. READY permits a new trial.\n'
            'Hold RECORD to acquire; release to finish. Keep each trial below 60 seconds.\n'
            'Contact changes and raw glitches create UNKNOWN intervals, not inferred grip time.\n'
            'Runs are observed segments; switch bounce can split one physical contact episode.\n'
            'No force, skin pressure, health recommendation, user identity or safety rating is measured.\n'
            'HALT freezes accounting: export first, reset target, then explicitly reconnect.\n'
            'LIVE requires advancing device time. Replay is always labelled replay.\n'
            'Only the newest 2000 snapshots are kept. Each new start replaces target trial bins.\n'
            'Use local notes to identify geometry and procedure, not people. CSV export refuses overwrite.')
        ttk.Label(setup,text=instructions,justify='left',wraplength=960).pack(anchor='w',padx=12,pady=12)
        ttk.Label(root,text='Trial note (local only)').pack(anchor='w',padx=8)
        ttk.Entry(root,textvariable=self.note).pack(fill='x',padx=8,pady=6)
        root.protocol('WM_DELETE_WINDOW',self.close)
        self.timer=root.after(25,self.poll)

    def disconnect(self):
        if self.serial is not None: self.serial.close(); self.serial=None
        self.session.connected=False
        self.status.set('Disconnected; retained evidence is not live')

    def connect(self):
        if self.serial is not None:
            self.status.set('Already connected; disconnect before opening another port')
            return
        try: port=Serial(self.port.get())
        except (OSError,termios.error) as exc:
            self.status.set('Connection failed: '+str(exc)); return
        self.serial=port
        self.decoder=Decoder()
        self.session=Session('live'); self.session.connected=True
        self.render()
        self.status.set('Connected; waiting for two advancing snapshots')

    def replay_path(self,path):
        # Validate before replacing the current evidence or closing a live port.
        replacement=load_replay(path)
        self.disconnect()
        self.session=replacement
        self.render()
        self.status.set('REPLAY — file data, never live measurements')

    def choose_replay(self):
        path=filedialog.askopenfilename()
        if not path: return
        try: self.replay_path(path)
        except (OSError,ValueError) as exc: self.status.set('Replay rejected: '+str(exc))

    def choose_export(self):
        path=filedialog.asksaveasfilename(defaultextension='.csv')
        if not path: return
        try:
            self.session.export(path,time.monotonic(),self.note.get())
            self.status.set('Exported private CSV; retained provenance and qualifications')
        except OSError as exc: self.status.set('Export failed: '+str(exc))

    def render(self):
        self.lattice.delete(*self.lattice.get_children())
        self.history.delete(*self.history.get_children())
        if not self.session.rows:
            self.summary.set('No evidence'); return
        r=self.session.rows[-1]
        self.summary.set(f"{PHASES[r['phase']]} | Starts {r['starts']} | Completed {r['completed']} | "
                         f"Aborted {r['aborted']} | Age {r['age']} ms | Unknown {r['unknown']} ms | "
                         f"Fault: {FAULTS[r['fault']]}")
        for i,label in enumerate(LABELS):
            self.lattice.insert('', 'end', values=(label,r[f'h{i}'],r[f'r{i}'],r[f'm{i}']))
        for row in self.session.rows:
            self.history.insert('', 'end',values=(row['now'],row['starts'],PHASES[row['phase']],
                                                row['age'],row['unknown']))

    def poll(self):
        try:
            if self.serial is not None:
                data=self.serial.read()
                if data:
                    rows=self.decoder.feed(data)
                    self.session.errors=self.decoder.errors
                    for row in rows: self.session.accept(row,time.monotonic())
                    self.render()
                if self.session.fresh(time.monotonic()):
                    self.status.set(f'LIVE — parser errors {self.session.errors}; contact evidence only')
                else:
                    self.status.set('Not live: waiting, stale, or acquisition halted; values are retained')
        except (OSError,ValueError) as exc:
            self.session.errors=self.decoder.errors
            self.disconnect()
            self.status.set('Stopped: '+str(exc))
        self.timer=self.root.after(25,self.poll)

    def close(self):
        self.root.after_cancel(self.timer)
        self.disconnect()
        self.root.destroy()

if __name__=='__main__':
    root=tk.Tk(); Workbench(root); root.mainloop()
