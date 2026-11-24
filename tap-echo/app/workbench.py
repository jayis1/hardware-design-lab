#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Linux desktop, real receive-only serial, strict replay and local export."""
import argparse
import time
import termios
import tkinter as tk
from tkinter import ttk, filedialog, messagebox
from model import Session, RESULTS
from transport import Decoder, Serial, load_replay

class Workbench:
    def __init__(self, root):
        self.root = root
        root.title('Tap Echo — jayis1')
        self.serial = None
        self.decoder = Decoder()
        self.session = Session('disconnected')
        self.last_seen = 0
        self.last_sample = None
        self.after = None
        self.path = tk.StringVar(value='/dev/ttyACM0')
        self.status = tk.StringVar(value='DISCONNECTED — data is not live')
        self.summary = tk.StringVar(value='No reference or trial received')
        top = ttk.Frame(root); top.pack(fill='x')
        ttk.Entry(top, textvariable=self.path, width=32).pack(side='left')
        for label, callback in [('Connect', self.connect), ('Disconnect', self.disconnect),
                                ('Replay TE1', self.choose_replay), ('Export JSON', self.export)]:
            ttk.Button(top, text=label, command=callback).pack(side='left')
        ttk.Label(root, textvariable=self.status).pack(fill='x')
        self.tabs = ttk.Notebook(root); self.tabs.pack(fill='both', expand=True)
        capture = ttk.Frame(self.tabs); history = ttk.Frame(self.tabs); wiring = ttk.Frame(self.tabs)
        self.tabs.add(capture, text='Pattern comparison')
        self.tabs.add(history, text='Session diagnostics')
        self.tabs.add(wiring, text='Wiring and procedure')
        ttk.Label(capture, textvariable=self.summary, wraplength=760).pack(fill='x')
        self.table = ttk.Treeview(capture, columns=('slot','reference','reference_ms','trial','trial_ms','delta'),
                                  show='headings', height=16)
        for name in self.table['columns']:
            self.table.heading(name, text=name.replace('_',' '))
            self.table.column(name, width=110)
        self.table.pack(fill='both', expand=True)
        self.diagnostics = tk.Text(history, width=90, height=20, state='disabled')
        self.diagnostics.pack(fill='both', expand=True)
        ttk.Label(wiring, text=(
            'Author: jayis1\nUnbuilt prototype; not a medical or safety instrument.\n\n'
            'J1: 1=3V3 (CN6.4), 2=GND (CN6.6). Never apply external powered signals.\n'
            'J2: 1=PA0, 2=PA1, 3=PA4, 4=PB0; Nucleo CN8.1 through CN8.4.\n'
            'J3=A pad, J4=B pad, J5=RECORD mode, J6=GATE. Pin 2 is ground.\n\n'
            'Start with GATE open and both pads released. Close RECORD, then close GATE.\n'
            'Tap 2–16 isolated A/B presses, releasing each before the next. Open GATE.\n'
            'Open RECORD. Close GATE, repeat the rhythm, release pads, then open GATE.\n'
            'Only onset intervals and identity are compared; hold duration is ignored.\n'
            'A match is within max(30ms, 20% reference interval), not an ability score.\n'
            'Mode changes, raw A+B overlap, sampling gaps and 30s timeout abort a capture.\n'
            'Reset clears the reference. Data is volatile and no host commands are accepted.\n'
            'Serial CRC detects errors; it does not authenticate devices. Export is local only.'
        ), justify='left', wraplength=800).pack(padx=15, pady=15)
        root.protocol('WM_DELETE_WINDOW', self.close)
        self.schedule()
    def schedule(self):
        self.poll()
        self.after = self.root.after(50, self.schedule)
    def connect(self):
        self.disconnect()
        try:
            self.serial = Serial(self.path.get())
            self.session = Session('serial:' + self.path.get())
            self.decoder = Decoder(); self.last_seen = 0
            self.last_sample = None
            self.clear()
            self.status.set('WAITING — no valid frame received')
        except (OSError, ValueError, termios.error) as exc:
            self.status.set('CONNECT ERROR: ' + str(exc))
    def disconnect(self):
        if self.serial is not None:
            self.serial.close(); self.serial = None
            if self.decoder.buffer:
                self.decoder.errors += 1
                self.decoder.buffer = b''
        self.status.set('DISCONNECTED — retained data is historical')
    def clear(self):
        self.table.delete(*self.table.get_children())
        self.summary.set('No reference or trial received')
    def poll(self):
        if self.serial is None: return
        try:
            rows = self.decoder.feed(self.serial.read())
            for row in rows:
                self.session.accept(row)
                if row['samples'] != self.last_sample:
                    self.last_seen = time.monotonic()
                    self.last_sample = row['samples']
            if rows: self.render(rows[-1])
            if not self.last_seen:
                state = 'WAITING'
            elif time.monotonic() - self.last_seen > 2:
                state = 'STALE — last valid frame older than 2s'
            else:
                state = 'LIVE'
            self.status.set(f'{state}; framing errors={self.decoder.errors}')
        except (OSError, ValueError) as exc:
            self.disconnect()
            self.status.set('DISCONNECTED: ' + str(exc))
    def render(self, row):
        self.summary.set(
            f'{RESULTS[row["result"]]}; active={row["active"]}; RECORD={row["recording"]}; '
            f'max interval error={row["max_error"]}ms; differing slots={row["wrong"]}; '
            f'fault={row["fault"]}. Last result is retained until next capture; '
            'compare completed, non-aborted captures only.')
        self.table.delete(*self.table.get_children())
        for i in range(max(row['reference_count'], row['count'])):
            rp = row[f'reference_pad{i}']; tp = row[f'event_pad{i}']
            ri = row[f'reference_interval{i}']; ti = row[f'event_interval{i}']
            name = {0:'—', 1:'A', 2:'B'}
            self.table.insert('', 'end', values=(i+1, name[rp], ri if rp else '—',
                              name[tp], ti if tp else '—', ti-ri if rp and tp and i else '—'))
        self.diagnostics.configure(state='normal'); self.diagnostics.delete('1.0', 'end')
        self.diagnostics.insert('end', f'Source: {self.session.source}\nEpochs: {self.session.epochs}\n'
                                f'Retained snapshots: {len(self.session.rows)}\nDropped: {self.session.dropped}\n')
        for key in ('samples','missed','tx_errors','fault','references','trials','matches','aborted','timeouts'):
            self.diagnostics.insert('end', f'{key}: {row[key]}\n')
        self.diagnostics.configure(state='disabled')
    def replay(self, path):
        session = load_replay(path)
        self.disconnect(); self.session = session
        self.decoder = Decoder(); self.last_seen = 0; self.last_sample = None
        self.render(session.rows[-1]); self.status.set('REPLAY — synthetic or recorded, not live')
    def choose_replay(self):
        path = filedialog.askopenfilename(title='Open TE1 raw recording')
        if not path: return
        try: self.replay(path)
        except (OSError, ValueError) as exc: messagebox.showerror('Replay failed', str(exc))
    def export(self):
        path = filedialog.asksaveasfilename(defaultextension='.json', title='New local evidence file')
        if not path: return
        try:
            self.session.export(path, dict(status=self.status.get(),
                                connected=self.serial is not None,
                                framing_errors=self.decoder.errors,
                                seconds_since_progress=(time.monotonic()-self.last_seen
                                                        if self.last_seen else None),
                                warning='Receive-only stream; absolute freshness and boot identity unproven'))
        except (OSError, ValueError) as exc: messagebox.showerror('Export failed', str(exc))
    def close(self):
        self.disconnect()
        if self.after is not None:
            self.root.after_cancel(self.after); self.after = None
        self.root.destroy()

def main():
    parser = argparse.ArgumentParser(description='Tap Echo desktop by jayis1')
    parser.add_argument('--replay'); parser.add_argument('--summary', action='store_true')
    args = parser.parse_args()
    if args.summary:
        if not args.replay: parser.error('--summary requires --replay')
        session = load_replay(args.replay)
        row = session.rows[-1]
        print(f'REPLAY: {len(session.rows)} snapshots; {RESULTS[row["result"]]}; matches={row["matches"]}')
        return
    root = tk.Tk(); app = Workbench(root)
    if args.replay: app.replay(args.replay)
    root.mainloop()
if __name__ == '__main__': main()
