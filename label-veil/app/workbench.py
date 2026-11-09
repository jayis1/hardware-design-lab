#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Local Label Veil notebook. No cloud, participant identifiers or control writes."""
import csv
import os
import termios
import tkinter as tk
from tkinter import ttk, filedialog, messagebox
from model import FIELDS, Session
from transport import Decoder, Serial, load_replay

class Workbench:
    def __init__(self, root):
        self.root = root
        root.title('Label Veil — jayis1')
        root.geometry('900x580')
        self.port = None
        self.session = Session('none')
        self.decoder = Decoder()
        self.job = None
        self.path = tk.StringVar(value='/dev/ttyACM0')
        self.warning = tk.StringVar(value='Unbuilt prototype; synthetic replay is not participant evidence.')
        self.summary = tk.StringVar(value='No observations')
        self.matrix = tk.StringVar(value='No completed classifications')
        self.phase = tk.StringVar(value='Connect or replay a recording')
        self.tabs = ttk.Notebook(root)
        self.tabs.pack(fill='both', expand=True)
        capture, history, policy = [ttk.Frame(self.tabs, padding=12) for _ in range(3)]
        for page, name in zip((capture, history, policy), ('Classification','Notebook','Policy / Export')):
            self.tabs.add(page, text=name)
        controls = ttk.Frame(capture); controls.pack(fill='x')
        ttk.Entry(controls, textvariable=self.path, width=36).pack(side='left')
        ttk.Button(controls, text='Connect', command=lambda:self.action(self.connect,self.path.get())).pack(side='left')
        ttk.Button(controls, text='Disconnect', command=self.disconnect).pack(side='left')
        ttk.Button(controls, text='Replay', command=self.choose_replay).pack(side='left')
        ttk.Label(capture, textvariable=self.phase, font=('TkDefaultFont',16)).pack(pady=20)
        ttk.Label(capture, textvariable=self.matrix, font=('TkFixedFont',20)).pack(pady=18)
        ttk.Label(capture, textvariable=self.summary).pack(pady=12)
        ttk.Label(capture, text='Current encoded class is withheld until a clean removal.\nPrevious completed results remain visible.').pack()
        columns = ('time','state','completed','rejected','last')
        self.history = ttk.Treeview(history, columns=columns, show='headings', height=18)
        for col in columns:
            self.history.heading(col, text=col)
            self.history.column(col, width=140)
        scroll = ttk.Scrollbar(history, command=self.history.yview)
        self.history.configure(yscrollcommand=scroll.set)
        scroll.pack(side='right', fill='y'); self.history.pack(fill='both', expand=True)
        text = ('Author: jayis1\n\nUse dry, safe tactile samples and a concealed encoded dock. '
                'Teach the two class names first. Start empty with both paddles released. '
                'Insert one sample, touch it, press and release exactly one answer paddle, '
                'then remove the sample.\n\nThe matrix is true class (rows) versus declared class (columns). '
                'It records classifications, not disability, product certification or sensory thresholds. '
                'Timeout, changed identity, contradictory answers and missing acquisition reject trials.\n\n'
                'No participant identity is requested. Reboot clears device evidence; export before resetting. '
                'Replays are historical. A live timestamp cannot prove physical sensor validity. '
                'Some resets remain undetectable without a persistent boot identifier.\n\n'
                'Export retains up to 2000 recent snapshots plus provenance and errors, not all individual trials. '
                'A new connection starts a new local notebook. Save retained evidence before reconnecting.')
        ttk.Label(policy, text=text, wraplength=820, justify='left').pack(anchor='w')
        ttk.Button(policy, text='Export private CSV (no overwrite)', command=self.choose_export).pack(pady=20)
        ttk.Label(root, textvariable=self.warning, wraplength=880).pack(fill='x', padx=10, pady=8)
        root.protocol('WM_DELETE_WINDOW', self.close)
        self.job = root.after(25, self.poll)

    def action(self, function, *args):
        try:
            function(*args)
        except (OSError, ValueError, termios.error) as exc:
            self.warning.set(str(exc))
            messagebox.showerror('Label Veil', str(exc))

    def connect(self, path):
        if self.port is not None:
            raise ValueError('Disconnect the owned serial port first')
        port = Serial(path)  # Failure leaves existing evidence intact.
        self.port = port
        self.decoder = Decoder()
        self.session = Session('live')
        self.session.connected = True
        self.warning.set('Awaiting advancing LV1 observations; new local session')
        self.render()

    def disconnect(self):
        if self.port is not None:
            self.port.close(); self.port = None
        self.session.connected = False
        self.warning.set('Disconnected; retained evidence is not live')
        self.render()

    def replay(self, path):
        if self.port is not None:
            raise ValueError('Disconnect before replay')
        candidate = load_replay(path)
        self.session = candidate
        self.decoder = Decoder()
        self.warning.set('Historical replay — not live or physically verified')
        self.render()

    def choose_replay(self):
        path = filedialog.askopenfilename()
        if path:
            self.action(self.replay, path)

    def choose_export(self):
        path = filedialog.asksaveasfilename(defaultextension='.csv')
        if path:
            self.action(self.export, path)

    def poll(self):
        if self.port is not None:
            try:
                data = self.port.read()
                if data:
                    before = self.decoder.errors
                    fragments = self.decoder.initial_fragments
                    rows = self.decoder.feed(data)
                    if self.decoder.errors-before != self.decoder.initial_fragments-fragments:
                        raise ValueError('Framing/corruption detected: reconnect required')
                    for row in rows:
                        self.session.accept(row)
                    self.render()
            except (OSError, ValueError, termios.error) as exc:
                self.session.error = str(exc)
                self.disconnect()
                self.warning.set(str(exc))
        if self.session.connected:
            self.warning.set('Live advancing acquisition' if self.session.fresh() else
                             'Waiting / stale / halted: do not treat this as fresh acquisition')
        self.job = self.root.after(25, self.poll)

    def render(self):
        for item in self.history.get_children():
            self.history.delete(item)
        if not self.session.rows:
            self.matrix.set('No completed classifications')
            self.summary.set('No observations')
            self.phase.set('Awaiting data')
            return
        r = self.session.rows[-1]
        self.phase.set(('WAIT: empty dock and release paddles', 'READY: insert sample',
                        'ACTIVE: class withheld — declare, release, remove',
                        'HALT: reset only after exporting evidence')[r['state']])
        self.matrix.set(f"True A:  A {r['aa']}   B {r['ab']}\nTrue B:  A {r['ba']}   B {r['bb']}")
        self.summary.set(f"{r['starts']} presentations; {r['completed']} classified; {r['rejected']} rejected; "
                         f"latest accepted duration {r['last_ms']} ms; reason {r['reason']}")
        for row in self.session.rows:
            last = 'none' if not row['completed'] else f"{'AB'[row['last_truth']-1]} / {'AB'[row['last_answer']-1]}"
            self.history.insert('', 'end', values=(row['now'], row['state'],row['completed'],row['rejected'],last))

    def export(self, path):
        fd = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
        with os.fdopen(fd, 'w', newline='') as handle:
            out = csv.writer(handle)
            # No user-authored text fields; prefix error text against spreadsheet formulas.
            out.writerows([['author','jayis1'], ['origin',self.session.origin],
                           ['connected',int(self.session.connected)], ['fresh',int(self.session.fresh())],
                           ['parser_errors',self.decoder.errors], ['partial_bytes',len(self.decoder.buffer)],
                           ['error', "'"+self.session.error], ['warning', "'"+self.warning.get()]])
            out.writerow(FIELDS)
            for row in self.session.rows:
                out.writerow([row[f] for f in FIELDS])

    def close(self):
        if self.job is not None:
            self.root.after_cancel(self.job); self.job = None
        if self.port is not None:
            self.port.close(); self.port = None
        self.root.destroy()

if __name__ == '__main__':
    window = tk.Tk()
    Workbench(window)
    window.mainloop()
