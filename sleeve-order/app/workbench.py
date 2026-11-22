#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Local, receive-only Sleeve Order workbench. Linux/POSIX; no packages."""
import tkinter as tk
from tkinter import ttk, filedialog, messagebox
import termios
from model import Session, PHASES, RESULTS
from transport import Decoder, Serial, load_replay

SETUP = '''Sleeve Order / Author: jayis1

UNBUILT PROTOTYPE — detached, unloaded tabletop mechanism only.
This is an observation notebook, never a protective interlock.

Connect NUCLEO-F401RE CN6.4 (3V3) to J1.1 and CN6.6 to J1.2.
J2.1..4 -> CN8.1..4 (PA0, PA1, PA4, PB0).
J3 outer home; J4 outer far; J5 inner home; J6 inner far.
Each switch closes its connector pin 1 to pin 2 (GND).
Only disconnected voltage-free switches. No powered sensor outputs.

Configuration is fixed in firmware: 1 ms acquisition, five unchanged
intervals for full-vector qualification, rejecting deadline 60000 ms.
The app does not transmit configuration or clear the MCU counters.

Wait for READY at both home stops (mask 5). Extend outer fully with
inner home: 5 -> 4 -> 6. Optional inner trips: 6 -> 2 -> 10 -> 2 -> 6.
Retract outer with inner home: 6 -> 4 -> 5. Pause at every region.
Never infer a missed endpoint. A return before far is a rejected cycle.

Connect starts a new host session only after serial setup succeeds.
Disconnect preserves observations. Reset MCU and reconnect for a new
experiment; the protocol has no durable boot identity. Replay is synthetic
or recorded evidence, never live. Export refuses existing filenames.

Counts describe contacts, not force, friction, retention or compliance.
Broken wires, shorts and moving switch brackets can invalidate meaning.
See README for electrical limits, fixture setup and physical release gates.
'''

class Workbench:
    def __init__(self, root):
        self.root = root
        self.root.title('Sleeve Order — jayis1')
        self.root.geometry('1040x700')
        self.serial = None
        self.decoder = Decoder()
        self.session = Session('none')
        self.timer = None
        self.port = tk.StringVar(value='/dev/ttyACM0')
        self.status = tk.StringVar()
        self.summary = tk.StringVar()
        self.notice = ''
        bar = ttk.Frame(root, padding=8)
        bar.pack(fill='x')
        ttk.Label(bar, text='Serial port').pack(side='left')
        ttk.Entry(bar, textvariable=self.port, width=25).pack(side='left')
        for name, callback in [('Connect', self.connect), ('Disconnect', self.disconnect),
                               ('Replay', self.replay), ('Export CSV', self.export)]:
            ttk.Button(bar, text=name, command=callback).pack(side='left', padx=4)
        ttk.Label(root, textvariable=self.status, padding=8).pack(fill='x')
        self.tabs = ttk.Notebook(root)
        self.tabs.pack(fill='both', expand=True)
        status = ttk.Frame(self.tabs, padding=12)
        self.tabs.add(status, text='Trajectory and outcomes')
        ttk.Label(status, textvariable=self.summary, justify='left',
                  font=('TkFixedFont', 13)).pack(anchor='nw')
        history = ttk.Frame(self.tabs)
        self.tabs.add(history, text='Retained snapshots')
        columns = ('tick', 'phase', 'mask', 'complete', 'nested', 'abort', 'last')
        self.history = ttk.Treeview(history, columns=columns, show='headings')
        for column in columns:
            self.history.heading(column, text=column)
            self.history.column(column, width=110)
        scroll = ttk.Scrollbar(history, orient='vertical', command=self.history.yview)
        self.history.configure(yscrollcommand=scroll.set)
        scroll.pack(side='right', fill='y')
        self.history.pack(fill='both', expand=True)
        setup = tk.Text(self.tabs, wrap='word', padx=12, pady=12)
        setup.insert('1.0', SETUP)
        setup.configure(state='disabled')
        self.tabs.add(setup, text='Setup and limits')
        root.protocol('WM_DELETE_WINDOW', self.close)
        self.render()
        self.timer = root.after(50, self.poll)

    def render(self):
        connected = self.serial is not None
        state = 'LIVE progress' if self.session.fresh(connected) else 'NOT LIVE'
        self.status.set(f'{self.session.source.upper()} | {state} | '
                        f'errors {self.session.errors} | evicted {self.session.evicted} | '
                        f'{self.session.blocked or self.notice}')
        self.history.delete(*self.history.get_children())
        if not self.session.rows:
            self.summary.set('No accepted observations in this session.')
            return
        r = self.session.rows[-1]
        self.summary.set(
            f'Phase: {PHASES[r["phase"]]}    fault mask: {r["fault"]}\n\n'
            f'Started: {r["starts"]}    completed: {r["completed"]}\n'
            f'Nested completions: {r["nested"]}    aborted: {r["aborted"]}\n'
            f'Witnessed inner trips: {r["inner_trips"]}\n'
            f'Ordering/overlap diagnostics: {r["violations"]}\n'
            f'Timeouts: {r["timeouts"]}    sampling gaps: {r["gaps"]}\n\n'
            f'Raw mask: {r["raw"]:04b}    qualified: {r["qualified"]:04b}\n'
            f'Current age: {r["age"]} ms    current inner trips: {r["cycle_trips"]}\n'
            f'Last: {RESULTS[r["last"]]} ({r["last_age"]} ms)\n'
            f'Longest completed cycle: {r["maximum"]} ms\n\n'
            'Contact evidence only. No physical safety or quality conclusion.')
        for row in self.session.rows:
            self.history.insert('', 'end', values=(row['now'], PHASES[row['phase']],
                row['raw'], row['completed'], row['nested'], row['aborted'], RESULTS[row['last']]))

    def connect(self):
        if self.serial is not None:
            messagebox.showerror('Connection', 'Disconnect the current port first.')
            return
        try:
            serial = Serial(self.port.get())
        except (OSError, termios.error) as exc:
            messagebox.showerror('Connection failed', str(exc))
            return
        self.serial = serial
        self.decoder = Decoder()
        self.session = Session('live')
        self.notice = 'Waiting for advancing acquisition'
        self.render()

    def disconnect(self):
        if self.serial is not None:
            try:
                self.decoder.finish()
            except ValueError:
                pass
            self.session.errors = self.decoder.errors
            self.serial.close()
            self.serial = None
        self.notice = 'Disconnected; evidence retained'
        self.render()

    def replay(self, path=None):
        if self.serial is not None:
            messagebox.showerror('Replay', 'Disconnect before replacing the session.')
            return
        path = path or filedialog.askopenfilename(title='Open SV1 recording')
        if not path:
            return
        try:
            session = load_replay(path)
        except (OSError, ValueError) as exc:
            messagebox.showerror('Replay failed', str(exc))
            return
        self.session = session
        self.notice = 'Recording, not live hardware'
        self.render()

    def export(self, path=None):
        path = path or filedialog.asksaveasfilename(title='New private CSV', defaultextension='.csv')
        if not path:
            return
        try:
            self.session.export(path, self.serial is not None)
            self.notice = 'Exported ' + path
            self.render()
        except OSError as exc:
            messagebox.showerror('Export failed', str(exc))

    def poll(self):
        if self.timer is not None:
            self.root.after_cancel(self.timer)
            self.timer = None
        try:
            if self.serial is not None and not self.session.blocked:
                data = self.serial.read()
                if data:
                    rows = self.decoder.feed(data)
                    self.session.errors = self.decoder.errors
                    for row in rows:
                        self.session.accept(row)
        except (OSError, ValueError, termios.error) as exc:
            self.disconnect()
            self.notice = str(exc)
        self.render()
        self.timer = self.root.after(100, self.poll)

    def close(self):
        if self.timer is not None:
            self.root.after_cancel(self.timer)
            self.timer = None
        self.disconnect()
        self.root.destroy()

if __name__ == '__main__':
    window = tk.Tk()
    Workbench(window)
    window.mainloop()
