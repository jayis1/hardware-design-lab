#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Linux Tk workbench; standard library only, no network or device writes."""
import argparse
import os
import select
import termios
import time
import tty
import tkinter as tk
from tkinter import ttk, filedialog, messagebox
from protocol import FIELDS, Session, Stream, read_replay


class Port:
    def __init__(self, path):
        self.fd = os.open(path, os.O_RDONLY | os.O_NONBLOCK | os.O_NOCTTY)
        self.saved = None
        try:
            self.saved = termios.tcgetattr(self.fd)
            tty.setraw(self.fd)
            settings = termios.tcgetattr(self.fd)
            settings[4] = settings[5] = termios.B115200
            settings[2] |= termios.CLOCAL | termios.CREAD
            termios.tcsetattr(self.fd, termios.TCSANOW, settings)
            termios.tcflush(self.fd, termios.TCIFLUSH)
            self.poller = select.poll()
            self.poller.register(self.fd, select.POLLIN | select.POLLHUP | select.POLLERR)
        except (OSError, termios.error):
            self.close()
            raise

    def read(self):
        if self.fd is None:
            raise ValueError('serial port is closed')
        flags = 0
        for _, event in self.poller.poll(0):
            flags |= event
        data = b''
        if flags & select.POLLIN:
            try:
                data = os.read(self.fd, 4096)
            except BlockingIOError:
                pass
        return data, bool(flags & (select.POLLHUP | select.POLLERR | select.POLLNVAL))

    def close(self):
        if self.fd is not None:
            try:
                if self.saved is not None:
                    termios.tcsetattr(self.fd, termios.TCSANOW, self.saved)
            except (OSError, termios.error):
                pass
            finally:
                os.close(self.fd)
                self.fd = None


class App:
    def __init__(self, root):
        self.root = root
        root.title('Ratchet Audit — jayis1')
        root.geometry('1050x650')
        self.port = None
        self.session = Session('none')
        self.stream = Stream()
        self.status = tk.StringVar(value='disconnected; no evidence')
        self.live = tk.StringVar(value='No snapshots')
        self.path = tk.StringVar(value='/dev/ttyACM0')
        self.tabs = ttk.Notebook(root)
        self.tabs.pack(fill='both', expand=True)
        connection = ttk.Frame(self.tabs, padding=15)
        live = ttk.Frame(self.tabs, padding=15)
        history = ttk.Frame(self.tabs, padding=15)
        self.tabs.add(connection, text='Connection / replay')
        self.tabs.add(live, text='Live evidence')
        self.tabs.add(history, text='History / export')
        ttk.Label(connection, text='Ratchet Audit | Author: jayis1\n'
                  'Diagnostic bench use only. No safety or lock certification.\n'
                  'Connect dry contacts, align positive rotation, then hold HOME + PAWL.').pack(anchor='w')
        ttk.Entry(connection, textvariable=self.path, width=65).pack(anchor='w', pady=15)
        ttk.Button(connection, text='Connect serial', command=lambda: self.action(
            lambda: self.connect(self.path.get()))).pack(anchor='w')
        ttk.Button(connection, text='Disconnect (retain evidence)', command=self.disconnect).pack(anchor='w')
        ttk.Button(connection, text='Open RA1 recording', command=self.choose_replay).pack(anchor='w')
        ttk.Label(live, textvariable=self.live, font=('monospace', 12), justify='left').pack(anchor='w')
        ttk.Label(live, text='WAIT=0  READY=1  ACTIVE=2  HALTED=3\n'
                  'Reverse while PAWL is indicated is a discrepancy, not proof of tooth failure.\n'
                  'All quantities are encoder edges. No automatic angle or torque calibration.').pack(anchor='w', pady=15)
        columns = ('now', 'starts', 'completed', 'aborted', 'forward', 'reverse', 'seated_reverse', 'rollback', 'faults')
        self.history = ttk.Treeview(history, columns=columns, show='headings', height=18)
        for name in columns:
            self.history.heading(name, text=name)
            self.history.column(name, width=100, stretch=True)
        self.history.pack(fill='both', expand=True)
        ttk.Button(history, text='Export evidence JSON (new file)', command=self.choose_export).pack(anchor='w')
        ttk.Label(root, textvariable=self.status, wraplength=1000).pack(fill='x')
        self.timer = root.after(25, self.poll)
        root.protocol('WM_DELETE_WINDOW', self.close)

    def action(self, operation):
        try:
            operation()
        except (OSError, ValueError, termios.error) as exc:
            messagebox.showerror('Ratchet Audit', str(exc))

    def connect(self, path):
        if self.port is not None:
            raise ValueError('Disconnect the current serial port first')
        port = Port(path)
        # A failed port setup must not erase retained evidence.
        self.port = port
        self.session = Session('live')
        self.stream = Stream()
        self.render()

    def disconnect(self):
        if self.port:
            self.port.close()
            self.port = None
        self.render()

    def choose_replay(self):
        path = filedialog.askopenfilename(title='Open bounded RA1 recording')
        if path:
            self.action(lambda: self.replay(path))

    def replay(self, path):
        if self.port:
            raise ValueError('Disconnect before replacing evidence with replay')
        session = read_replay(path)
        self.session = session
        self.render()

    def choose_export(self):
        path = filedialog.asksaveasfilename(defaultextension='.json')
        if path:
            self.action(lambda: self.session.export(path, self.port is not None, time.monotonic()))

    def poll(self):
        try:
            if self.port:
                data, hangup = self.port.read()
                rows = self.stream.feed(data)
                self.session.parser_errors = self.stream.errors
                for row in rows:
                    self.session.accept(row, time.monotonic())
                if hangup:
                    self.disconnect()
        except (OSError, ValueError, termios.error) as exc:
            if not self.session.blocked:
                self.session.blocked = str(exc)
            self.disconnect()
        self.render()
        self.timer = self.root.after(25, self.poll)

    def render(self):
        connected = self.port is not None
        fresh = self.session.fresh(time.monotonic(), connected)
        self.status.set(f'{"serial open" if connected else "disconnected"} | '
                        f'{self.session.source} | {"fresh" if fresh else "not fresh"} | '
                        f'parser errors {self.session.parser_errors} | '
                        f'dropped snapshots {self.session.dropped} | {self.session.blocked}')
        if self.session.rows:
            row = self.session.rows[-1]
            self.live.set('\n'.join(f'{key:18} {row[key]}' for key in FIELDS))
        else:
            self.live.set('No snapshots')
        # Update history only when the dataset changes, not every timer tick.
        signature = (id(self.session), self.session.progress, self.session.dropped)
        if getattr(self, 'history_signature', None) != signature:
            self.history_signature = signature
            self.history.delete(*self.history.get_children())
            for row in self.session.rows:
                self.history.insert('', 'end', values=[row[k] for k in self.history['columns']])

    def close(self):
        self.root.after_cancel(self.timer)
        self.disconnect()
        self.root.destroy()


def main():
    parser = argparse.ArgumentParser(description='Ratchet Audit — author jayis1')
    parser.add_argument('--check', help='validate an RA1 recording without opening a window')
    args = parser.parse_args()
    if args.check:
        session = read_replay(args.check)
        print(f'Valid replay: {len(session.rows)} retained snapshots; not physical validation')
    else:
        root = tk.Tk()
        App(root)
        root.mainloop()

if __name__ == '__main__':
    main()
