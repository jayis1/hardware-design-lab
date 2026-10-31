#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Linux local companion: serial setup, live crossing trace, evidence export."""
import argparse
from pathlib import Path
import termios
import tkinter as tk
from tkinter import ttk
from model import Session, STATES
from transport import Serial, Decoder, load_replay

class Workbench:
    def __init__(self, root):
        self.root=root
        root.title('Rebound Rungs — jayis1')
        root.geometry('980x640')
        self.serial=None
        self.session=Session('live')
        self.decoder=Decoder()
        self.port=tk.StringVar(value='/dev/ttyACM0')
        self.replay=tk.StringVar()
        self.output=tk.StringVar(value=str(Path.home()/'rebound-evidence.csv'))
        self.fixture=tk.StringVar(value='Record slider mass, gate zones, sample and release position')
        self.message=tk.StringVar(value='One trial per hardware reset. Position above all gates before resetting.')
        self.result=tk.StringVar(value='No evidence')
        self.health=tk.StringVar(value='Disconnected')
        self.tabs=ttk.Notebook(root); self.tabs.pack(fill='both',expand=True)
        connect=ttk.Frame(self.tabs,padding=14)
        live=ttk.Frame(self.tabs,padding=14)
        evidence=ttk.Frame(self.tabs,padding=14)
        self.tabs.add(connect,text='Connection')
        self.tabs.add(live,text='Crossings')
        self.tabs.add(evidence,text='Evidence')
        ttk.Label(connect,text='Serial device (115200 8N1; receive only)').pack(anchor='w')
        ttk.Entry(connect,textvariable=self.port,width=80).pack(fill='x',pady=6)
        buttons=ttk.Frame(connect); buttons.pack(anchor='w')
        ttk.Button(buttons,text='Connect / new session',command=self.connect).pack(side='left')
        ttk.Button(buttons,text='Disconnect',command=self.disconnect).pack(side='left')
        ttk.Label(connect,text='RR1 raw recording (regular file, at most 2 MiB)').pack(anchor='w',pady=10)
        ttk.Entry(connect,textvariable=self.replay,width=80).pack(fill='x')
        ttk.Button(connect,text='Load replay',command=self.load).pack(anchor='w',pady=6)
        ttk.Label(connect,text='Reset the Nucleo only after exporting. Reconnect after reset.\n'
                  'No actuator control, cloud connection, or calibration claim.\n'
                  'A timeout is invalid evidence, not a zero-height bounce.').pack(anchor='w',pady=16)
        ttk.Label(live,textvariable=self.result,font=('TkDefaultFont',15)).pack(anchor='w')
        ttk.Label(live,textvariable=self.health).pack(anchor='w',pady=8)
        self.table=ttk.Treeview(live,columns=('gate','down','up'),show='headings',height=5)
        for key,label in [('gate','Rung'),('down','Descent closure (ms)'),('up','Rebound closure (ms)')]:
            self.table.heading(key,text=label)
        self.table.pack(fill='x',pady=10)
        self.detail=tk.Text(live,height=12,wrap='word'); self.detail.pack(fill='both',expand=True)
        ttk.Label(evidence,text='Fixture and specimen notes (CSV metadata, not calibration)').pack(anchor='w')
        ttk.Entry(evidence,textvariable=self.fixture,width=100).pack(fill='x',pady=6)
        ttk.Label(evidence,text='New export filename; existing files are never overwritten').pack(anchor='w')
        ttk.Entry(evidence,textvariable=self.output,width=100).pack(fill='x',pady=6)
        ttk.Button(evidence,text='Export retained evidence',command=self.export).pack(anchor='w')
        ttk.Label(evidence,text='Exports include source, freshness, corruption count and warnings.\n'
                  'The most recent 2048 snapshots are retained; dropped rows are counted.\n'
                  'Gate visits are not a coefficient of restitution or a material certification.').pack(anchor='w',pady=20)
        ttk.Label(root,textvariable=self.message,wraplength=950).pack(fill='x',padx=12,pady=10)
        root.protocol('WM_DELETE_WINDOW',self.close)
        self.timer=root.after(25,self.poll)
    def connect(self):
        if self.serial is not None:
            self.message.set('Already connected; disconnect first')
            return
        try:
            serial=Serial(self.port.get())
        except (OSError,termios.error) as exc:
            self.message.set(str(exc)); return
        self.serial=serial
        self.decoder=Decoder()
        self.session=Session('live'); self.session.connected=True
        self.message.set('Connected; awaiting two advancing snapshots')
        self.render()
    def disconnect(self):
        if self.serial is not None:
            self.serial.close(); self.serial=None
            try: self.decoder.finish()
            except ValueError: pass
            self.session.errors=self.decoder.errors
        self.session.connected=False
        self.render()
    def load(self):
        if self.serial is not None:
            self.message.set('Disconnect before replacing evidence with replay'); return
        try:
            session=load_replay(self.replay.get())
        except (OSError,ValueError) as exc:
            self.message.set(str(exc)); return
        self.session=session
        self.message.set('Replay loaded; never live')
        self.render()
    def export(self):
        try:
            self.session.export(self.output.get(),self.fixture.get())
            self.message.set('Exported '+self.output.get())
        except (OSError,ValueError) as exc:
            self.message.set(str(exc))
    def render(self):
        self.result.set(self.session.result())
        status='LIVE' if self.session.fresh() else 'NOT LIVE'
        self.health.set(f'{status} | {self.session.provenance} | retained {len(self.session.rows)} | '
                        f'parser errors {self.session.errors} | {self.session.warning}')
        self.table.delete(*self.table.get_children())
        self.detail.delete('1.0','end')
        if not self.session.rows: return
        row=self.session.rows[-1]
        for i in range(3,-1,-1):
            down=row[f'down{i}'] if row['downmask'] & (1<<i) else 'not visited'
            up=row[f'up{i}'] if row['upmask'] & (1<<i) else 'not visited'
            self.table.insert('','end',values=(i,down,up))
        self.detail.insert('end',f"State: {STATES[row['state']]}\nRaw closed mask: {row['raw']}\n"
                           f"Accepted crossings: {row['events']}\nTrial duration: {row['age']} ms\n"
                           f"Sampling gaps: {row['gaps']}\nTransmit losses: {row['tx_errors']}\n"
                           f"Fault mask: {row['faults']}\n\n"
                           'Times refer to qualified closures, not the impact. A repeated highest gate\n'
                           'bounds the observed first return only under the single-slider geometry\n'
                           'and uninterrupted pulse-detection assumptions. Read README before assembly.')
    def poll(self):
        if self.serial is not None:
            try:
                data=self.serial.read()
                rows=self.decoder.feed(data)
                self.session.errors=self.decoder.errors
                for row in rows: self.session.accept(row)
            except (OSError,ValueError,termios.error) as exc:
                self.message.set(str(exc))
                self.disconnect()
        self.render()
        self.timer=self.root.after(25,self.poll)
    def close(self):
        self.root.after_cancel(self.timer)
        self.disconnect()
        self.root.destroy()

def main():
    parser=argparse.ArgumentParser(description='Rebound Rungs, author jayis1')
    parser.add_argument('--check',metavar='RECORDING',help='strictly validate a recording without a display')
    args=parser.parse_args()
    if args.check:
        session=load_replay(args.check)
        print(session.result())
    else:
        root=tk.Tk(); Workbench(root); root.mainloop()

if __name__=='__main__': main()
