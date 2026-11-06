#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Stroke Weave Linux/Tk evidence notebook; no device command channel."""
import termios
import tkinter as tk
from tkinter import ttk, filedialog, messagebox
from model import Session, STATES, PHASES
from transport import Decoder, Serial, load_replay

class Workbench:
    def __init__(self, root):
        self.root = root
        root.title('Stroke Weave — jayis1 — endpoint evidence, not flow')
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
                              ('Disconnect', self.disconnect), ('Replay TW1', self.choose_replay),
                              ('Export CSV', self.choose_export)]:
            ttk.Button(bar, text=label, command=lambda f=action: self.guarded(f)).pack(side='left', padx=3)
        ttk.Label(root, textvariable=self.status).pack(fill='x', padx=8)
        self.tabs = ttk.Notebook(root); self.tabs.pack(fill='both', expand=True, padx=8)
        live = ttk.Frame(self.tabs); self.tabs.add(live, text='Stroke evidence')
        ttk.Label(live, textvariable=self.summary, font=('TkDefaultFont', 15), justify='left').pack(anchor='nw', padx=20, pady=20)
        ttk.Label(live, justify='left', text='Visit each cylinder endpoint to establish an origin; then move slowly through a clear middle region.\n\n'
                  'HOME → clear → END is a delivery-direction stroke; END → clear → HOME is a refill.\n'
                  'Return to the origin is a canceled excursion, not a delivered quantity.\n'
                  'Tied deliveries clear the ordering anchor; no artificial A-before-B order.\n'
                  'HALT is terminal: export, inspect the fixture, then reset the observer.').pack(anchor='nw', padx=20)
        history = ttk.Frame(self.tabs); self.tabs.add(history, text='Trace')
        columns = 'sequence now state raw qualified a_phase b_phase a_deliveries b_deliveries alternating repeated ties'.split()
        self.trace = ttk.Treeview(history, columns=tuple(columns), show='headings')
        for col in columns:
            self.trace.heading(col, text=col); self.trace.column(col, width=85)
        scroll = ttk.Scrollbar(history, command=self.trace.yview)
        self.trace.configure(yscrollcommand=scroll.set)
        self.trace.pack(side='left', fill='both', expand=True); scroll.pack(side='right', fill='y')
        setup = ttk.Frame(self.tabs); self.tabs.add(setup, text='Wiring and limits')
        ttk.Label(setup, justify='left', text='Author: jayis1\n\n'
                  'NUCLEO-F401RE / USB ST-LINK VCP / 115200 8N1\n'
                  'J1: 1=3V3 from CN6.4, 2=GND from CN6.6\n'
                  'J2: PA0 / PA1 / PA4 / PB0 from CN8.1 through CN8.4\n'
                  'J3=A_HOME, J4=A_END, J5=B_HOME, J6=B_END\n'
                  'Sensor connectors: pin 1 signal, pin 2 ground; voltage-free contacts only.\n\n'
                  'Five consecutive samples qualify each channel independently.\n'
                  'An excursion reaching 60000 acquired milliseconds is rejected.\n'
                  'Snapshots are not an event log. Latest 512 snapshots retained locally.\n'
                  'TW1 CRC detects corruption, not forgery. Export before resetting.\n'
                  'Fixed two-second host/device drift budget can require reconnecting.\n'
                  'Unbuilt prototype. No pressure, volume, flow, safety or medical claims.').pack(anchor='nw', padx=20, pady=20)
        root.protocol('WM_DELETE_WINDOW', self.close)
        self.tick()

    def guarded(self, action):
        try:
            action()
        except (OSError, ValueError, termios.error) as exc:
            messagebox.showerror('Stroke Weave', str(exc), parent=self.root)

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
        path = filedialog.askopenfilename(filetypes=[('TW1 recording', '*.sw1'), ('All files', '*')])
        if path: self.replay(path)

    def choose_export(self):
        path = filedialog.asksaveasfilename(defaultextension='.csv')
        if path: self.export(path)

    def render(self):
        s = self.session
        if s.rows:
            r = s.rows[-1]
            text = [f"{STATES[r['state']]}   fault={r['fault']}   raw={r['raw']:04b} qualified={r['qualified']:04b}"]
            for side in ('a','b'):
                text.append(f"{side.upper()} deliveries {r[side+'_deliveries']} / refills {r[side+'_refills']} / canceled {r[side+'_canceled']} / abandoned {r[side+'_abandoned']}")
                text.append(f"  {PHASES[r[side+'_phase']]}   active {r[side+'_age']} ms / last full stroke {r[side+'_last_ms']} ms")
            text.append(f"Alternating pairs {r['alternating']} / repeated pairs {r['repeated']} / tied endpoints {r['ties']}")
            text.append(f"Current delivery streak {r['streak']} / longest {r['peak']}")
            self.summary.set('\n'.join(text))
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
