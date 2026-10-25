#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Nest Settle local Linux workbench. No network or third-party packages."""
import binascii
import csv
import os
from pathlib import Path
import select
import stat
import termios
import time
import tkinter as tk
from tkinter import ttk, filedialog, messagebox

FIELDS = ('time state mask gaps tx id reason duration first_settle full_ms '
          'longest drops drop_mask seen').split()
STATES = ['WAIT EMPTY', 'READY', 'ACTIVE', 'FAULT']
REASONS = ['NONE', 'SETTLED', 'UNSETTLED', 'TIMEOUT', 'GAP']
RESULT_FIELDS = FIELDS[5:]
MAX_FILE = 1024 * 1024


def decode(line):
    if len(line) > 255:
        raise ValueError('frame too long')
    body, checksum = line.rsplit(b'*', 1)
    if len(checksum) != 4 or int(checksum, 16) != binascii.crc_hqx(body, 0xffff):
        raise ValueError('CRC mismatch')
    parts = body.split(b',')
    if parts[0] != b'NS1' or len(parts) != 15:
        raise ValueError('unsupported version/field count')
    if any(not x or not x.isdigit() or len(x) > 10 for x in parts[1:]):
        raise ValueError('noncanonical integer')
    d = dict(zip(FIELDS, map(int, parts[1:])))
    if any(x > 0xffffffff for x in d.values()):
        raise ValueError('integer overflow')
    if d['state'] > 3 or d['mask'] > 15 or d['reason'] > 4:
        raise ValueError('invalid state')
    if d['state'] == 1 and d['mask'] or d['state'] == 2 and not d['mask']:
        raise ValueError('state/contacts disagree')
    if not d['id']:
        if any(d[k] for k in RESULT_FIELDS[1:]):
            raise ValueError('result without identity')
        return d
    if not d['reason'] or not 0 < d['duration'] <= 30000:
        raise ValueError('invalid result')
    if not 0 < d['seen'] <= 15 or d['drop_mask'] > 15:
        raise ValueError('invalid corner mask')
    if not 0 <= d['longest'] <= d['full_ms'] <= d['duration']:
        raise ValueError('impossible dwell')
    if d['first_settle'] and not 250 <= d['first_settle'] <= d['duration']:
        raise ValueError('impossible settling time')
    if d['first_settle'] and (d['longest'] < 250 or d['seen'] != 15):
        raise ValueError('settling without full seating')
    if d['reason'] == 1 and not d['first_settle']:
        raise ValueError('success without settling')
    if d['reason'] == 2 and d['first_settle']:
        raise ValueError('unsettled with settling')
    if d['reason'] == 3 and d['duration'] != 30000:
        raise ValueError('early timeout')
    if d['drops'] > 4 * d['duration'] or d['drops'] < d['drop_mask'].bit_count():
        raise ValueError('invalid dropout count')
    if bool(d['drops']) != bool(d['drop_mask']):
        raise ValueError('dropout mask mismatch')
    if d['drops'] and d['seen'] != 15:
        raise ValueError('dropout before first full seating')
    return d


class Model:
    def __init__(self, source):
        self.source = source
        self.buffer = b''
        self.errors = 0
        self.blocked = False
        self.warning = ''
        self.latest = None
        self.received = None
        self.rows = []
        self.missed = 0
        self.evicted = 0
        self.connected = source == 'live'

    def fresh(self, now):
        return (self.connected and not self.blocked and self.received is not None
                and now - self.received <= 1.0)

    def accept(self, d, now):
        if self.blocked:
            return
        if self.source == 'live' and self.received is not None and now - self.received > 1.0:
            self.blocked = True
            self.warning = 'Host reception gap; buffered data cannot renew freshness. Reconnect.'
            return
        old = self.latest
        if old:
            delta = (d['time'] - old['time']) & 0xffffffff
            if not delta:
                if d != old:
                    self.blocked = True
                    self.warning = 'Contradictory same-time snapshot; reconnect'
                return
            rollback = delta >= 0x80000000 or any(d[k] < old[k] for k in ('id', 'gaps', 'tx'))
            changed = (d['id'] == old['id'] and
                       any(d[k] != old[k] for k in RESULT_FIELDS))
            if rollback or changed:
                self.blocked = True
                self.warning = 'Reset or contradictory repeated result; reconnect'
                return
        if d['id'] and (old is None or d['id'] != old['id']):
            last = old['id'] if old else 0
            self.missed += max(0, d['id'] - last - 1)
            self.rows.append(d.copy())
            if len(self.rows) > 500:
                self.rows.pop(0)
                self.evicted += 1
        self.latest = d
        self.received = now

    def feed(self, data, now):
        # Consume bounded chunks even when callers provide an entire recording.
        for offset in range(0, len(data), 256):
            self.buffer += data[offset:offset + 256]
            while b'\n' in self.buffer:
                line, self.buffer = self.buffer.split(b'\n', 1)
                marker = line.rfind(b'NS1,')
                if marker > 0:
                    self.errors += 1
                    line = line[marker:]
                try:
                    self.accept(decode(line), now)
                except (ValueError, UnicodeError):
                    self.errors += 1
            if len(self.buffer) > 255:
                self.errors += 1
                marker = self.buffer.rfind(b'NS1,')
                self.buffer = self.buffer[marker:] if marker > 0 else b''
                if len(self.buffer) > 255:
                    self.buffer = b''

    def finish(self):
        if self.buffer:
            self.errors += 1
            self.buffer = b''
        if self.latest is None:
            self.warning = 'No valid snapshots'


def load_replay(path):
    fd = os.open(path, os.O_RDONLY | os.O_NONBLOCK)
    try:
        info = os.fstat(fd)
        if not stat.S_ISREG(info.st_mode) or info.st_size > MAX_FILE:
            raise ValueError('Replay must be a regular file of at most 1 MiB')
        with os.fdopen(fd, 'rb', closefd=False) as source:
            data = source.read(MAX_FILE + 1)
        if len(data) > MAX_FILE:
            raise ValueError('Replay grew beyond limit')
    finally:
        os.close(fd)
    m = Model('replay')
    m.feed(data, time.monotonic())
    m.finish()
    return m


def export(model, path, now):
    # Exclusive create prevents silent evidence replacement; private by default.
    fd = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
    with os.fdopen(fd, 'w', newline='') as output:
        w = csv.writer(output)
        w.writerow(['author', 'source', 'connected', 'fresh', 'errors', 'blocked',
                    'missed', 'evicted', 'warning'])
        w.writerow(['jayis1', model.source, model.connected, model.fresh(now),
                    model.errors, model.blocked, model.missed, model.evicted, model.warning])
        w.writerow(FIELDS)
        for row in model.rows:
            w.writerow([row[k] for k in FIELDS])


class Serial:
    def __init__(self, path):
        self.fd = os.open(path, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
        self.saved = None
        try:
            self.saved = termios.tcgetattr(self.fd)
            attrs = termios.tcgetattr(self.fd)
            attrs[0] = 0
            attrs[1] = 0
            attrs[2] = termios.CS8 | termios.CREAD | termios.CLOCAL
            attrs[3] = 0
            attrs[4] = attrs[5] = termios.B115200
            attrs[6][termios.VMIN] = 0
            attrs[6][termios.VTIME] = 0
            termios.tcsetattr(self.fd, termios.TCSANOW, attrs)
            termios.tcflush(self.fd, termios.TCIFLUSH)
            self.poll = select.poll()
            self.poll.register(self.fd, select.POLLIN | select.POLLHUP | select.POLLERR)
        except (OSError, termios.error):
            self.close()
            raise

    def read(self):
        if self.fd is None:
            raise OSError('Serial port is closed')
        data = bytearray()
        hungup = False
        for _, events in self.poll.poll(0):
            if events & select.POLLIN:
                try:
                    data.extend(os.read(self.fd, 4096))
                except BlockingIOError:
                    pass
            hungup = bool(events & (select.POLLHUP | select.POLLERR | select.POLLNVAL))
        return bytes(data), hungup

    def close(self):
        if self.fd is None:
            return
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
        root.title('Nest Settle — jayis1')
        self.serial = None
        self.model = Model('none')
        self.port = tk.StringVar(value='/dev/ttyACM0')
        self.status = tk.StringVar(value='Disconnected')
        self.live = tk.StringVar(value='No samples')
        self.policy = tk.StringVar(value='500')
        self.comparison = tk.StringVar()
        top = ttk.Frame(root); top.pack(fill='x')
        ttk.Entry(top, textvariable=self.port, width=35).pack(side='left')
        ttk.Button(top, text='Connect (new session)', command=self.connect).pack(side='left')
        ttk.Button(top, text='Disconnect', command=self.disconnect).pack(side='left')
        ttk.Button(top, text='Replay', command=self.replay).pack(side='left')
        ttk.Button(top, text='Export CSV', command=self.save).pack(side='left')
        ttk.Label(root, textvariable=self.status, wraplength=900).pack(fill='x')
        self.tabs = ttk.Notebook(root); self.tabs.pack(fill='both', expand=True)
        live = ttk.Frame(self.tabs); history = ttk.Frame(self.tabs); policy = ttk.Frame(self.tabs)
        for widget, title in ((live, 'Live seating'), (history, 'Trial history'), (policy, 'Comparison')):
            self.tabs.add(widget, text=title)
        ttk.Label(live, textvariable=self.live, font=('TkDefaultFont', 16)).pack(padx=20, pady=30)
        ttk.Label(live, text='Qualified contact evidence only. Not a load, gap or safety measurement.').pack()
        columns = ['id', 'reason', 'duration', 'first_settle', 'full_ms', 'longest', 'drops', 'drop_mask']
        self.tree = ttk.Treeview(history, columns=columns, show='headings', height=15)
        for name in columns:
            self.tree.heading(name, text=name); self.tree.column(name, width=100)
        self.tree.pack(fill='both', expand=True)
        ttk.Label(policy, text='Maximum first-settle delay (ms). Local comparison only; firmware remains at 250 ms dwell.').pack()
        ttk.Entry(policy, textvariable=self.policy).pack()
        ttk.Button(policy, text='Compare retained trials', command=self.compare).pack()
        ttk.Label(policy, textvariable=self.comparison).pack()
        root.protocol('WM_DELETE_WINDOW', self.close)
        self.after = root.after(50, self.tick)

    def disconnect(self):
        if self.serial:
            self.serial.close(); self.serial = None
        self.model.connected = False
        self.render()

    def connect(self):
        self.disconnect()
        try:
            connection = Serial(self.port.get())
        except (OSError, termios.error) as error:
            self.status.set('Connection failed: ' + str(error)); return
        self.serial = connection
        self.model = Model('live')
        self.comparison.set('')
        self.render()

    def replay(self):
        path = filedialog.askopenfilename()
        if not path:
            return
        try:
            model = load_replay(path)
        except (OSError, ValueError) as error:
            messagebox.showerror('Replay', str(error)); return
        self.disconnect()
        self.model = model
        self.comparison.set('')
        self.render()

    def save(self):
        path = filedialog.asksaveasfilename(defaultextension='.csv')
        if path:
            try:
                export(self.model, path, time.monotonic())
            except OSError as error:
                messagebox.showerror('Export', str(error))

    def compare(self):
        try:
            threshold = int(self.policy.get())
            if not 250 <= threshold <= 30000:
                raise ValueError()
        except ValueError:
            self.comparison.set('Enter an integer from 250 to 30000'); return
        accepted = sum(r['reason'] == 1 and r['first_settle'] <= threshold and not r['drops']
                       for r in self.model.rows)
        self.comparison.set(f'{accepted} / {len(self.model.rows)} retained trials settled within limit with no partial dropouts. Not a safety verdict.')

    def render(self):
        m = self.model
        condition = 'LIVE' if m.fresh(time.monotonic()) else ('STALE' if m.connected else 'OFFLINE')
        self.status.set(f'{m.source} / {condition}; errors={m.errors}; missed={m.missed}; evicted={m.evicted}; {m.warning}')
        if m.latest:
            d = m.latest
            bits = '  '.join(f'{name}:{"SEATED" if d["mask"] & (1 << i) else "open"}' for i, name in enumerate('ABCD'))
            self.live.set(f'{STATES[d["state"]]}\n{bits}\nDevice ms: {d["time"]}; acquisition gaps: {d["gaps"]}; TX loss: {d["tx"]}')
        else:
            self.live.set('No samples')
        children = self.tree.get_children()
        if children:
            self.tree.delete(*children)
        for row in m.rows:
            self.tree.insert('', 'end', values=[REASONS[row[k]] if k == 'reason' else row[k] for k in self.tree['columns']])

    def tick(self):
        if self.serial:
            try:
                data, hungup = self.serial.read()
                self.model.feed(data, time.monotonic())
                if hungup:
                    self.model.warning = 'Serial disconnected'
                    self.disconnect()
            except (OSError, termios.error) as error:
                self.model.warning = str(error); self.disconnect()
        self.render()
        self.after = self.root.after(50, self.tick)

    def close(self):
        self.root.after_cancel(self.after)
        self.disconnect()
        self.root.destroy()


if __name__ == '__main__':
    root = tk.Tk()
    App(root)
    root.mainloop()
