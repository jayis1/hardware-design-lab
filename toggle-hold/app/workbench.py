#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Receive-only Linux/Tk bench application for Toggle Hold."""
import tkinter as tk
from tkinter import ttk, filedialog, messagebox
import termios
from model import Session, STATES, RESULTS, MASK
from transport import Decoder, Serial, load_replay


class Workbench:
    def __init__(self, root):
        self.root = root
        self.port = None
        self.session = Session('none')
        self.decoder = Decoder()
        self.closed = False
        root.title('Toggle Hold — jayis1 — unbuilt prototype')
        root.geometry('1100x650')
        self.tabs = ttk.Notebook(root)
        self.tabs.pack(fill='both', expand=True)
        live, history, setup = [ttk.Frame(self.tabs) for _ in range(3)]
        for frame, title in zip((live, history, setup), ('Connect / Trial', 'History / Export', 'Setup / Limits')):
            self.tabs.add(frame, text=title)
        self.path = tk.StringVar(value='/dev/ttyACM0')
        ttk.Entry(live, textvariable=self.path, width=50).pack(pady=10)
        buttons = ttk.Frame(live); buttons.pack()
        for label, action in [('Connect', lambda: self.connect(self.path.get())),
                              ('Disconnect', self.disconnect), ('Replay', self.choose_replay)]:
            ttk.Button(buttons, text=label, command=lambda f=action: self.guard(f)).pack(side='left')
        self.status = tk.StringVar(value='NOT LIVE — no session')
        self.summary = tk.StringVar(value='No trial evidence')
        ttk.Label(live, textvariable=self.status, wraplength=1000).pack(pady=20)
        ttk.Label(live, textvariable=self.summary, wraplength=1000,
                  font=('TkDefaultFont', 14)).pack(pady=20)
        ttk.Label(live, text='Pressure witnesses are not force sensors. Retained means 500 observed ms, not permanent stability.\n'
                  'Export, disconnect, reset the Nucleo and reconnect for the next trial.').pack()
        columns = ('now', 'state', 'origin', 'age', 'hold', 'reached_ms', 'released_ms', 'result', 'fault')
        self.table = ttk.Treeview(history, columns=columns, show='headings', height=20)
        for c in columns:
            self.table.heading(c, text=c); self.table.column(c, width=100)
        self.table.pack(fill='both', expand=True)
        ttk.Button(history, text='Export private CSV (new file)',
                   command=lambda: self.guard(self.choose_export)).pack(pady=10)
        text = tk.Text(setup, wrap='word', padx=20, pady=20)
        text.pack(fill='both', expand=True)
        text.insert('1.0', 'Author and creator: jayis1\n\n'
                    'Mount an unpowered, low-energy bistable specimen between two adjustable endpoint switches. '
                    'J3 is END_A; J4 END_B. Two independent spring-return hand plungers actuate pressure '
                    'witnesses J5 PUSH_B and J6 PUSH_A. Neither contact measures actual force.\n\n'
                    'J1: pin 1 regulated 3V3 from Nucleo CN6.4; pin 2 ground CN6.6. '
                    'J2 pins 1–4: CN8.1–4 (PA0, PA1, PA4, PB0). Each sensor header pin 1 is signal, '
                    'pin 2 ground. Voltage-free low-level contacts only; no powered signals or mains.\n\n'
                    'First leave the specimen at either endpoint with both plungers released for 50 observed ms. '
                    'READY establishes that origin. Press the plunger toward the opposite endpoint before moving '
                    'the specimen. Hold each intended indication at least 6 ms, preferably 100 ms during bring-up. '
                    'Reach the opposite endpoint while pressure is still indicated. Release the plunger only after '
                    'LOADED. HOLD begins after qualified release. Any raw endpoint loss or renewed pressure during '
                    'the following 500 ms rejects retention. Ten seconds from start ends the trial as timeout.\n\n'
                    'One trial per reset prevents automatic reuse of a failed specimen position. DONE and HALT '
                    'freeze evidence. Heartbeats continue but are not new acquisition. Disconnect preserves history; '
                    'successful reconnect starts a new host session. Export before replacing evidence.\n\n'
                    'Replay is never live. The 2000-row history is a bounded snapshot window, not an edge log. '
                    'CRC detects corruption, not forgery. Acquisition gaps halt interpretation; noisy switches can '
                    'reject a mechanically stable specimen. No persistent boot identity exists. Absolute acquisition '
                    'age, physical fabrication, force, contact calibration and long-term stability are unverified.')
        text.configure(state='disabled')
        root.protocol('WM_DELETE_WINDOW', self.close)
        self.timer = root.after(20, self.poll)

    def guard(self, action):
        try:
            action()
        except (ValueError, OSError, termios.error) as exc:
            messagebox.showerror('Toggle Hold', str(exc), parent=self.root)

    def connect(self, path):
        if self.port is not None:
            raise ValueError('Disconnect the owned serial port first')
        candidate = Serial(path)
        self.port = candidate
        self.session = Session('live'); self.session.connected = True
        self.decoder = Decoder(); self.render()

    def disconnect(self):
        if self.port is not None:
            self.port.close(); self.port = None
        self.session.connected = False
        self.render()

    def replay(self, path):
        if self.port is not None:
            raise ValueError('Disconnect before replacing evidence with replay')
        candidate = load_replay(path)
        self.session = candidate; self.decoder = Decoder(); self.render()

    def choose_replay(self):
        path = filedialog.askopenfilename(parent=self.root)
        if path:
            self.replay(path)

    def choose_export(self):
        path = filedialog.asksaveasfilename(parent=self.root, defaultextension='.csv')
        if path:
            self.session.export(path, self.decoder.errors)

    def render(self):
        s = self.session
        self.status.set(f'{"LIVE" if s.live() else "NOT LIVE / retained"} | source={s.source} | '
                        f'connected={s.connected} | blocked={s.blocked} | '
                        f'parser errors={self.decoder.errors} | {s.warning}')
        if s.rows:
            r = s.rows[-1]
            reach = 'absent' if r['reached_ms'] == MASK else f'{r["reached_ms"]} ms'
            release = 'absent' if r['released_ms'] == MASK else f'{r["released_ms"]} ms'
            origin = {0: 'unassigned', 1: 'A → B', 2: 'B → A'}[r['origin']]
            self.summary.set(f'{STATES[r["state"]]} | {origin} | {RESULTS[r["result"]]}\n'
                             f'Trial age {r["age"]} ms | unloaded hold {r["hold"]} / 500 ms\n'
                             f'Arrival {reach} | release {release}\n'
                             f'Faults {r["fault"]}, gaps {r["gaps"]}, TX errors {r["tx_errors"]}')
        else:
            self.summary.set('No trial evidence')
        self.table.delete(*self.table.get_children())
        for r in s.rows:
            self.table.insert('', 'end', values=[r[k] for k in self.table['columns']])

    def poll(self):
        if self.closed:
            return
        if self.port is not None and not self.session.blocked:
            try:
                data = self.port.read()
                if data:
                    for row in self.decoder.feed(data):
                        self.session.accept(row)
            except (ValueError, OSError, termios.error) as exc:
                self.session.warning = str(exc)
                self.disconnect()
        self.render()
        self.timer = self.root.after(20, self.poll)

    def close(self):
        self.closed = True
        self.root.after_cancel(self.timer)
        self.disconnect()
        self.root.destroy()


if __name__ == '__main__':
    root = tk.Tk(); Workbench(root); root.mainloop()
