#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Case Delta Linux desktop companion. Serial input is receive-only."""
import argparse
import termios
import time
import tkinter as tk
from tkinter import filedialog, messagebox, ttk
from model import Session, PHASES, pocket_names
from transport import Decoder, Serial, load_replay

class Workbench:
    def __init__(self, root):
        self.root = root
        self.root.title('Case Delta — jayis1')
        self.serial = None
        self.decoder = Decoder()
        self.session = Session('none')
        self.running = True
        self.port = tk.StringVar(value='/dev/ttyACM0')
        self.state = tk.StringVar(value='Disconnected')
        self.summary = tk.StringVar(value='No snapshots')
        self.result = tk.StringVar(value='No completed opening session')
        toolbar = ttk.Frame(root, padding=8)
        toolbar.pack(fill='x')
        ttk.Label(toolbar, text='Serial device').pack(side='left')
        ttk.Entry(toolbar, textvariable=self.port, width=24).pack(side='left')
        for label, action in [('Connect', lambda: self.connect(self.port.get())),
                              ('Disconnect', self.disconnect), ('Replay', self.choose_replay),
                              ('Export CSV', self.choose_export)]:
            ttk.Button(toolbar, text=label, command=lambda f=action: self.guarded(f)).pack(side='left')
        ttk.Label(root, textvariable=self.state, wraplength=900).pack(fill='x', padx=8)
        self.tabs = ttk.Notebook(root)
        self.tabs.pack(fill='both', expand=True, padx=8, pady=8)
        dashboard = ttk.Frame(self.tabs, padding=12)
        history_page = ttk.Frame(self.tabs, padding=8)
        setup = ttk.Frame(self.tabs, padding=12)
        for page, title in [(dashboard, 'Inventory delta'), (history_page, 'Snapshots'), (setup, 'Setup and limits')]:
            self.tabs.add(page, text=title)
        ttk.Label(dashboard, textvariable=self.summary, wraplength=850).pack(anchor='w', pady=10)
        ttk.Label(dashboard, textvariable=self.result, wraplength=850, justify='left').pack(anchor='w', pady=10)
        ttk.Label(dashboard, text='Presence contacts do not establish tool identity or safe case contents.\n'
                  'Returned means a previously present pocket became qualified absent and present again.\n'
                  'Only the latest completed result is retained on the device.',
                  wraplength=850, justify='left').pack(anchor='w', pady=10)
        columns = ('tick', 'state', 'completed', 'before', 'after', 'touched', 'returned')
        self.history = ttk.Treeview(history_page, columns=columns, show='headings', height=16)
        for name in columns:
            self.history.heading(name, text=name.title())
            self.history.column(name, width=115, anchor='center')
        self.history.pack(side='left', fill='both', expand=True)
        scrollbar = ttk.Scrollbar(history_page, orient='vertical', command=self.history.yview)
        scrollbar.pack(side='right', fill='y')
        self.history.configure(yscrollcommand=scrollbar.set)
        instructions = (
            'Author and creator: jayis1\n\n'
            '1. Use disconnected, voltage-free contacts only. J1 is 3.3 V, never 5 V.\n'
            '2. J3/J4/J5 are pocket A/B/C. J6 closes only when the lid is fully shut.\n'
            '3. Close the case without moving pockets; allow at least 105 nominal ms for a reference.\n'
            '4. Open the lid, remove or replace tools, then close and allow 105 ms to settle.\n'
            '5. Compare missing, added and emptied/refilled pockets. Inspect actual tools separately.\n\n'
            'A raw pocket change while sealed halts correlation. Acquisition gaps also halt.\n'
            'A lid glitch creates an aborted session, not a completed opening. Maximum opening: 10 minutes.\n'
            'Reset the MCU and start a new capture after a HALT; reconnect alone cannot fix the device.\n'
            'LIVE progress is relative serial progress, not proof of absolute acquisition freshness.\n'
            'Replay is never live. Data is volatile; export before replacing a session.\n'
            'CSV has metadata and up to 2,000 snapshots, not every physical opening.\n'
            'No cloud, radio, control output, identity reader, alarm guarantee or protective function.'
        )
        ttk.Label(setup, text=instructions, justify='left', wraplength=850).pack(anchor='w')
        root.protocol('WM_DELETE_WINDOW', self.close)
        self.timer = root.after(25, self.poll)

    def guarded(self, action):
        try:
            action()
        except (ValueError, OSError, termios.error) as exc:
            messagebox.showerror('Case Delta', str(exc), parent=self.root)

    def connect(self, path):
        if self.serial is not None:
            raise ValueError('disconnect the owned port before connecting again')
        serial = Serial(path)  # failed setup leaves existing session intact
        self.serial = serial
        self.session = Session('live')
        self.decoder = Decoder()
        self.render()

    def disconnect(self):
        if self.serial is not None:
            if self.decoder.buffer:
                self.decoder.errors += 1
                self.decoder.buffer = b''
            self.session.parser_errors = self.decoder.errors
            self.serial.close()
            self.serial = None
        self.session.connected = False
        self.render()

    def replay(self, path):
        replacement = load_replay(path)  # validate before discarding old evidence
        self.disconnect()
        self.session = replacement
        self.render()

    def export(self, path):
        self.session.export(path)

    def choose_replay(self):
        path = filedialog.askopenfilename(parent=self.root, title='Open CD1 recording')
        if path:
            self.replay(path)

    def choose_export(self):
        path = filedialog.asksaveasfilename(parent=self.root, title='New CSV file (no overwrite)',
                                           defaultextension='.csv')
        if path:
            self.export(path)

    def poll(self):
        if not self.running:
            return
        changed = False
        if self.serial is not None:
            try:
                rows = self.decoder.feed(self.serial.read())
                self.session.parser_errors = self.decoder.errors
                for row in rows:
                    self.session.accept(row)
                    changed = True
            except (ValueError, OSError, termios.error) as exc:
                self.session.warning = str(exc)
                self.disconnect()
                changed = True
        if changed:
            self.render()
        else:
            self.render_status()
        self.timer = self.root.after(25, self.poll)

    def render_status(self):
        fresh = self.session.fresh()
        phase = PHASES[self.session.rows[-1]['phase']] if self.session.rows else 'Waiting'
        transport = 'connected' if self.session.connected else 'disconnected'
        label = 'LIVE progress' if fresh else 'not live'
        self.state.set(f'{self.session.provenance} / {transport} / {label} / {phase} / '
                       f'parser errors: {self.session.parser_errors} {self.session.warning}')

    def render(self):
        self.render_status()
        children = self.history.get_children()
        if children:
            self.history.delete(*children)
        for row in self.session.rows:
            self.history.insert('', 'end', values=(row['now'], PHASES[row['phase']], row['completed'],
                                row['last_before'], row['last_after'], row['last_touched'], row['last_returned']))
        if not self.session.rows:
            self.summary.set('No snapshots')
            self.result.set('No completed opening session')
            return
        row = self.session.rows[-1]
        self.summary.set(f"Started: {row['starts']}   Completed: {row['completed']}   Aborted: {row['aborted']}\n"
                         f"Reference pockets: {pocket_names(row['reference'])}   Session age: {row['age']} ms\n"
                         f"Fault mask: {row['fault']}   Acquisition gaps: {row['gaps']}   UART errors: {row['tx_errors']}")
        if row['completed']:
            missing = row['last_before'] & ~row['last_after']
            added = row['last_after'] & ~row['last_before']
            self.result.set(f"Latest completed session #{row['completed']} ({row['last_age']} nominal ms)\n"
                            f"Missing since opening: {pocket_names(missing)}\n"
                            f"Added since opening: {pocket_names(added)}\n"
                            f"Qualified occupancy changed: {pocket_names(row['last_touched'])}\n"
                            f"Emptied and refilled: {pocket_names(row['last_returned'])}")
        else:
            self.result.set('No completed opening session')

    def close(self):
        if not self.running:
            return
        self.running = False
        self.root.after_cancel(self.timer)
        self.disconnect()
        self.root.destroy()

def main():
    parser = argparse.ArgumentParser(description='Case Delta companion — jayis1')
    parser.add_argument('--replay', help='load a recorded CD1 stream (never live)')
    args = parser.parse_args()
    root = tk.Tk()
    ui = Workbench(root)
    if args.replay:
        ui.guarded(lambda: ui.replay(args.replay))
    root.mainloop()

if __name__ == '__main__':
    main()
