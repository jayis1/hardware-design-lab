# Author: jayis1. SPDX-License-Identifier: MIT
"""Linux desktop exposure workbench. No network, commands or third-party packages."""
import argparse
import termios
import time
import tkinter as tk
from tkinter import ttk, filedialog
from model import Session
from transport import Decoder, Serial, load_replay

class Workbench:
    def __init__(self,root):
        self.root=root
        root.title('Shutter Quartet — jayis1')
        root.geometry('1000x650')
        self.port=None
        self.decoder=Decoder()
        self.session=Session()
        self.path=tk.StringVar(value='/dev/ttyACM0')
        self.status=tk.StringVar(value='OFFLINE')
        self.message=tk.StringVar(value='Darken all four detectors before a sweep')
        self.advice=tk.StringVar()
        self.tolerance=tk.StringVar(value='10')
        bar=ttk.Frame(root); bar.pack(fill='x')
        ttk.Entry(bar,textvariable=self.path,width=28).pack(side='left')
        for label,command in [('Connect',self.connect),('Disconnect',self.disconnect),
                              ('Replay',self.replay),('Export CSV',self.export)]:
            ttk.Button(bar,text=label,command=command).pack(side='left')
        ttk.Label(root,textvariable=self.status).pack(anchor='w')
        ttk.Label(root,textvariable=self.message,wraplength=950).pack(anchor='w')
        self.tabs=ttk.Notebook(root); self.tabs.pack(fill='both',expand=True)
        live=ttk.Frame(self.tabs); history=ttk.Frame(self.tabs); setup=ttk.Frame(self.tabs)
        self.tabs.add(live,text='Exposure comparison')
        self.tabs.add(history,text='Snapshot notebook')
        self.tabs.add(setup,text='Setup and interpretation')
        ttk.Label(live,textvariable=self.advice,wraplength=950).pack(anchor='w',pady=12)
        row=ttk.Frame(live); row.pack(anchor='w')
        ttk.Label(row,text='Local width-spread tolerance (ms), not a camera specification:').pack(side='left')
        ttk.Entry(row,textvariable=self.tolerance,width=8).pack(side='left')
        self.points=ttk.Treeview(live,columns=('first','last','lit','pulses'),height=4)
        self.points.heading('#0',text='Point')
        for key in ('first','last','lit','pulses'): self.points.heading(key,text=key+' (ms)' if key!='pulses' else 'Segments')
        self.points.pack(fill='x')
        for i in range(4): self.points.insert('', 'end',iid=str(i),text='ABCD'[i])
        self.summary=tk.StringVar(); ttk.Label(live,textvariable=self.summary).pack(anchor='w',pady=12)
        self.history=ttk.Treeview(history,columns=('time','state','completed','aborted','seen','span'),show='headings')
        for key in self.history['columns']: self.history.heading(key,text=key)
        self.history.pack(fill='both',expand=True)
        text=tk.Text(setup,wrap='word'); text.pack(fill='both',expand=True)
        text.insert('1.0',
            'Author: jayis1\n\nFour unpowered TEPT4400 NPN phototransistors: collectors to J3–J6 pin 1, '
            'emitters to pin 2 (GND). Verify actual datasheet lead orientation. J1 is 3V3/GND from Nucleo; '
            'J2 goes to A0/A1/A2/A3. Never connect powered sensor outputs or mains.\n\n'
            'Use an enclosed, low-power, steady diffuse visible light source and a hand-operated opaque shutter. '
            'This is a slow teaching fixture, not a camera shutter calibrator. Sensor thresholds and filtering bias timings. '
            'No lasers, high-power lamps or driven machinery.\n\n'
            'Hold all points dark for 250 ms after qualification. Any qualified illumination starts a capture. '
            'Dark intervals shorter than the clear window belong to that capture. Qualified reopening increases segments. '
            'A full dark window publishes the result; a 10 s deadline or acquisition gap aborts it. '
            'Retained results exclude aborted captures.\n\n'
            'Missing points are not zero-duration exposures. Total lit time includes all qualified segments; '
            'last minus first is the envelope, not exposure when fragmented. The comparison tolerance is local and '
            'never sent to firmware. No radiometric dose, film exposure, EV error or accuracy claim is made.\n\n'
            'LIVE refers to advancing telemetry only. CRC is not authentication. Export before reconnecting; '
            'new sessions clear the desktop notebook but not MCU totals. Replays are always OFFLINE. '
            'Counter/time rollback blocks the session, but resets with forward-looking evidence can be undetectable. '
            'Only 500 snapshots are retained; completion jumps can omit individual captures.\n')
        text.configure(state='disabled')
        root.protocol('WM_DELETE_WINDOW',self.close)
        self.job=root.after(20,self.poll)
        self.render()
    def connect(self):
        if self.port is not None:
            self.message.set('Already connected; disconnect first'); return
        try:
            candidate=Serial(self.path.get())
        except (OSError,termios.error) as exc:
            self.message.set('Connection failed: '+str(exc)); return
        self.port=candidate
        self.decoder=Decoder()
        self.session=Session('live'); self.session.connected=True
        self.message.set('Connected; waiting for advancing SQ1 records')
        self.render()
    def disconnect(self):
        if self.port:
            self.port.close(); self.port=None
            try: self.decoder.finish()
            except ValueError: pass
            self.session.errors=self.decoder.errors
        self.session.connected=False
        self.render()
    def replay_path(self,path):
        if self.port is not None: raise ValueError('Disconnect before replay')
        replacement=load_replay(path)
        self.session=replacement
        self.message.set('Offline replay; no physical measurement implied')
        self.render()
    def replay(self):
        path=filedialog.askopenfilename()
        if path:
            try: self.replay_path(path)
            except (OSError,ValueError) as exc: self.message.set(str(exc))
    def export_path(self,path):
        self.session.export(path)
        self.message.set('Exported private CSV without overwriting an existing file')
    def export(self):
        path=filedialog.asksaveasfilename(defaultextension='.csv')
        if path:
            try: self.export_path(path)
            except (OSError,ValueError) as exc: self.message.set(str(exc))
    def poll(self):
        if self.port is not None and not self.session.blocked:
            try:
                data=self.port.read()
                rows=self.decoder.feed(data)
                self.session.errors=self.decoder.errors
                for row in rows: self.session.accept(row)
            except (OSError,termios.error,ValueError) as exc:
                self.message.set(str(exc)); self.disconnect()
        self.render()
        self.job=self.root.after(20,self.poll)
    def render(self):
        self.status.set(self.session.freshness()+' — source: '+self.session.source)
        try:
            tolerance=int(self.tolerance.get())
            if not 0<=tolerance<10000: raise ValueError()
            self.advice.set(self.session.decision(tolerance))
        except ValueError:
            self.advice.set('Enter a local tolerance from 0 to 9999 ms')
        r=self.session.rows[-1] if self.session.rows else None
        for i in range(4):
            values=[r[f'{key}{i}'] for key in ('first','last','lit','pulses')] if r and r['seen']&(1<<i) else ['—']*4
            self.points.item(str(i),values=values)
        self.summary.set('No snapshot' if r is None else
            f"Starts {r['starts']} / completed {r['completed']} / aborted {r['aborted']} / timeouts {r['timeouts']}\n"
            f"Current state {['WAIT','READY','ACTIVE','HALTED'][r['state']]} / gaps {r['gaps']} / flags {r['flags']} / parser errors {self.session.errors}")
        self.history.delete(*self.history.get_children())
        for row in self.session.rows:
            self.history.insert('','end',values=[row[k] for k in self.history['columns']])
    def close(self):
        self.root.after_cancel(self.job)
        self.disconnect()
        self.root.destroy()

if __name__=='__main__':
    parser=argparse.ArgumentParser(description='Shutter Quartet — author jayis1')
    parser.add_argument('--check',help='validate a recording without a display')
    args=parser.parse_args()
    if args.check:
        result=load_replay(args.check)
        print(f'Author: jayis1; {len(result.rows)} retained snapshots; '+result.decision(10))
    else:
        root=tk.Tk(); Workbench(root); root.mainloop()
