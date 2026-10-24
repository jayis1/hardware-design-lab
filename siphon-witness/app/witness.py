#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Siphon Witness: local Linux serial/replay desktop application."""
import collections
import csv
import os
from pathlib import Path
import stat
import termios
import time
import tkinter as tk
from tkinter import filedialog, messagebox, ttk

FIELDS = ('seq', 'ms', 'trial', 'phase', 'mask', 'flags', 'elapsed_ms',
          'drain_ms', 'fill_ms', 'cycles', 'failed', 'gaps', 'drops')
PHASES = ('settling', 'ready', 'armed', 'transfer', 'complete', 'fault')
FLAGS = {1: 'impossible float order', 2: 'reverse movement', 4: 'timeout',
         8: 'sampling gap', 16: 'skipped band', 32: 'transmit loss'}
UNKNOWN = 4294967295

def crc16(data):
    crc = 0xffff
    for byte in data:
        crc ^= byte << 8
        for _ in range(8):
            crc = ((crc << 1) ^ (0x1021 if crc & 0x8000 else 0)) & 0xffff
    return crc

class Decoder:
    """Bounded framing, including resync after a partial transmitter timeout."""
    def __init__(self):
        self.buffer = bytearray()
        self.errors = 0

    def feed(self, data):
        rows = []
        for byte in data:
            self.buffer.append(byte)
            if self.buffer.endswith(b'SW1,') and len(self.buffer) > 4:
                self.buffer = bytearray(b'SW1,')
                self.errors += 1
            if len(self.buffer) > 256:
                self.buffer.clear()
                self.errors += 1
            if byte != 10:
                continue
            line = bytes(self.buffer).strip()
            self.buffer.clear()
            try:
                body, checksum = line.rsplit(b'*', 1)
                if len(checksum) != 4 or int(checksum, 16) != crc16(body):
                    raise ValueError('CRC')
                words = body.decode('ascii').split(',')
                if len(words) != 14 or words[0] != 'SW1':
                    raise ValueError('version or field count')
                if not all(w.isascii() and w.isdecimal() for w in words[1:]):
                    raise ValueError('nondecimal')
                values = [int(w) for w in words[1:]]
                if any(v > UNKNOWN for v in values):
                    raise ValueError('range')
                row = dict(zip(FIELDS, values))
                if row['phase'] > 5 or row['mask'] not in (*range(16), 255) or row['flags'] > 63:
                    raise ValueError('enum')
                rows.append(row)
            except (ValueError, UnicodeError):
                self.errors += 1
        return rows

class Session:
    def __init__(self, source='unknown'):
        self.source = source
        self.rows = collections.deque(maxlen=10000)
        self.last = None
        self.received = None
        self.warning = ''
        self.lost = 0
        self.evicted = 0
        self.blocked = False

    def accept(self, row, now):
        if self.blocked:
            return False
        if self.last is not None:
            advance = (row['seq'] - self.last['seq']) & UNKNOWN
            clock = (row['ms'] - self.last['ms']) & UNKNOWN
            if not advance:
                self.warning = 'Duplicate frame ignored'
                return False
            if advance >= 0x80000000 or clock >= 0x80000000:
                self.warning = 'Possible device restart: reconnect required'
                self.blocked = True
                return False
            self.lost += advance - 1
        if len(self.rows) == self.rows.maxlen:
            self.evicted += 1
        saved = dict(row, host_received=now)
        self.rows.append(saved)
        self.last = row
        self.received = now
        return True

    def fresh(self, now):
        return not self.blocked and self.received is not None and now - self.received <= 1.5

    def export(self, path, mode, now, parse_errors=0):
        # Exclusive create protects prior evidence; restrictive mode is deliberate.
        fd = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
        with os.fdopen(fd, 'w', newline='') as output:
            writer = csv.writer(output)
            writer.writerow(['author', 'source', 'connection', 'fresh', 'parse_errors',
                             'blocked', 'warning', 'age_seconds', 'lost_frames',
                             'evicted_rows', *FIELDS])
            for row in self.rows:
                writer.writerow(['jayis1', self.source, mode,
                                 mode == 'live' and self.fresh(now), parse_errors,
                                 self.blocked, self.warning, f"{max(0, now-row['host_received']):.3f}",
                                 self.lost, self.evicted, *(row[k] for k in FIELDS)])

class Serial:
    def __init__(self, path):
        self.fd = os.open(path, os.O_RDONLY | os.O_NONBLOCK | os.O_NOCTTY)
        try:
            if not stat.S_ISCHR(os.fstat(self.fd).st_mode) or not os.isatty(self.fd):
                raise ValueError('Select a serial TTY, not a regular file')
            attrs = termios.tcgetattr(self.fd)
            self.original = attrs
            raw = termios.tcgetattr(self.fd)
            raw[0] = 0
            raw[1] = 0
            raw[2] = termios.CS8 | termios.CREAD | termios.CLOCAL
            raw[3] = 0
            raw[4] = termios.B115200
            raw[5] = termios.B115200
            raw[6][termios.VMIN] = 1
            raw[6][termios.VTIME] = 0
            termios.tcsetattr(self.fd, termios.TCSANOW, raw)
            termios.tcflush(self.fd, termios.TCIFLUSH)
        except Exception:
            os.close(self.fd)
            raise

    def read(self):
        if self.fd is None:
            raise OSError('Serial port is closed')
        try:
            data = os.read(self.fd, 4096)
        except BlockingIOError:
            return b''
        if not data:
            raise OSError('Serial device disconnected')
        return data

    def close(self):
        if self.fd is not None:
            try:
                termios.tcsetattr(self.fd, termios.TCSANOW, self.original)
            except termios.error:
                pass
            os.close(self.fd)
            self.fd = None

class App:
    def __init__(self, root):
        self.root = root
        self.root.title('Siphon Witness — jayis1')
        self.root.geometry('860x570')
        self.serial = None
        self.mode = 'disconnected'
        self.decoder = Decoder()
        self.session = Session()
        self.status = tk.StringVar(value='Disconnected; no measurements')
        self.summary = tk.StringVar(value='No evidence loaded')
        self.port = tk.StringVar(value='/dev/ttyACM0')
        book = ttk.Notebook(root)
        book.pack(fill='both', expand=True)
        connection = ttk.Frame(book, padding=16)
        monitor = ttk.Frame(book, padding=16)
        evidence = ttk.Frame(book, padding=16)
        for page, label in [(connection, 'Connect / Replay'), (monitor, 'Live trial'), (evidence, 'Evidence')]:
            book.add(page, text=label)
        ttk.Label(connection, text='Read-only 115200 8N1. No pump or valve commands.').pack(anchor='w')
        ttk.Entry(connection, textvariable=self.port, width=60).pack(anchor='w', pady=12)
        ttk.Button(connection, text='Connect (new session)', command=self.connect).pack(anchor='w')
        ttk.Button(connection, text='Disconnect', command=self.disconnect).pack(anchor='w', pady=8)
        ttk.Button(connection, text='Open captured SW1 stream', command=self.replay).pack(anchor='w')
        ttk.Label(connection, text='New connection/replay clears the prior session. Export first.\nReplay is synthetic or captured evidence, never a live reading.').pack(anchor='w', pady=20)
        ttk.Label(monitor, textvariable=self.summary, justify='left', font=('TkDefaultFont', 13)).pack(anchor='w')
        ttk.Label(monitor, text='UNKNOWN is not zero. A passing trial is only a threshold sequence,\nnot proof of flow rate, leak tightness or reliable unattended operation.').pack(anchor='w', pady=25)
        self.tree = ttk.Treeview(evidence, columns=('trial','phase','drain','fill','flags'), show='headings', height=15)
        for key in self.tree['columns']:
            self.tree.heading(key, text=key)
            self.tree.column(key, width=140)
        self.tree.pack(fill='both', expand=True)
        ttk.Button(evidence, text='Export retained rows (new CSV only)', command=self.export).pack(pady=10)
        ttk.Label(root, textvariable=self.status).pack(fill='x', padx=16, pady=10)
        self.root.protocol('WM_DELETE_WINDOW', self.close)
        self.timer = self.root.after(50, self.poll)

    def disconnect(self):
        if self.serial is not None:
            self.serial.close()
            self.serial = None
        self.mode = 'disconnected'
        self.render()

    def reset(self, mode):
        self.disconnect()
        self.mode = mode
        self.decoder = Decoder()
        self.session = Session(mode)
        for child in self.tree.get_children():
            self.tree.delete(child)
        self.summary.set('No evidence loaded')

    def connect(self):
        self.reset('live')
        try:
            self.serial = Serial(self.port.get())
            self.mode = 'live'
        except (OSError, ValueError, termios.error) as error:
            self.mode = 'disconnected'
            self.status.set(str(error))
            return
        self.render()

    def load_replay(self, path):
        if Path(path).stat().st_size > 2_000_000:
            raise ValueError('Replay limit: 2 MB')
        data = Path(path).read_bytes()
        if len(data) > 2_000_000:
            raise ValueError('Replay grew beyond 2 MB')
        self.reset('replay')
        self.ingest(data)
        self.render()

    def replay(self):
        path = filedialog.askopenfilename(title='Select SW1 stream')
        if not path:
            return
        try:
            self.load_replay(path)
        except (OSError, ValueError) as error:
            messagebox.showerror('Replay failed', str(error))

    def ingest(self, data):
        for row in self.decoder.feed(data):
            if not self.session.accept(row, time.monotonic()):
                continue
            self.tree.insert('', 'end', values=(row['trial'], PHASES[row['phase']],
                             self.duration(row['drain_ms']), self.duration(row['fill_ms']), row['flags']))
            children = self.tree.get_children()
            if len(children) > 200:
                self.tree.delete(children[0])

    @staticmethod
    def duration(value):
        return 'unknown' if value == UNKNOWN else f'{value} ms'

    def render(self):
        now = time.monotonic()
        fresh = self.mode == 'live' and self.session.fresh(now)
        quality = 'LIVE' if fresh else ('REPLAY' if self.mode == 'replay' else 'STALE / DISCONNECTED')
        self.status.set(f'{quality} | parse errors {self.decoder.errors} | lost {self.session.lost} | {self.session.warning}')
        row = self.session.last
        if row is None:
            self.summary.set('No evidence loaded')
            return
        errors = ', '.join(label for bit, label in FLAGS.items() if row['flags'] & bit) or 'none'
        self.summary.set(f"{quality}\nTrial {row['trial']}: {PHASES[row['phase']]}\n"
                         f"Float mask: {row['mask']} (255 = unqualified)\n"
                         f"Drain band: {self.duration(row['drain_ms'])}\n"
                         f"Fill band: {self.duration(row['fill_ms'])}\n"
                         f"Elapsed from arm: {row['elapsed_ms']} ms\n"
                         f"Completed: {row['cycles']}  Failed: {row['failed']}\n"
                         f"Sample gaps: {row['gaps']}  TX drops: {row['drops']}\nFlags: {errors}")

    def export(self):
        path = filedialog.asksaveasfilename(defaultextension='.csv')
        if not path:
            return
        try:
            self.session.export(path, self.mode, time.monotonic(), self.decoder.errors)
        except OSError as error:
            messagebox.showerror('Export failed (existing files are protected)', str(error))

    def poll(self):
        if self.serial is not None:
            try:
                self.ingest(self.serial.read())
            except OSError as error:
                self.disconnect()
                self.session.warning = str(error)
        self.render()
        self.timer = self.root.after(50, self.poll)

    def close(self):
        self.root.after_cancel(self.timer)
        self.disconnect()
        self.root.destroy()

if __name__ == '__main__':
    root = tk.Tk()
    App(root)
    root.mainloop()
