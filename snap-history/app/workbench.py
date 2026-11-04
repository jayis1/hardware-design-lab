# Author: jayis1. SPDX-License-Identifier: MIT
"""Linux Tk notebook. No remote services, commands to the MCU, or sample data."""
import csv
import json
import os
import termios
import tkinter as tk
from tkinter import ttk, filedialog, messagebox
from model import Session, PHASES, FAULTS
from transport import Decoder, Serial, load_replay

class Workbench:
    def __init__(self, root):
        self.root=root
        root.title('Snap History — jayis1')
        root.geometry('900x600')
        self.port=None
        self.decoder=Decoder()
        self.session=Session('replay')
        self.path=tk.StringVar(value='/dev/ttyACM0')
        self.status=tk.StringVar(value='Disconnected; no evidence')
        bar=ttk.Frame(root); bar.pack(fill='x')
        ttk.Entry(bar,textvariable=self.path,width=30).pack(side='left')
        for text,command in [('Connect',self.connect),('Disconnect',self.disconnect),
                             ('Replay',self.choose_replay),('Export CSV',self.choose_export)]:
            ttk.Button(bar,text=text,command=lambda f=command:self.guard(f)).pack(side='left')
        ttk.Label(root,textvariable=self.status,wraplength=880).pack(fill='x')
        tabs=ttk.Notebook(root); tabs.pack(fill='both',expand=True)
        history=ttk.Frame(tabs); tabs.add(history,text='Closure history')
        self.summary=tk.StringVar(value='No trial')
        ttk.Label(history,textvariable=self.summary).pack(fill='x')
        columns=('group','ms','closed','opened','occupancy')
        self.tree=ttk.Treeview(history,columns=columns,show='headings')
        for column in columns:
            self.tree.heading(column,text=column)
            self.tree.column(column,width=140)
        self.tree.pack(fill='both',expand=True)
        setup=ttk.Frame(tabs); tabs.add(setup,text='Setup and limits')
        text=('Author: jayis1\n\nJ1: pin 1 = Nucleo CN6.4 3V3; pin 2 = CN6.6 GND.\n'
              'J2 pins 1..4 = CN8.1..4 PA0, PA1, PA4, PB0.\n'
              'J3..J6 = SNAP_A..SNAP_D; pin 1 signal, pin 2 ground.\n\n'
              'Only isolated voltage-free contacts on a tabletop fabric mock-up.\n'
              'Not a worn garment, medical test or certified engagement detector.\n'
              'Open all contacts for 250 ms; wait READY. First closure starts the trial.\n'
              'Reopenings are retained. All closed for 250 ms ends the trial.\n'
              '60 s deadline; maximum 32 changed-vector groups.\n'
              'Tied changes share a row; no within-row order is inferred.\n'
              'Export before MCU reset. A reset starts a new volatile notebook.\n\n'
              'Fresh means advancing telemetry, not valid acquisition: inspect phase/fault.\n'
              'Replay is never live. Disconnect preserves evidence and provenance.\n'
              'No physical assembly or timing calibration has been verified.')
        ttk.Label(setup,text=text,justify='left',wraplength=850).pack(anchor='w',padx=12,pady=12)
        root.protocol('WM_DELETE_WINDOW',self.close)
        self.job=root.after(25,self.poll)
    def guard(self, operation):
        try: operation()
        except (ValueError,OSError,termios.error) as exc:
            self.status.set(str(exc)); messagebox.showerror('Snap History',str(exc),parent=self.root)
    def connect(self):
        if self.port is not None: raise ValueError('Disconnect before opening another port')
        # Establish new transport before replacing retained evidence.
        port=Serial(self.path.get())
        self.port=port; self.decoder=Decoder(); self.session=Session('live')
        self.render()
    def disconnect(self):
        if self.port is not None:
            if self.decoder.buffer: self.decoder.errors+=1; self.decoder.buffer=b''
            self.port.close(); self.port=None
        self.session.connected=False
        self.session.errors=self.decoder.errors
        self.render()
    def replay(self,path):
        if self.port is not None: raise ValueError('Disconnect before replay')
        session=load_replay(path)
        self.session=session; self.decoder=Decoder(); self.render()
    def choose_replay(self):
        path=filedialog.askopenfilename(parent=self.root)
        if path: self.replay(path)
    def choose_export(self):
        path=filedialog.asksaveasfilename(parent=self.root,defaultextension='.csv')
        if path: self.export(path)
    def export(self,path):
        metadata=dict(author='jayis1',protocol='SH1',provenance=self.session.provenance,
                      connected=self.session.connected,fresh=self.session.fresh(),
                      parser_errors=self.session.errors,warning=self.session.warning,
                      snapshot=self.session.rows[-1] if self.session.rows else None)
        # Exclusive create prevents accidental overwrite; restrictive permissions
        # keep participant notebook data local to the account by default.
        fd=os.open(path,os.O_WRONLY|os.O_CREAT|os.O_EXCL,0o600)
        with os.fdopen(fd,'w',newline='') as stream:
            writer=csv.writer(stream)
            writer.writerow(['metadata',json.dumps(metadata,sort_keys=True)])
            writer.writerow(['group','ms','closed','opened','occupancy'])
            for event in self.session.events(): writer.writerow(event.values())
    def poll(self):
        if self.port is not None:
            try:
                rows=self.decoder.feed(self.port.read())
                self.session.errors=self.decoder.errors
                for row in rows: self.session.accept(row)
            except (ValueError,OSError,termios.error) as exc:
                self.session.warning=str(exc)
                self.disconnect()
        self.render()
        self.job=self.root.after(25,self.poll)
    def render(self):
        state='LIVE' if self.session.fresh() else ('waiting/stale' if self.port else 'disconnected')
        self.status.set(f'{self.session.provenance} | {state} | parser errors {self.session.errors} | {self.session.warning}')
        if self.session.rows:
            r=self.session.rows[-1]
            self.summary.set(f'{PHASES[r["phase"]]} | fault: {FAULTS[r["fault"]]} | trial {r["age"]} ms | settlement {r["quiet"]}/250 ms | groups {r["count"]}/32')
        else: self.summary.set('No trial')
        self.tree.delete(*self.tree.get_children())
        for event in self.session.events(): self.tree.insert('','end',values=tuple(event.values()))
    def close(self):
        self.root.after_cancel(self.job)
        self.disconnect(); self.root.destroy()

if __name__=='__main__':
    root=tk.Tk(); Workbench(root); root.mainloop()
