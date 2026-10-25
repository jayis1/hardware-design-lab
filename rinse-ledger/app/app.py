#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Rinse Ledger: Linux/Tk offline basket and water-batch notebook."""
import csv
import os
from pathlib import Path
import re
import select
import termios
import time
import tkinter as tk
from tkinter import ttk, filedialog, messagebox

FIELDS = ('seq', 'ms', 'ready', 'levels', 'epoch', 'rejected', 'gaps', 'losses',
          'debounce', 'max_gap') + tuple(f'{b}_{k}' for b in 'abc' for k in
          ('total', 'epoch_ms', 'streak', 'longest', 'entries', 'baths', 'flags'))
COUNTERS = ('epoch', 'rejected', 'gaps', 'losses') + tuple(f'{b}_{k}' for b in 'abc'
          for k in ('total', 'longest', 'entries', 'baths'))


def crc(data):
    value = 0xffff
    for byte in data:
        value ^= byte << 8
        for _ in range(8):
            value = ((value << 1) ^ (0x1021 if value & 0x8000 else 0)) & 0xffff
    return value


def parse(line):
    if len(line) > 512:
        raise ValueError('frame too long')
    match = re.fullmatch(rb'(RL1,[0-9,]+)\*([0-9A-F]{4})\n', line)
    if not match or crc(match[1]) != int(match[2], 16):
        raise ValueError('framing or CRC')
    words = match[1].split(b',')[1:]
    if len(words) != len(FIELDS) or any(not word or len(word) > 10 for word in words):
        raise ValueError('field count or length')
    values = [int(word) for word in words]
    if any(value > 0xffffffff for value in values):
        raise ValueError('range')
    row = dict(zip(FIELDS, values))
    if row['ready'] > 1 or row['levels'] > 15:
        raise ValueError('enum')
    if (row['debounce'], row['max_gap']) != (8, 4):
        raise ValueError('unsupported policy')
    for b in 'abc':
        total, epoch, streak, longest, entries, baths, flags = (
            row[f'{b}_{k}'] for k in ('total', 'epoch_ms', 'streak', 'longest', 'entries', 'baths', 'flags'))
        if flags > 3 or epoch > total or streak > longest or longest > total:
            raise ValueError('duration invariant')
        if baths > row['epoch'] or baths > entries:
            raise ValueError('bath invariant')
        if bool(baths) != bool(entries) or (not baths and total):
            raise ValueError('exposure without a declared bath and entry')
    return row


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

    def stale(self, now):
        return self.last_seen is None or now - self.last_seen > 1.0

    def feed(self, data, now):
        # The live reader supplies at most 4096 bytes per UI tick.
        self.pending += data
        while self.pending:
            start = self.pending.find(b'RL1,')
            if start < 0:
                if len(self.pending) > 512:
                    self.errors += 1
                    self.pending = self.pending[-3:]
                return
            if start:
                self.errors += 1
                self.pending = self.pending[start:]
            newline = self.pending.find(b'\n')
            restart = self.pending.find(b'RL1,', 4)
            if restart >= 0 and (newline < 0 or restart < newline):
                self.errors += 1
                self.pending = self.pending[restart:]
                continue
            if newline < 0:
                if len(self.pending) > 512:
                    self.errors += 1
                    self.pending = self.pending[-3:]
                return
            line, self.pending = self.pending[:newline+1], self.pending[newline+1:]
            try:
                record = parse(line)
            except ValueError:
                self.errors += 1
                continue
            self.accept(record, now)

    def accept(self, record, now):
        if self.blocked:
            return
        if self.latest:
            old = self.latest
            delta = (record['seq'] - old['seq']) & 0xffffffff
            clock = (record['ms'] - old['ms']) & 0xffffffff
            rollback = any(record[k] < old[k] for k in COUNTERS)
            vanished = any(old[f'{b}_flags'] & ~record[f'{b}_flags'] for b in 'abc')
            same_bath_rollback = record['epoch'] == old['epoch'] and any(
                record[f'{b}_epoch_ms'] < old[f'{b}_epoch_ms'] for b in 'abc')
            impossible_time = any(record[f'{b}_total'] - old[f'{b}_total'] > clock
                                  for b in 'abc')
            if (delta >= 0x80000000 or clock >= 0x80000000 or rollback or vanished
                    or same_bath_rollback or impossible_time):
                self.blocked = True
                return
            if not clock:
                if any(record[k] != old[k] for k in FIELDS if k != 'seq'):
                    self.blocked = True
                return
            if not delta:
                return
            self.missed += delta - 1
        self.latest = record
        self.last_seen = now
        self.history.append(record.copy())
        self.history = self.history[-256:]

    def export(self, path, connected, now):
        # Private from creation, not a chmod after potentially public creation.
        fd = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
        with os.fdopen(fd, 'w', newline='', encoding='utf-8') as stream:
            writer = csv.writer(stream)
            age = '' if self.last_seen is None else max(0, now-self.last_seen)
            writer.writerow(['author', 'jayis1', 'source', self.source,
                             'connected', connected, 'stale', self.stale(now), 'age_s', age,
                             'blocked', self.blocked, 'parser_errors', self.errors,
                             'missing_frames', self.missed, 'retained_rows', len(self.history)])
            writer.writerow(FIELDS)
            for row in self.history:
                writer.writerow([row[k] for k in FIELDS])


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
            attrs[6][termios.VMIN] = attrs[6][termios.VTIME] = 0
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
        self.message = 'No device. No chemical cleanliness is inferred.'
        root.title('Rinse Ledger — jayis1')
        root.geometry('1100x650')
        self.path = tk.StringVar(value='/dev/ttyACM0')
        bar = ttk.Frame(root)
        bar.pack(fill='x')
        ttk.Entry(bar, textvariable=self.path, width=25).pack(side='left')
        for text, command in [('Connect / new session', self.connect), ('Disconnect', self.disconnect),
                              ('Replay', self.replay_dialog), ('Export CSV', self.export_dialog)]:
            ttk.Button(bar, text=text, command=command).pack(side='left')
        self.status = tk.StringVar()
        ttk.Label(root, textvariable=self.status, wraplength=1080).pack(fill='x', pady=8)
        self.tabs = ttk.Notebook(root)
        self.tabs.pack(fill='both', expand=True)
        live, history, guide = [ttk.Frame(self.tabs) for _ in range(3)]
        for panel, name in [(live, 'Basket exposure'), (history, 'Water-batch notebook'), (guide, 'Setup and limits')]:
            self.tabs.add(panel, text=name)
        self.summary = tk.StringVar(value='No qualified measurements')
        ttk.Label(live, textvariable=self.summary, font=('TkDefaultFont', 13), wraplength=1050).pack(pady=25)
        columns = ('seq', 'ms', 'epoch', 'a_total', 'a_epoch_ms', 'b_total', 'b_epoch_ms', 'c_total', 'c_epoch_ms')
        self.table = ttk.Treeview(history, columns=columns, show='headings')
        for name in columns:
            self.table.heading(name, text=name)
            self.table.column(name, width=110)
        self.table.pack(fill='both', expand=True)
        ttk.Label(guide, wraplength=1000, justify='left', text=(
            'Author: jayis1\n\nJ3/J4/J5 = baskets A/B/C lowered; J6 = water-replaced marker. '
            'Use only independent voltage-free dry contacts. Keep electronics and USB away from liquids.\n\n'
            'Start with all baskets raised and button released. Replace water with every basket out, '
            'then press and release the marker. A marker with a basket in, or coincident with extraction, is rejected. '
            'The device records a declaration, not water quality. Keep the same material in each basket for the entire session.\n\n'
            'After a sampling gap, all contacts must return open to resume. Flag 1 means incomplete observation; '
            'flag 2 means saturation. Neither clears without hardware reset. Reconnect resets only desktop history. '
            'A reset clears device totals. Export before reset. Only the last 256 received snapshots are retained. '
            'No archive-safe, potable, sterile or chemically clean claim is made.')).pack(padx=20, pady=20)
        root.protocol('WM_DELETE_WINDOW', self.close)
        self.timer = root.after(50, self.poll)
        self.render()

    def disconnect(self):
        if self.serial:
            self.serial.close()
            self.serial = None
        self.message = 'Disconnected; retained evidence has not been erased.'
        self.render()

    def connect(self):
        self.disconnect()
        try:
            self.serial = Serial(self.path.get())
        except (OSError, termios.error) as exc:
            self.message = f'Connection failed: {exc}'
        else:
            self.model = Model()
            self.model.source = 'live'
            self.message = 'Connected; awaiting advancing device timestamps.'
        self.render()

    def replay(self, path):
        # Read a bounded payload before replacing the current evidence.
        with open(path, 'rb') as source:
            data = source.read(1024*1024 + 1)
        if len(data) > 1024*1024:
            raise ValueError('Replay limit is 1 MiB')
        self.disconnect()
        self.model = Model()
        self.model.source = 'replay'
        for offset in range(0, len(data), 4096):
            self.model.feed(data[offset:offset+4096], time.monotonic())
        self.message = 'OFFLINE replay: not a live water measurement.'
        self.render()

    def replay_dialog(self):
        path = filedialog.askopenfilename()
        if path:
            try:
                self.replay(Path(path))
            except (OSError, ValueError) as exc:
                messagebox.showerror('Replay failed', str(exc))

    def export_dialog(self):
        path = filedialog.asksaveasfilename(defaultextension='.csv')
        if path:
            try:
                self.model.export(path, self.serial is not None, time.monotonic())
            except OSError as exc:
                messagebox.showerror('Export refused', str(exc))

    def render(self):
        m = self.model
        self.status.set(f'{self.message} Source={m.source}; connected={self.serial is not None}; '
                        f'stale={m.stale(time.monotonic())}; reset-blocked={m.blocked}; '
                        f'parser errors={m.errors}; missing frames={m.missed}')
        if m.latest:
            r = m.latest
            lines = [f'Water batch {r["epoch"]}; ready={r["ready"]}; contacts={r["levels"]:04b}; '
                     f'rejected marks={r["rejected"]}; acquisition gaps={r["gaps"]}; TX losses={r["losses"]}']
            for b in 'abc':
                lines.append(f'{b.upper()}: total {r[b+"_total"]/1000:.3f}s; this bath {r[b+"_epoch_ms"]/1000:.3f}s; '
                             f'streak {r[b+"_streak"]/1000:.3f}s; longest {r[b+"_longest"]/1000:.3f}s; '
                             f'entries {r[b+"_entries"]}; baths visited {r[b+"_baths"]}; flags {r[b+"_flags"]}')
            self.summary.set('\n\n'.join(lines))
        else:
            self.summary.set('No qualified measurements')
        self.table.delete(*self.table.get_children())
        for row in m.history:
            self.table.insert('', 'end', values=[row[k] for k in self.table['columns']])

    def poll(self):
        if self.serial:
            try:
                self.model.feed(self.serial.read(), time.monotonic())
            except (OSError, termios.error) as exc:
                self.disconnect()
                self.message = f'Serial lost: {exc}'
        self.render()
        self.timer = self.root.after(50, self.poll)

    def close(self):
        self.root.after_cancel(self.timer)
        if self.serial:
            self.serial.close()
        self.root.destroy()


if __name__ == '__main__':
    window = tk.Tk()
    App(window)
    window.mainloop()
