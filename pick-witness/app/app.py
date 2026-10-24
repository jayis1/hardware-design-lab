#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Pick Witness: Linux serial/replay workbench; standard library + Tk only."""
import collections
import csv
import fcntl
import os
import pathlib
import re
import stat
import termios
import time
import tkinter as tk
from tkinter import ttk, filedialog, messagebox

FIELDS = ('sequence', 'millis', 'contacts', 'ready', 'pending', 'accepted',
          'rejected', 'anomalies', 'gaps', 'events', 'reason', 'last_ms',
          'min_ms', 'max_ms', 'tx_losses', 'saturated', 'qualify_ms', 'limit_ms')
REASONS = ('none', 'accepted', 'beat without pass', 'repeated shed', 'both docks',
           'unbeaten pass', 'shed moved before beat', 'arrival expired',
           'acquisition gap', 'ambiguous order', 'returned to origin', 'counter limit', 'flight expired')


def crc(data):
    value = 0xffff
    for byte in data:
        value ^= byte << 8
        for _ in range(8):
            value = ((value << 1) ^ (0x1021 if value & 0x8000 else 0)) & 0xffff
    return value


def decode(line):
    if len(line) > 256:
        raise ValueError('oversize frame')
    match = re.fullmatch(rb'(@PW1(?:,[0-9]{1,10}){18})\*([0-9A-F]{4})\n', line)
    if not match or crc(match[1]) != int(match[2], 16):
        raise ValueError('version, framing or CRC')
    values = [int(x) for x in match[1].split(b',')[1:]]
    if any(x > 0xffffffff for x in values):
        raise ValueError('integer overflow')
    row = dict(zip(FIELDS, values))
    if row['contacts'] > 15 or any(row[k] > 1 for k in ('ready', 'pending', 'saturated')):
        raise ValueError('state range')
    if row['reason'] >= len(REASONS) or row['qualify_ms'] != 20 or row['limit_ms'] != 10000:
        raise ValueError('incompatible policy')
    return row


class Model:
    def __init__(self):
        self.start('none')
        self.connected = False

    def start(self, source):
        self.source = source
        self.connected = True
        self.buffer = bytearray()
        self.rows = collections.deque(maxlen=4096)
        self.latest = None
        self.errors = 0
        self.dropped = 0
        self.missing = 0
        self.blocked = False
        self.updated = None

    def feed(self, data):
        # Byte-wise cap keeps memory bounded even for a hostile serial peer.
        for byte in data:
            self.buffer.append(byte)
            if self.buffer.endswith(b'@PW1') and len(self.buffer) > 4:
                self.errors += 1
                self.buffer[:] = b'@PW1'
            if len(self.buffer) > 256:
                self.errors += 1
                self.buffer.clear()
            elif byte == 10:
                line = bytes(self.buffer)
                self.buffer.clear()
                try:
                    row = decode(line)
                    self.accept(row)
                except ValueError:
                    self.errors += 1

    def accept(self, row):
        if self.blocked:
            return
        if self.latest is not None:
            ds = (row['sequence'] - self.latest['sequence']) & 0xffffffff
            dt = (row['millis'] - self.latest['millis']) & 0xffffffff
            if ds == 0 or ds >= 0x80000000 or dt >= 0x80000000:
                self.blocked = True
                return
            if any(row[k] < self.latest[k] for k in
                   ('accepted', 'rejected', 'anomalies', 'gaps', 'events', 'tx_losses')):
                self.blocked = True
                return
            self.missing += ds - 1
        if len(self.rows) == self.rows.maxlen:
            self.dropped += 1
        self.latest = row
        self.updated = time.monotonic()
        self.rows.append((time.time(), row.copy()))

    def age(self):
        return None if self.updated is None else time.monotonic() - self.updated

    def export(self, path):
        # No overwrite, no world-readable workflow history by default.
        descriptor = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
        with os.fdopen(descriptor, 'w', newline='') as out:
            writer = csv.writer(out)
            writer.writerow(('author', 'source', 'connected', 'suspected_reset',
                             'parser_errors', 'missing_frames', 'history_dropped',
                             'export_age_seconds', 'received_unix', *FIELDS))
            for received, row in self.rows:
                writer.writerow(('jayis1', self.source, self.connected, self.blocked,
                                 self.errors, self.missing, self.dropped, self.age(),
                                 received, *(row[k] for k in FIELDS)))


class Workbench:
    def __init__(self, root):
        self.root = root
        self.model = Model()
        self.fd = None
        self.saved_termios = None
        root.title('Pick Witness — jayis1')
        root.geometry('1000x640')
        self.status = tk.StringVar(value='Disconnected; retained values are not live.')
        self.summary = tk.StringVar(value='No observations')
        self.path = tk.StringVar(value='/dev/ttyACM0')
        book = ttk.Notebook(root)
        book.pack(fill='both', expand=True)
        self.book = book
        setup, live, history = (ttk.Frame(book, padding=12) for _ in range(3))
        for frame, name in ((setup, 'Connection / setup'), (live, 'Live witness'), (history, 'History / export')):
            book.add(frame, text=name)
        ttk.Label(setup, text='Linux ST-LINK serial device (115200 8N1, receive only)').pack(anchor='w')
        ttk.Entry(setup, textvariable=self.path, width=60).pack(anchor='w', pady=8)
        controls = ttk.Frame(setup); controls.pack(anchor='w')
        for name, action in (('Connect / new session', self.connect), ('Disconnect', self.disconnect),
                             ('Replay file', self.replay), ('Export CSV', self.export)):
            ttk.Button(controls, text=name, command=action).pack(side='left', padx=4)
        ttk.Label(setup, wraplength=850, justify='left', text=(
            'Use only a slow manual two-shed plain weave. J3 left shuttle dock; J4 right dock; '
            'J5 beater forward; J6 shed B. Closed contacts are 1. Start with exactly one dock '
            'closed and beater released. Move away from the dock before arriving at the other. '
            'Beat only after arrival, then change shed. Qualification: 20 ms; arrival expires '
            'after 10 s. Counts describe contact sequences, not actual thread insertion. '
            'No motor control, no safety interlock, no fabric certification. Export before '
            'starting a new session; new connections clear retained history.')).pack(anchor='w', pady=24)
        ttk.Label(live, textvariable=self.summary, font=('TkDefaultFont', 17),
                  justify='left').pack(anchor='w', pady=20)
        ttk.Label(live, text='Sticky reason describes the latest event, not a continuing alarm.').pack(anchor='w')
        self.tree = ttk.Treeview(history, columns=('time', 'accepted', 'rejected', 'reason'), show='headings')
        for column in self.tree['columns']:
            self.tree.heading(column, text=column)
        self.tree.pack(fill='both', expand=True)
        ttk.Button(history, text='Export bounded session CSV', command=self.export).pack(anchor='w', pady=10)
        ttk.Label(root, textvariable=self.status, wraplength=970).pack(fill='x', padx=12, pady=10)
        self.last_render = None
        self.root.protocol('WM_DELETE_WINDOW', self.close)
        self.after_id = root.after(50, self.poll)

    def disconnect(self):
        if self.fd is not None:
            try:
                if self.saved_termios is not None:
                    termios.tcsetattr(self.fd, termios.TCSANOW, self.saved_termios)
            except (OSError, termios.error):
                pass
            os.close(self.fd)
        self.fd = None
        self.saved_termios = None
        self.model.connected = False
        self.render()

    def connect(self):
        self.disconnect()
        fd = None
        try:
            fd = os.open(self.path.get(), os.O_RDONLY | os.O_NOCTTY | os.O_NONBLOCK)
            if not stat.S_ISCHR(os.fstat(fd).st_mode):
                raise ValueError('Not a serial character device')
            fcntl.ioctl(fd, termios.TIOCEXCL)
            saved = termios.tcgetattr(fd)
            settings = termios.tcgetattr(fd)
            settings[0] = 0; settings[1] = 0
            settings[2] = termios.CLOCAL | termios.CREAD | termios.CS8
            settings[3] = 0
            settings[4] = termios.B115200; settings[5] = termios.B115200
            settings[6][termios.VMIN] = 1; settings[6][termios.VTIME] = 0
            termios.tcsetattr(fd, termios.TCSANOW, settings)
            termios.tcflush(fd, termios.TCIFLUSH)
            self.fd = fd
            self.saved_termios = saved
            self.model.start('live serial ' + self.path.get())
            self.last_render = None
            self.tree.delete(*self.tree.get_children())
            self.render()
            self.book.select(1)
        except (OSError, ValueError, termios.error) as error:
            if fd is not None:
                os.close(fd)
            messagebox.showerror('Connection failed', str(error))

    def replay(self):
        path = filedialog.askopenfilename(title='Synthetic or previously captured PW1 stream')
        if not path:
            return
        try:
            with open(path, 'rb') as source:
                data = source.read(1048577)
            if len(data) > 1048576:
                raise ValueError('Replay limited to 1 MiB')
            self.disconnect()
            self.model.start('replay ' + pathlib.Path(path).name)
            self.model.feed(data)
            self.model.connected = False
            self.last_render = None
            self.tree.delete(*self.tree.get_children())
            self.render()
            self.book.select(1)
        except (OSError, ValueError) as error:
            messagebox.showerror('Replay failed', str(error))

    def export(self):
        path = filedialog.asksaveasfilename(defaultextension='.csv')
        if path:
            try:
                self.model.export(path)
            except OSError as error:
                messagebox.showerror('Export failed (overwrites forbidden)', str(error))

    def render(self):
        row = self.model.latest
        age = self.model.age()
        fresh = self.fd is not None and age is not None and age < 1.5 and not self.model.blocked
        self.status.set(f'{"LIVE" if fresh else "NOT LIVE"} | {self.model.source} | '
                        f'connected={self.model.connected} | reset blocked={self.model.blocked} | '
                        f'bad frames={self.model.errors} | missing={self.model.missing} | '
                        f'history dropped={self.model.dropped} | age={"none" if age is None else f"{age:.1f}s"}')
        if row is None:
            self.summary.set('No observations — waiting for valid PW1 policy frame')
            return
        self.summary.set(f'Accepted sequences: {row["accepted"]}\n'
                         f'Rejected beats: {row["rejected"]}\n'
                         f'Anomalies: {row["anomalies"]}    acquisition gaps: {row["gaps"]}\n'
                         f'Contacts L/R/beat/B: {row["contacts"]:04b} (B at left)\n'
                         f'Ready: {row["ready"]}    pending arrival: {row["pending"]}\n'
                         f'Arrival-to-beat: {row["last_ms"]} ms\n'
                         f'Latest reason: {REASONS[row["reason"]]}\n'
                         f'Transport losses: {row["tx_losses"]}    saturated: {row["saturated"]}')
        if self.last_render != row['sequence']:
            self.last_render = row['sequence']
            self.tree.delete(*self.tree.get_children())
            for _, retained in list(self.model.rows)[-200:]:
                self.tree.insert('', 'end', values=(retained['millis'], retained['accepted'],
                                 retained['rejected'], REASONS[retained['reason']]))
            self.tree.see(self.tree.get_children()[-1])

    def poll(self):
        if self.fd is not None:
            try:
                # Bound each Tk turn so a noisy peer cannot starve the interface.
                data = os.read(self.fd, 4096)
                if not data:
                    self.disconnect()
                else:
                    self.model.feed(data)
            except BlockingIOError:
                pass
            except OSError:
                self.disconnect()
        self.render()
        self.after_id = self.root.after(50, self.poll)

    def close(self):
        self.root.after_cancel(self.after_id)
        self.disconnect()
        self.root.destroy()


if __name__ == '__main__':
    root = tk.Tk()
    Workbench(root)
    root.mainloop()
