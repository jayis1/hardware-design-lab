#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Sand Stroke: Linux serial capture, local replay and private CSV export."""
import csv
import os
import select
import termios
import time
import tkinter as tk
from tkinter import filedialog, messagebox, ttk
from protocol import FIELDS, Session, Stream, replay

class Serial:
    def __init__(self, path):
        self.fd = os.open(path, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
        self.original = None
        try:
            self.original = termios.tcgetattr(self.fd)
            settings = termios.tcgetattr(self.fd)
            settings[0] = 0
            settings[1] = 0
            settings[2] = termios.CLOCAL | termios.CREAD | termios.CS8
            settings[3] = 0
            settings[4] = settings[5] = termios.B115200
            settings[6][termios.VMIN] = 0
            settings[6][termios.VTIME] = 0
            termios.tcsetattr(self.fd, termios.TCSANOW, settings)
            termios.tcflush(self.fd, termios.TCIFLUSH)
            self.poller = select.poll()
            self.poller.register(self.fd, select.POLLIN | select.POLLERR | select.POLLHUP)
        except Exception:
            self.close()
            raise

    def read(self):
        if self.fd is None:
            return b'', True
        data = b''
        dead = False
        for _, flags in self.poller.poll(0):
            if flags & select.POLLIN:
                try:
                    data = os.read(self.fd, 8192)
                except BlockingIOError:
                    pass
                except OSError:
                    dead = True
            dead |= bool(flags & (select.POLLERR | select.POLLHUP | select.POLLNVAL))
        return data, dead

    def close(self):
        if self.fd is not None:
            try:
                if self.original is not None:
                    termios.tcsetattr(self.fd, termios.TCSANOW, self.original)
            except (OSError, termios.error):
                pass
            finally:
                os.close(self.fd)
                self.fd = None

class Workbench:
    def __init__(self, root):
        self.root = root
        self.root.title('Sand Stroke — jayis1')
        self.root.geometry('1000x540')
        self.serial = None
        self.session = Session('none')
        self.stream = Stream()
        self.path = tk.StringVar(value='/dev/ttyACM0')
        self.status = tk.StringVar(value='Disconnected; no measurements')
        self.live = tk.StringVar(value='No observations')
        self.tabs = ttk.Notebook(root)
        self.tabs.pack(fill='both', expand=True)
        connect = ttk.Frame(self.tabs, padding=12)
        live = ttk.Frame(self.tabs, padding=12)
        history = ttk.Frame(self.tabs, padding=12)
        self.tabs.add(connect, text='Connection / fixture')
        self.tabs.add(live, text='Stroke balance')
        self.tabs.add(history, text='History / export')
        ttk.Label(connect, text='Author: jayis1\nPassive observer; not a removal, force or finish gauge.\n'
                  'J3 left endpoint; J4 right endpoint; J5 engaged; J6 face B.\n'
                  'Voltage-free switches only. Lift before changing face.\n'
                  'A full stroke requires engaged departure and opposite arrival within 10 s.\n'
                  'Ten consecutive 1 ms samples qualify each contact.').pack(anchor='w', pady=12)
        ttk.Entry(connect, textvariable=self.path, width=60).pack(anchor='w')
        ttk.Button(connect, text='Connect', command=lambda: self.action(self.connect, self.path.get())).pack(anchor='w')
        ttk.Button(connect, text='Disconnect', command=self.disconnect).pack(anchor='w')
        ttk.Button(connect, text='Replay recording', command=self.choose_replay).pack(anchor='w')
        ttk.Label(live, textvariable=self.live, font=('TkDefaultFont', 15), justify='left').pack(anchor='w')
        ttk.Label(live, text='Counts are endpoint traversals, not abrasive work.\n'
                  'Elapsed totals exclude rejected strokes and endpoint residence.\n'
                  'Face is a declared fixture selector, not automatic specimen identification.').pack(anchor='w', pady=20)
        columns = ('time', 'state', 'face', 'a', 'b', 'abort', 'gap')
        self.history = ttk.Treeview(history, columns=columns, show='headings', height=14)
        for name in columns:
            self.history.heading(name, text=name)
            self.history.column(name, width=100)
        self.history.pack(fill='both', expand=True)
        ttk.Button(history, text='Export new CSV', command=self.choose_export).pack(anchor='w')
        ttk.Label(root, textvariable=self.status, wraplength=950).pack(fill='x')
        root.protocol('WM_DELETE_WINDOW', self.close)
        self.timer = root.after(25, self.poll)

    def action(self, function, *args):
        try:
            function(*args)
        except (OSError, ValueError, termios.error) as error:
            messagebox.showerror('Sand Stroke', str(error), parent=self.root)

    def connect(self, path):
        if self.serial:
            raise ValueError('Disconnect before opening another port')
        port = Serial(path)  # Preserve retained evidence if opening fails.
        self.serial = port
        self.session = Session('live')
        self.stream = Stream()
        self.refresh()

    def disconnect(self):
        if self.serial:
            if self.stream.buffer:
                self.stream.errors += 1
                self.stream.buffer = b''
            self.session.errors = self.stream.errors
            self.serial.close()
            self.serial = None
        self.session.connected = False
        self.refresh()

    def load(self, path):
        if self.serial:
            raise ValueError('Disconnect before replay')
        candidate = replay(path)
        self.session = candidate
        self.refresh()

    def choose_replay(self):
        path = filedialog.askopenfilename(parent=self.root)
        if path:
            self.action(self.load, path)

    def choose_export(self):
        path = filedialog.asksaveasfilename(parent=self.root, defaultextension='.csv')
        if path:
            self.action(self.export, path)

    def export(self, path):
        # Exclusive creation and owner-only mode protect older evidence.
        fd = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
        with os.fdopen(fd, 'w', newline='') as file:
            writer = csv.writer(file)
            writer.writerow(['author', 'jayis1', 'provenance', self.session.provenance,
                             'connected', self.session.connected,
                             'fresh', self.session.fresh(time.monotonic()),
                             'errors', self.session.errors, 'evicted', self.session.evicted,
                             'warning', self.session.warning])
            writer.writerow(FIELDS)
            for row in self.session.rows:
                writer.writerow([row[k] for k in FIELDS])

    def refresh(self):
        s = self.session
        label = 'live progress' if s.fresh(time.monotonic()) else 'not live / stale / historical'
        self.status.set(f'{s.provenance}: {label}; connected={s.connected}; errors={s.errors}; '
                        f'evicted={s.evicted}; {s.warning}')
        if s.rows:
            r = s.rows[-1]
            self.live.set(f"State: {('WAIT','READY','ACTIVE','HALT')[r['state']]}   "
                          f"Contacts: {r['mask']:04b}   Known: {r['known']:04b}\n"
                          f"Full A: {r['full_a']}    Full B: {r['full_b']}\n"
                          f"Accepted transit ms A: {r['ms_a']}    B: {r['ms_b']}\n"
                          f"Started: {r['started']}    Aborted: {r['aborted']}\n"
                          f"Short: {r['short_returns']}  Lift: {r['lifts']}  Flip: {r['flips']}\n"
                          f"Collision: {r['collisions']}  Timeout: {r['timeouts']}\n"
                          f"Gaps: {r['gaps']}  TX: {r['tx']}  Fault: {r['fault']}")
        else:
            self.live.set('No observations')
        self.history.delete(*self.history.get_children())
        for row in s.rows:
            self.history.insert('', 'end', values=[row[k] for k in
                                ('now','state','face','full_a','full_b','aborted','gaps')])

    def poll(self):
        if self.serial:
            try:
                data, dead = self.serial.read()
                rows = self.stream.feed(data)
                self.session.errors = self.stream.errors
                for row in rows:
                    self.session.accept(row, time.monotonic())
                if dead:
                    self.session.warning = self.session.warning or 'Serial disconnected'
                    self.disconnect()
            except (OSError, ValueError, termios.error) as error:
                self.session.warning = str(error)
                self.disconnect()
        self.refresh()
        self.timer = self.root.after(50, self.poll)

    def close(self):
        self.root.after_cancel(self.timer)
        self.disconnect()
        self.root.destroy()

if __name__ == '__main__':
    root = tk.Tk()
    Workbench(root)
    root.mainloop()
