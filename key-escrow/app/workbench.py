#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Local Linux/Tk workbench; receive-only, no machine-control capability."""
from pathlib import Path
import termios
import tkinter as tk
from tkinter import ttk, filedialog, messagebox
from model import Session, STATES, RESULTS, FIELDS
from transport import Decoder, Serial, load_replay

class Workbench:
    def __init__(self, root):
        self.root = root
        self.root.title('Key Escrow — jayis1 — bench evidence only')
        self.root.geometry('1100x650')
        self.serial = None
        self.decoder = Decoder()
        self.session = Session('replay')
        self.timer = None
        self.summary = tk.StringVar(value='No evidence loaded')
        self.status = tk.StringVar(value='Disconnected')
        self.path = tk.StringVar(value='/dev/ttyACM0')
        bar = ttk.Frame(root); bar.pack(fill='x', padx=8, pady=8)
        ttk.Entry(bar, textvariable=self.path, width=30).pack(side='left')
        for text, command in [('Connect', lambda: self.connect(self.path.get())),
                              ('Disconnect', self.disconnect),
                              ('Replay KE1', self.choose_replay),
                              ('Export CSV', self.choose_export)]:
            ttk.Button(bar, text=text, command=lambda f=command: self.guarded(f)).pack(side='left', padx=3)
        ttk.Label(root, textvariable=self.status).pack(fill='x', padx=8)
        self.tabs = ttk.Notebook(root); self.tabs.pack(fill='both', expand=True, padx=8, pady=8)
        overview = ttk.Frame(self.tabs); self.tabs.add(overview, text='Sequence')
        ttk.Label(overview, textvariable=self.summary, font=('TkDefaultFont', 16), justify='left').pack(anchor='nw', padx=20, pady=20)
        ttk.Label(overview, text='WAIT → READY → insert key → retract bolt → open/close flap → extend bolt → remove key\n\n'
                  'HALT is terminal. Restore the detached fixture baseline and reset the observer.\n'
                  'No actuator, relay or safety output. A cycle is contact evidence, not a safe-machine certificate.',
                  justify='left').pack(anchor='nw', padx=20)
        history = ttk.Frame(self.tabs); self.tabs.add(history, text='Trace')
        columns = ('sequence', 'now', 'state', 'raw', 'qualified', 'opened', 'starts', 'completed', 'canceled', 'aborted', 'result')
        self.trace = ttk.Treeview(history, columns=columns, show='headings')
        for col in columns:
            self.trace.heading(col, text=col); self.trace.column(col, width=85, stretch=True)
        scroll = ttk.Scrollbar(history, command=self.trace.yview)
        self.trace.configure(yscrollcommand=scroll.set)
        self.trace.pack(side='left', fill='both', expand=True); scroll.pack(side='right', fill='y')
        setup = ttk.Frame(self.tabs); self.tabs.add(setup, text='Wiring and limits')
        ttk.Label(setup, justify='left', text='Author: jayis1\n\n'
                  'External NUCLEO-F401RE / 115200 8N1 / USB ST-LINK VCP\n'
                  'J1: 1=3V3 (CN6.4), 2=GND (CN6.6)\n'
                  'J2: PA0 / PA1 / PA4 / PB0 (CN8.1 through CN8.4)\n'
                  'J3: RETRACTED, J4: EXTENDED, J5: KEY_SEATED, J6: FLAP_CLOSED\n'
                  'Each sensor connector: pin 1 signal, pin 2 ground; dry contacts only.\n\n'
                  'Inputs qualify independently after five identical samples. Baseline: 50 ms.\n'
                  'Trial deadline: 60000 acquired ms, rejecting at equality.\n'
                  'KE1 CRC detects damage, not forgery. Local replay and exports are not authenticated.\n'
                  'Only the most recent 512 snapshots are retained by the companion.\n'
                  'Fresh requires advancing acquisition and connected transport, not just receipt.\n'
                  'Fixed ±2 s cumulative host/device tolerance may require long-session reconnects.\n'
                  'Do not attach to machinery, energized wiring or a real protective interlock.').pack(anchor='nw', padx=20, pady=20)
        root.protocol('WM_DELETE_WINDOW', self.close)
        self.tick()

    def guarded(self, action):
        try:
            action()
        except (OSError, ValueError, termios.error) as exc:
            messagebox.showerror('Key Escrow', str(exc), parent=self.root)

    def connect(self, path):
        if self.serial is not None:
            raise ValueError('Disconnect before opening a second serial connection')
        candidate = Serial(path)  # preserve old evidence if opening fails
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
            self.serial.close(); self.serial = None
        self.session.connected = False
        self.render()

    def replay(self, path):
        if self.serial is not None:
            raise ValueError('Disconnect before replacing live evidence')
        candidate = load_replay(path)
        self.session = candidate
        self.render()

    def export(self, path):
        self.session.export(path)

    def choose_replay(self):
        path = filedialog.askopenfilename(filetypes=[('KE1 recording', '*.ke1'), ('All files', '*')])
        if path: self.replay(path)

    def choose_export(self):
        path = filedialog.asksaveasfilename(defaultextension='.csv')
        if path: self.export(path)

    def render(self):
        s = self.session
        if s.rows:
            r = s.rows[-1]
            self.summary.set(f"{STATES[r['state']]} — {RESULTS[r['result']]}\n"
                             f"Cycles {r['completed']}   Returned {r['canceled']}   Aborted {r['aborted']}\n"
                             f"Trial age {r['age']} ms   Retained duration {r['last_ms']} ms\n"
                             f"Raw {r['raw']:04b}   Qualified {r['qualified']:04b}   Valid {r['valid']:04b}\n"
                             f"Flap opened this/latest trial: {bool(r['opened'])}")
        else:
            self.summary.set('No evidence loaded')
        self.status.set(f'{s.provenance} | ' + ('advancing acquisition' if s.fresh() else 'NOT LIVE') +
                        f' | connected={s.connected} | parser errors={s.errors} | {s.warning}')
        self.trace.delete(*self.trace.get_children())
        columns = self.trace['columns']
        for r in s.rows:
            self.trace.insert('', 'end', values=[r[x] for x in columns])

    def tick(self):
        if self.serial is not None:
            try:
                data = self.serial.read()
                if data:
                    rows = self.decoder.feed(data)
                    self.session.errors = self.decoder.errors
                    for row in rows:
                        self.session.accept(row)
            except (OSError, ValueError, termios.error) as exc:
                self.session.warning = str(exc)
                self.disconnect()
        self.render()
        self.timer = self.root.after(40, self.tick)

    def close(self):
        if self.timer is not None:
            self.root.after_cancel(self.timer); self.timer = None
        self.disconnect()
        self.root.destroy()

if __name__ == '__main__':
    root = tk.Tk()
    Workbench(root)
    root.mainloop()
