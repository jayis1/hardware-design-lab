#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Pedal Window: offline Linux/Tk contact-timing notebook, no dependencies."""
import csv
import os
from pathlib import Path
import re
import select
import termios
import time
import tkinter as tk
from tkinter import filedialog, messagebox, ttk

FIELDS = ('seq', 'ms', 'flags', 'levels', 'phase', 'attempts', 'completed',
          'rejected', 'gaps', 'transport_losses', 'id', 'outcome', 'result_flags',
          'key_gap', 'release_delta', 'down_delta', 'debounce', 'trial_limit')
PHASES = ('Release both keys and pedal', 'Ready: hold A and depress pedal',
          'Capture: transfer A to B and cycle pedal')


def crc(data):
    value = 0xffff
    for byte in data:
        value ^= byte << 8
        for _ in range(8):
            value = ((value << 1) ^ (0x1021 if value & 0x8000 else 0)) & 0xffff
    return value


def parse(line):
    if len(line) > 256:
        raise ValueError('frame too long')
    match = re.fullmatch(rb'(PW1,[0-9,\-]+)\*([0-9A-F]{4})\n', line)
    if not match or crc(match[1]) != int(match[2], 16):
        raise ValueError('framing or CRC')
    words = match[1].split(b',')[1:]
    if len(words) != len(FIELDS):
        raise ValueError('field count')
    values = [int(word) for word in words]
    for index, value in enumerate(values):
        low, high = (-5000, 5000) if 13 <= index <= 15 else (0, 0xffffffff)
        if not low <= value <= high:
            raise ValueError('field range')
    record = dict(zip(FIELDS, values))
    if record['levels'] > 15 or record['phase'] > 2 or record['outcome'] > 2:
        raise ValueError('enum')
    if record['flags'] > 127 or record['result_flags'] > 127:
        raise ValueError('flags')
    if (record['debounce'], record['trial_limit']) != (8, 5000):
        raise ValueError('unsupported firmware policy')
    if record['completed'] + record['rejected'] > record['attempts']:
        raise ValueError('counter consistency')
    if record['id'] > record['attempts']:
        raise ValueError('result identity')
    outcome = record['outcome']
    if bool(outcome) != bool(record['id']):
        raise ValueError('absent result identity')
    intervals = ('key_gap', 'release_delta', 'down_delta')
    if outcome != 1 and any(record[key] for key in intervals):
        raise ValueError('intervals without completion')
    if outcome == 1:
        if not record['completed'] or record['result_flags'] & ~1:
            raise ValueError('invalid completion flags or counter')
        if record['down_delta'] < max(0, record['release_delta'], -record['key_gap']):
            raise ValueError('invalid event order')
        times = (-record['key_gap'], 0, record['release_delta'], record['down_delta'])
        if max(times) - min(times) >= record['trial_limit']:
            raise ValueError('event span exceeds trial limit')
    if outcome == 2 and not record['rejected']:
        raise ValueError('invalid rejection counter')
    return record


class Model:
    def __init__(self):
        self.pending = b''
        self.latest = None
        self.history = []
        self.errors = 0
        self.missed = 0
        self.last_seen = None
        self.blocked = False
        self.source = 'none'
        self.last_result = 0

    def stale(self, now):
        return self.last_seen is None or now - self.last_seen > 1.0

    def feed(self, data, now):
        self.pending += data
        while self.pending:
            start = self.pending.find(b'PW1,')
            if start < 0:
                if len(self.pending) > 256:
                    self.errors += 1
                    self.pending = self.pending[-3:]
                return
            if start:
                self.errors += 1
                self.pending = self.pending[start:]
            newline = self.pending.find(b'\n')
            restart = self.pending.find(b'PW1,', 4)
            if restart >= 0 and (newline < 0 or restart < newline):
                self.errors += 1
                self.pending = self.pending[restart:]
                continue
            if newline < 0:
                if len(self.pending) > 256:
                    self.errors += 1
                    self.pending = self.pending[-3:]
                return
            line, self.pending = self.pending[:newline+1], self.pending[newline+1:]
            try:
                record = parse(line)
            except (ValueError, OverflowError):
                self.errors += 1
                continue
            self.accept(record, now)

    def accept(self, record, now):
        if self.blocked:
            return
        if self.latest:
            delta = (record['seq'] - self.latest['seq']) & 0xffffffff
            clock = (record['ms'] - self.latest['ms']) & 0xffffffff
            counters = ('attempts', 'completed', 'rejected', 'gaps', 'transport_losses')
            if delta >= 0x80000000 or clock >= 0x80000000 or any(
                    record[key] < self.latest[key] for key in counters):
                self.blocked = True
                return
            if not delta:
                return  # Duplicates are not freshness evidence.
            self.missed += delta - 1
        self.latest = record
        self.last_seen = now
        if record['outcome'] and record['id'] > self.last_result:
            self.missed += max(0, record['id'] - self.last_result - 1)
            self.last_result = record['id']
            self.history.append(record.copy())
            self.history = self.history[-256:]

    def export(self, path, connected, now):
        # Exclusive creation avoids silently replacing a lesson recording.
        with open(path, 'x', newline='', encoding='utf-8') as stream:
            os.chmod(path, 0o600)
            writer = csv.writer(stream)
            writer.writerow(['author', 'jayis1', 'source', self.source,
                             'connected', connected, 'stale', self.stale(now),
                             'blocked', self.blocked, 'parser_errors', self.errors,
                             'missing_evidence', self.missed])
            writer.writerow(FIELDS)
            for row in self.history:
                writer.writerow([row[key] for key in FIELDS])


class Serial:
    def __init__(self, path):
        self.fd = os.open(path, os.O_RDONLY | os.O_NOCTTY | os.O_NONBLOCK)
        self.saved = None
        try:
            self.saved = termios.tcgetattr(self.fd)
            attrs = termios.tcgetattr(self.fd)
            attrs[0] = attrs[1] = attrs[3] = 0
            attrs[2] = termios.CS8 | termios.CREAD | termios.CLOCAL
            attrs[4] = attrs[5] = termios.B115200
            attrs[6][termios.VMIN] = 0
            attrs[6][termios.VTIME] = 0
            termios.tcsetattr(self.fd, termios.TCSANOW, attrs)
            termios.tcflush(self.fd, termios.TCIFLUSH)
            self.poller = select.poll()
            self.poller.register(self.fd, select.POLLIN | select.POLLHUP | select.POLLERR)
        except (OSError, termios.error):
            self.close()
            raise

    def read(self):
        if self.fd is None:
            raise OSError('serial closed')
        for _, events in self.poller.poll(0):
            if events & (select.POLLHUP | select.POLLERR | select.POLLNVAL):
                raise OSError('serial disconnected')
        try:
            return os.read(self.fd, 4096)
        except BlockingIOError:
            return b''

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
        self.model = Model()
        self.serial = None
        self.message = 'No device; replay is explicitly synthetic or imported evidence.'
        root.title('Pedal Window — jayis1')
        root.geometry('1000x620')
        self.path = tk.StringVar(value='/dev/ttyACM0')
        toolbar = ttk.Frame(root)
        toolbar.pack(fill='x')
        ttk.Entry(toolbar, textvariable=self.path, width=28).pack(side='left')
        ttk.Button(toolbar, text='Connect / new session', command=self.connect).pack(side='left')
        ttk.Button(toolbar, text='Disconnect', command=self.disconnect).pack(side='left')
        ttk.Button(toolbar, text='Replay file', command=self.replay_dialog).pack(side='left')
        ttk.Button(toolbar, text='Export CSV', command=self.export_dialog).pack(side='left')
        self.status = tk.StringVar()
        ttk.Label(root, textvariable=self.status, wraplength=970).pack(fill='x', pady=8)
        self.tabs = ttk.Notebook(root)
        self.tabs.pack(fill='both', expand=True)
        live, history, setup = [ttk.Frame(self.tabs) for _ in range(3)]
        for panel, name in [(live, 'Live exercise'), (history, 'Trial notebook'), (setup, 'Mounting and interpretation')]:
            self.tabs.add(panel, text=name)
        self.summary = tk.StringVar(value='No qualified measurements')
        ttk.Label(live, textvariable=self.summary, font=('TkDefaultFont', 14), wraplength=950).pack(pady=20)
        self.contacts = tk.StringVar()
        ttk.Label(live, textvariable=self.contacts).pack(pady=10)
        self.metrics = tk.StringVar()
        ttk.Label(live, textvariable=self.metrics, wraplength=950).pack(pady=10)
        columns = ('id', 'outcome', 'key_gap', 'release_delta', 'down_delta', 'result_flags')
        self.table = ttk.Treeview(history, columns=columns, show='headings')
        for name in columns:
            self.table.heading(name, text=name)
            self.table.column(name, width=140)
        self.table.pack(fill='both', expand=True)
        text = ('Author: jayis1\n\nAttach only isolated, voltage-free contacts. Never connect piano electronics. '
                'Use two removable key travel flags plus separate pedal REST and DOWN contacts. '
                'Neither pedal contact should be closed in the middle of travel; both closed is a fault.\n\n'
                'Release both keys and pedal to establish baseline. Hold A and depress the pedal. '
                'Move from A to B, fully release the pedal, then depress it while holding B. '
                'Finish within five seconds. Release everything before the next trial.\n\n'
                'Positive key_gap means a gap from A release to B press; negative means overlap. '
                'Positive release_delta means pedal REST occurred after B press; negative means before. '
                'down_delta measures pedal DOWN relative to B. These are contact times, not acoustic damping. '
                'A simultaneous flag means ordering is unresolved within one qualified sample.\n\n'
                'A 1 ms scan and independent 8 ms contact debounce reject chatter. Short events are not observed. '
                'Calibrate mounting by observing all four contacts before each lesson. Do not obstruct keys or pedals. '
                'Keep cables clear of feet and remove the fixture if playing resistance changes.\n\n'
                'No score matching, audio recording, diagnosis, pass/fail grading, or remote firmware update. '
                'Replay never represents a live connection. A suspected reset blocks updates until an explicit new session.')
        ttk.Label(setup, text=text, wraplength=950, justify='left').pack(padx=20, pady=20)
        self.root.protocol('WM_DELETE_WINDOW', self.close)
        self.job = self.root.after(20, self.poll)
        self.render()

    def disconnect(self):
        if self.serial:
            self.serial.close()
            self.serial = None
        self.message = 'Disconnected; retained evidence is not live.'
        self.render()

    def connect(self):
        self.disconnect()
        try:
            port = Serial(self.path.get())
        except (OSError, termios.error) as error:
            self.message = 'Cannot open port: ' + str(error)
            self.render()
            return
        self.serial = port
        self.model = Model()
        self.model.source = 'live serial: ' + self.path.get()
        self.message = 'Connected; waiting for compatible PW1 frames.'
        self.render()

    def replay(self, path):
        self.disconnect()
        # Bound memory and reject huge files before replacing retained evidence.
        if Path(path).stat().st_size > 2_000_000:
            raise ValueError('replay limit is 2 MB')
        data = Path(path).read_bytes()
        self.model = Model()
        self.model.source = 'replay: ' + str(path)
        self.model.feed(data, time.monotonic())
        self.message = 'Replay only — not a live device.'
        self.render()

    def replay_dialog(self):
        path = filedialog.askopenfilename()
        if path:
            try:
                self.replay(path)
            except (OSError, ValueError) as error:
                messagebox.showerror('Replay', str(error))

    def export_dialog(self):
        path = filedialog.asksaveasfilename(defaultextension='.csv')
        if path:
            try:
                self.model.export(path, self.serial is not None, time.monotonic())
            except (OSError, ValueError) as error:
                messagebox.showerror('Export', str(error))

    def render(self):
        state = 'LIVE' if self.serial else 'OFFLINE'
        if self.model.stale(time.monotonic()):
            state += ' / STALE'
        if self.model.blocked:
            state += ' / RESET SUSPECTED — start new session'
        self.status.set(f'{state} | {self.message} | errors={self.model.errors} missing={self.model.missed}\n{self.model.source}')
        row = self.model.latest
        if row:
            self.summary.set(PHASES[row['phase']])
            self.contacts.set('  '.join(f'{name}: {"closed" if row["levels"] & (1 << i) else "open"}'
                                       for i, name in enumerate(('Key A', 'Key B', 'Pedal REST', 'Pedal DOWN'))))
            self.metrics.set(f'Completed {row["completed"]}; rejected {row["rejected"]}; gaps {row["gaps"]}; '
                             f'transport losses {row["transport_losses"]}; flags {row["flags"]}.\n'
                             'Read intervals in the notebook; completed means observed, not musically correct.')
        else:
            self.summary.set('No qualified measurements')
            self.contacts.set('')
            self.metrics.set('')
        self.table.delete(*self.table.get_children())
        for record in self.model.history:
            self.table.insert('', 'end', values=[record[key] for key in self.table['columns']])

    def poll(self):
        if self.serial:
            try:
                data = self.serial.read()
                self.model.feed(data, time.monotonic())
            except (OSError, termios.error) as error:
                self.disconnect()
                self.message = 'Port lost: ' + str(error)
        self.render()
        self.job = self.root.after(20, self.poll)

    def close(self):
        self.root.after_cancel(self.job)
        self.disconnect()
        self.root.destroy()


if __name__ == '__main__':
    window = tk.Tk()
    App(window)
    window.mainloop()
