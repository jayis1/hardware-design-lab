#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Local Linux/Tk notebook. No transmit commands or actuator control."""
import termios
import tkinter as tk
from tkinter import ttk, filedialog, messagebox
from model import Session, STATES
from transport import Decoder, Serial, load_replay

class Workbench:
    def __init__(self, root):
        self.root = root
        root.title('Pallet Census — jayis1 — electrical evidence only')
        root.geometry('1120x650')
        self.serial = None
        self.decoder = Decoder()
        self.session = Session('replay')
        self.timer = None
        self.summary = tk.StringVar(value='No evidence loaded')
        self.status = tk.StringVar(value='Disconnected')
        self.path = tk.StringVar(value='/dev/ttyACM0')
        bar = ttk.Frame(root); bar.pack(fill='x', padx=8, pady=8)
        ttk.Entry(bar, textvariable=self.path, width=30).pack(side='left')
        for label, action in [('Connect', lambda: self.connect(self.path.get())),
                              ('Disconnect', self.disconnect), ('Replay PC1', self.choose_replay),
                              ('Export CSV', self.choose_export)]:
            ttk.Button(bar, text=label, command=lambda f=action: self.guarded(f)).pack(side='left', padx=3)
        ttk.Label(root, textvariable=self.status).pack(fill='x', padx=8)
        self.tabs = ttk.Notebook(root); self.tabs.pack(fill='both', expand=True, padx=8)
        live = ttk.Frame(self.tabs); self.tabs.add(live, text='Pallet evidence')
        ttk.Label(live, textvariable=self.summary, font=('TkDefaultFont', 16), justify='left').pack(anchor='nw', padx=20, pady=20)
        ttk.Label(live, justify='left', text='Release RUN and INDEX → wait for READY → hold RUN → move slowly → stop INDEX low → release RUN.\n\n'
                  'A/B label pallet contact positions, not measured engagement or torque.\n'
                  'AA and BB are repeated-side arrival pairs; AB and BA are alternating pairs.\n'
                  'Neither, both and uncertain arrivals break the adjacency chain.\n'
                  'HALT is terminal: export evidence, restore the fixture, then reset the observer.').pack(anchor='nw', padx=20)
        history = ttk.Frame(self.tabs); self.tabs.add(history, text='Trace')
        columns = 'sequence now state raw qualified starts completed aborted age pulses a b uncertain'.split()
        self.trace = ttk.Treeview(history, columns=tuple(columns), show='headings')
        for col in columns:
            self.trace.heading(col, text=col); self.trace.column(col, width=78)
        scroll = ttk.Scrollbar(history, command=self.trace.yview)
        self.trace.configure(yscrollcommand=scroll.set)
        self.trace.pack(side='left', fill='both', expand=True); scroll.pack(side='right', fill='y')
        setup = ttk.Frame(self.tabs); self.tabs.add(setup, text='Wiring and limits')
        ttk.Label(setup, justify='left', text='Author: jayis1\n\n'
                  'NUCLEO-F401RE / USB ST-LINK VCP / 115200 8N1\n'
                  'J1: 1=3V3 from CN6.4, 2=GND from CN6.6\n'
                  'J2: PA0 / PA1 / PA4 / PB0 from CN8.1 through CN8.4\n'
                  'J3=PALLET_A, J4=PALLET_B, J5=INDEX, J6=RUN\n'
                  'Sensor connectors: pin 1 signal, pin 2 ground; voltage-free contacts only.\n\n'
                  'Five samples qualify each channel independently. Baseline: fifty intervals.\n'
                  'Deadline: 60000 acquired milliseconds, rejecting at equality.\n'
                  'Snapshots are not a complete event log. Latest 512 snapshots retained locally.\n'
                  'PC1 CRC checks corruption, not authenticity. Export before starting another run.\n'
                  'Fixed two-second clock-drift budget can require reconnecting on long sessions.\n'
                  'Unbuilt prototype. Not for powered machinery, timekeeping calibration or safety testing.').pack(anchor='nw', padx=20, pady=20)
        root.protocol('WM_DELETE_WINDOW', self.close)
        self.tick()

    def guarded(self, action):
        try:
            action()
        except (OSError, ValueError, termios.error) as exc:
            messagebox.showerror('Pallet Census', str(exc), parent=self.root)

    def connect(self, path):
        if self.serial is not None:
            raise ValueError('Disconnect before opening another serial connection')
        candidate = Serial(path)
        self.serial = candidate
        self.decoder = Decoder()
        self.session = Session('live')
        self.session.connected = True
        self.render()

    def disconnect(self):
        if self.serial is not None:
            if self.decoder.buffer:
                self.decoder.errors += 1
                self.decoder.buffer = b''
            self.session.errors = self.decoder.errors
            self.serial.close()
            self.serial = None
        self.session.connected = False
        self.render()

    def replay(self, path):
        if self.serial is not None:
            raise ValueError('Disconnect before replacing live evidence')
        self.session = load_replay(path)
        self.render()

    def export(self, path):
        self.session.export(path)

    def choose_replay(self):
        path = filedialog.askopenfilename(filetypes=[('PC1 recording', '*.pc1'), ('All files', '*')])
        if path: self.replay(path)

    def choose_export(self):
        path = filedialog.asksaveasfilename(defaultextension='.csv')
        if path: self.export(path)

    def render(self):
        s = self.session
        if s.rows:
            r = s.rows[-1]
            self.summary.set(f"{STATES[r['state']]}   fault={r['fault']}   age={r['age']} ms\n"
                             f"Runs started {r['starts']} / completed {r['completed']} / aborted {r['aborted']}\n"
                             f"Arrivals {r['pulses']}   A {r['a']}   B {r['b']}\n"
                             f"Neither {r['neither']}   Both {r['both']}   Uncertain {r['uncertain']}\n"
                             f"AA {r['aa']}   AB {r['ab']}   BA {r['ba']}   BB {r['bb']}\n"
                             f"Raw {r['raw']:04b}   Qualified {r['qualified']:04b}   Valid {r['valid']:04b}")
        else:
            self.summary.set('No evidence loaded')
        self.status.set(f'{s.provenance} | ' + ('advancing acquisition' if s.fresh() else 'NOT LIVE') +
                        f' | connected={s.connected} | parser errors={s.errors} | {s.warning}')
        self.trace.delete(*self.trace.get_children())
        for r in s.rows:
            self.trace.insert('', 'end', values=[r[x] for x in self.trace['columns']])

    def tick(self):
        if self.serial is not None:
            try:
                data = self.serial.read()
                if data:
                    rows = self.decoder.feed(data)
                    self.session.errors = self.decoder.errors
                    for row in rows: self.session.accept(row)
            except (OSError, ValueError, termios.error) as exc:
                self.session.warning = str(exc)
                self.disconnect()
        self.render()
        self.timer = self.root.after(40, self.tick)

    def close(self):
        if self.timer is not None:
            self.root.after_cancel(self.timer)
            self.timer = None
        self.disconnect()
        self.root.destroy()

if __name__ == '__main__':
    root = tk.Tk()
    Workbench(root)
    root.mainloop()
