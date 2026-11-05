#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Local Linux/Tk workbench; no network service or third-party packages."""
import tkinter as tk
from tkinter import ttk, filedialog, messagebox
import termios
from model import Session
from transport import Decoder, Serial, load_replay

class Workbench:
    def __init__(self, root):
        self.root = root
        self.port = None
        self.session = Session('none')
        self.decoder = Decoder()
        self.closed = False
        root.title('Detent Route — jayis1 — unbuilt prototype')
        root.geometry('1100x650')
        self.tabs = ttk.Notebook(root)
        self.tabs.pack(fill='both',expand=True)
        live, history, setup = [ttk.Frame(self.tabs) for _ in range(3)]
        for frame,title in zip((live,history,setup),('Connect / Live','History / Export','Setup / Limits')):
            self.tabs.add(frame,text=title)
        self.path = tk.StringVar(value='/dev/ttyACM0')
        ttk.Entry(live,textvariable=self.path,width=50).pack(pady=10)
        buttons=ttk.Frame(live);buttons.pack()
        for label, action in [('Connect',lambda:self.connect(self.path.get())),
                              ('Disconnect',self.disconnect),('Replay',self.choose_replay)]:
            ttk.Button(buttons,text=label,command=lambda f=action:self.guard(f)).pack(side='left')
        self.status=tk.StringVar(value='NOT LIVE — no session')
        self.summary=tk.StringVar(value='No station observations')
        ttk.Label(live,textvariable=self.status,wraplength=1000).pack(pady=20)
        ttk.Label(live,textvariable=self.summary,wraplength=1000,font=('TkDefaultFont',13)).pack(pady=20)
        ttk.Label(live,text='CW/CCW name adjacent station order, not proven shaft travel.\n'
                  'No motor, guard, relay or machine-control connection is permitted.').pack()
        columns=('now','station','active','origin','age','cw','ccw','returns','jumps','aborted','last_ms')
        self.table=ttk.Treeview(history,columns=columns,show='headings',height=20)
        for c in columns:
            self.table.heading(c,text=c);self.table.column(c,width=85)
        self.table.pack(fill='both',expand=True)
        ttk.Button(history,text='Export private CSV (new file)',command=lambda:self.guard(self.choose_export)).pack(pady=10)
        text=tk.Text(setup,wrap='word',padx=20,pady=20)
        text.pack(fill='both',expand=True)
        text.insert('1.0','Author and creator: jayis1\n\n'
            'Mount four low-level dry switches at station marks 0, 1, 2, 3 around a slow manual indexer. '
            'Only one switch should close at a station; all should open between stations. '
            'J3/J4/J5/J6 correspond to those four identities. J1 is 3V3/GND; '
            'J2 goes to Nucleo CN8 pins 1–4 (PA0, PA1, PA4, PB0).\n\n'
            'Hold every intended station and all-open departure at least 6 ms; use 100 ms for bench checks. '
            'A station is an identity, not a calibrated angle. First stable singleton anchors without homing. '
            'A qualified all-open interval starts a journey. Return, clockwise neighbor, counterclockwise '
            'neighbor and opposite station are separate outcomes. Opposite arrivals do not earn two steps. '
            'Four consecutive same-direction adjacent moves earn a route lap, not a verified physical revolution.\n\n'
            'Multiple contacts abandon the active journey. Direct station changes without observed clear '
            'departure increment shortcuts. Sampling loss and ten-second timeout abandon evidence. '
            'Counters persist until reset; reconnect does not reset the device.\n\n'
            'USB serial is 115200 8N1. No commands are transmitted. A session needs advancing timestamps '
            'before LIVE appears. Two seconds without progress is stale. Host pauses or device/host drift '
            'beyond two seconds block acceptance; explicitly reconnect after exporting. '
            'Replay is never live and does not prove authenticity.\n\n'
            'The 2000-row history is a snapshot window, not an exact event log. CRC detects accidental '
            'corruption, not tampering. No persistent boot identity exists. Some resets and hidden motion '
            'remain undetectable. Use only detached voltage-free contacts and a guarded manual fixture. '
            'No fabrication, physical timing, angular accuracy or electrical safety testing is claimed.')
        text.configure(state='disabled')
        root.protocol('WM_DELETE_WINDOW',self.close)
        self.timer=root.after(20,self.poll)

    def guard(self, action):
        try: action()
        except (ValueError,OSError,termios.error) as exc:
            messagebox.showerror('Detent Route',str(exc),parent=self.root)

    def connect(self,path):
        if self.port is not None:
            raise ValueError('Disconnect the owned serial port first')
        candidate=Serial(path)  # Preserve previous evidence if opening fails.
        self.port=candidate
        self.session=Session('live');self.session.connected=True
        self.decoder=Decoder();self.render()

    def disconnect(self):
        if self.port is not None:
            self.port.close();self.port=None
        self.session.connected=False
        self.render()

    def replay(self,path):
        if self.port is not None:
            raise ValueError('Disconnect before replacing evidence with replay')
        candidate=load_replay(path)
        self.session=candidate;self.decoder=Decoder();self.render()

    def choose_replay(self):
        path=filedialog.askopenfilename(parent=self.root)
        if path:self.replay(path)

    def choose_export(self):
        path=filedialog.asksaveasfilename(parent=self.root,defaultextension='.csv')
        if path:self.session.export(path,self.decoder.errors)

    def render(self):
        s=self.session
        state='LIVE' if s.live() else 'NOT LIVE'
        self.status.set(f'{state} | source={s.source} | connected={s.connected} | '
                        f'blocked={s.blocked} | parser errors={self.decoder.errors} | '
                        f'sequence losses={s.losses} | {s.warning}')
        if s.rows:
            r=s.rows[-1]
            station='unknown' if r['station']==4 else str(r['station'])
            self.summary.set(f'Station {station} | active {r["active"]} | age {r["age"]} ms\n'
                f'CW {r["cw"]} / CCW {r["ccw"]} / returns {r["returns"]} / jumps {r["jumps"]}\n'
                f'Aborted {r["aborted"]} | reversals {r["reversals"]} | '
                f'route laps +{r["cw_laps"]} / -{r["ccw_laps"]}\n'
                f'overlaps {r["overlaps"]}, shortcuts {r["shortcuts"]}, '
                f'timeouts {r["timeouts"]}, gaps {r["gaps"]}, faults {r["fault"]}')
        else:self.summary.set('No station observations')
        self.table.delete(*self.table.get_children())
        for r in s.rows:
            self.table.insert('', 'end', values=[r[k] for k in self.table['columns']])

    def poll(self):
        if self.closed:return
        changed=False
        if self.port is not None and not self.session.blocked:
            try:
                data=self.port.read()
                if data:
                    for row in self.decoder.feed(data):self.session.accept(row)
                    changed=True
            except (ValueError,OSError,termios.error) as exc:
                self.session.warning=str(exc)
                self.disconnect();changed=True
        if changed:self.render()
        else:
            state='LIVE' if self.session.live() else 'NOT LIVE'
            self.status.set(f'{state} | source={self.session.source} | connected={self.session.connected} | '
                            f'blocked={self.session.blocked} | parser errors={self.decoder.errors} | '
                            f'{self.session.warning}')
        self.timer=self.root.after(20,self.poll)

    def close(self):
        self.closed=True
        self.root.after_cancel(self.timer)
        self.disconnect()
        self.root.destroy()

if __name__=='__main__':
    root=tk.Tk();Workbench(root);root.mainloop()
