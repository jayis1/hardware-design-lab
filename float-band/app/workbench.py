#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Float Band: local Linux live acquisition, replay and nonoverwriting export."""
import argparse
import termios
import tkinter as tk
from tkinter import ttk, filedialog, messagebox
from model import Session
from transport import Serial, Decoder, load_replay

BANDS=['below low','low–mid','mid–high','above high','unknown / disturbed']
class Workbench:
    def __init__(self,root):
        self.root=root
        self.root.title('Float Band — jayis1')
        self.port=None
        self.decoder=Decoder()
        self.session=Session('live')
        self.path=tk.StringVar(value='/dev/ttyACM0')
        self.status=tk.StringVar(value='DISCONNECTED')
        self.summary=tk.StringVar(value='No evidence')
        controls=ttk.Frame(root,padding=8); controls.pack(fill='x')
        ttk.Entry(controls,textvariable=self.path,width=28).pack(side='left')
        for text,action in [('Connect',lambda:self.connect(self.path.get())),
                            ('Disconnect',self.disconnect),('Replay',self.choose_replay),
                            ('Export CSV',self.choose_export)]:
            ttk.Button(controls,text=text,command=lambda a=action:self.guarded(a)).pack(side='left')
        ttk.Label(root,textvariable=self.status,padding=8).pack(anchor='w')
        tabs=ttk.Notebook(root); tabs.pack(fill='both',expand=True)
        measurements=ttk.Frame(tabs,padding=8); tabs.add(measurements,text='Switching bands')
        self.table=ttk.Treeview(measurements,columns=('band','open','close'),show='headings',height=5)
        for key,label in [('band','Reference bracket'),('open','Opening edges'),('close','Closing edges')]:
            self.table.heading(key,text=label)
        for i,label in enumerate(BANDS): self.table.insert('','end',iid=str(i),values=(label,0,0))
        self.table.pack(fill='x')
        ttk.Label(measurements,textvariable=self.summary,padding=8).pack(anchor='w')
        info=ttk.Frame(tabs,padding=12); tabs.add(info,text='Setup and interpretation')
        ttk.Label(info,justify='left',wraplength=740,text=(
            'Author: jayis1\n\n'
            'J3 LOW → A0 / PA0; J4 MID → A1 / PA1; J5 HIGH → A2 / PA4; J6 DUT → A3 / PB0.\n'
            'Each field connector: pin 1 signal, pin 2 ground. J1: 3V3, GND.\n'
            'References must close when submerged, in ascending physical order.\n\n'
            'Only disconnected voltage-free switches. No pumps, powered outputs or mains.\n'
            'Keep the carrier and Nucleo dry above the spill tray. Observe while filling and draining slowly.\n\n'
            'Counts describe qualified electrical edges, not exact water heights or passed safety tests.\n'
            'Unknown means a reference changed during the raw edge envelope, was invalid, or was not ready.\n'
            'The direction is only the latest adjacent reference movement, not proof of current flow.\n'
            'Compare distributions; this application does not compute calibrated hysteresis in millimetres.\n\n'
            'Replay is explicitly synthetic or recorded evidence, never live. Reset MCU for a new physical run;\n'
            'reconnecting the app does not clear the MCU counters. All firmware history is volatile.')).pack(anchor='w')
        self.root.protocol('WM_DELETE_WINDOW',self.close)
        self.timer=self.root.after(25,self.tick)
        self.refresh()

    def guarded(self,action):
        try: action()
        except (ValueError,OSError,termios.error) as exc: messagebox.showerror('Float Band',str(exc))

    def connect(self,path):
        if self.port is not None: raise ValueError('Disconnect before opening another port')
        port=Serial(path) # Failed open must preserve existing evidence.
        self.port=port
        self.decoder=Decoder()
        self.session=Session('live'); self.session.connected=True
        self.refresh()

    def disconnect(self):
        if self.port is not None: self.port.close(); self.port=None
        if self.decoder.buffer:
            self.decoder.errors+=1; self.decoder.buffer=b''
        self.session.errors=self.decoder.errors
        self.session.connected=False
        self.refresh()

    def replay(self,path):
        if self.port is not None: raise ValueError('Disconnect before replay')
        replacement=load_replay(path)
        self.session=replacement
        self.decoder=Decoder()
        self.refresh()

    def choose_replay(self):
        path=filedialog.askopenfilename(title='Open FB1 recording (max 2 MiB)')
        if path: self.replay(path)

    def choose_export(self):
        path=filedialog.asksaveasfilename(title='Export to NEW file',defaultextension='.csv')
        if path: self.session.export(path)

    def refresh(self):
        self.status.set(self.session.status())
        row=self.session.last
        for i,label in enumerate(BANDS):
            self.table.item(str(i),values=(label,row[f'open{i}'] if row else 0,row[f'close{i}'] if row else 0))
        if row:
            direction=['unknown','last moved upward','last moved downward'][row['direction']]
            self.summary.set(f"Qualified band: {BANDS[row['band']]} | {direction}\n"
                f"Edges {row['edges']} | rejected raw excursions {row['noise']} | invalid-reference samples {row['invalid_ticks']}\n"
                f"Skipped bands {row['skips']} | reversals {row['reversals']} | fault mask {row['fault']} | parser errors {self.session.errors}")
        else: self.summary.set('No evidence')

    def tick(self):
        if self.port is not None:
            try:
                data=self.port.read()
                rows=self.decoder.feed(data)
                self.session.errors=self.decoder.errors
                for row in rows: self.session.accept(row)
            except (OSError,termios.error,ValueError):
                self.disconnect()
        self.refresh()
        self.timer=self.root.after(25,self.tick)

    def close(self):
        self.root.after_cancel(self.timer)
        self.disconnect()
        self.root.destroy()

if __name__=='__main__':
    parser=argparse.ArgumentParser(description='Float Band, author jayis1')
    parser.add_argument('--replay')
    parser.add_argument('--export',help='validate replay and export CSV without opening GUI')
    args=parser.parse_args()
    if args.export:
        if not args.replay: parser.error('--export requires --replay')
        session=load_replay(args.replay); session.export(args.export)
        print(f'Exported {len(session.rows)} validated snapshots (replay)')
    else:
        root=tk.Tk(); app=Workbench(root)
        if args.replay: app.replay(args.replay)
        root.mainloop()
