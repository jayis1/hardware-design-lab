#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Local Linux desktop companion; no dependency packages or remote services."""
import binascii
import csv
import glob
import os
from pathlib import Path
import re
import select
import stat
import termios
import time
import tkinter as tk
from tkinter import filedialog, messagebox, ttk
from typing import Any

FIELDS = ('time state raw follower travel completed rejected gaps reason '
          'duration closed transitions tx_errors').split()
STATES = ('WAIT FOR INDEX', 'ACQUIRING TURN', 'TERMINAL FAULT')
REASONS = ('none', 'reverse', 'invalid quadrature', 'sampling gap',
           'wrong turn length', 'timeout', 'exhaustion / startup fault')
LIMIT = 2 * 1024 * 1024

def parse(line):
    """CRC authenticates neither the sender nor the physical measurement."""
    try:
        body, crc = line.rstrip(b'\r\n').split(b'*')
        if not re.fullmatch(b'[0-9A-F]{4}', crc):
            raise ValueError('CRC syntax')
        if binascii.crc_hqx(body, 65535) != int(crc, 16):
            raise ValueError('CRC mismatch')
        fields = body.decode('ascii').split(',')
        if len(fields) != 15 or fields[0] != 'CDM1':
            raise ValueError('unsupported policy or field count')
        if any(not re.fullmatch(r'0|[1-9][0-9]{0,9}', x) for x in fields[1:14]):
            raise ValueError('decimal syntax')
        r: dict[str, Any] = dict(zip(FIELDS, map(int, fields[1:14])))
        if any(v > 0xffffffff for v in r.values()):
            raise ValueError('integer range')
        if not re.fullmatch('[0-9A]{40}', fields[14]):
            raise ValueError('bin range')
        r['bins'] = tuple(int(x, 16) for x in fields[14])
        if r['state'] > 2 or r['raw'] > 15 or r['follower'] > 1 or r['travel'] > 400:
            raise ValueError('state range')
        if r['reason'] > 6 or ((r['state'] == 2) != (r['reason'] == 6)):
            raise ValueError('fault consistency')
        if r['state'] == 0 and r['travel']:
            raise ValueError('idle travel')
        if sum(r['bins']) != r['closed'] or r['transitions'] > 399:
            raise ValueError('map conservation')
        minimum = sum(0 < n < 10 for n in r['bins'])
        maximum = sum(min(9, 2 * min(n, 10-n)) for n in r['bins'])
        for a, b in zip(r['bins'], r['bins'][1:]):
            uniform = a in (0, 10) and b in (0, 10)
            minimum += uniform and a != b
            maximum += not uniform or a != b
        if not minimum <= r['transitions'] <= maximum:
            raise ValueError('changes inconsistent with angular bins')
        if not r['completed']:
            if r['closed'] or r['duration'] or r['transitions']:
                raise ValueError('result without turn')
        else:
            if not 400 <= r['duration'] < 60000:
                raise ValueError('turn duration')
            mixed = 0 < r['closed'] < 400
            if mixed != bool(r['transitions']):
                raise ValueError('transition consistency')
        return r
    except (UnicodeError, TypeError, OverflowError) as exc:
        raise ValueError('malformed frame') from exc

class Stream:
    """Bounded framing, including truncated-frame + fresh-marker recovery."""
    def __init__(self):
        self.buffer = bytearray()
        self.errors = 0

    def feed(self, data):
        records = []
        for byte in data:
            self.buffer.append(byte)
            if self.buffer.endswith(b'CDM1') and len(self.buffer) > 4:
                self.errors += 1
                self.buffer[:] = b'CDM1'
            if byte == 10:
                try:
                    records.append(parse(bytes(self.buffer)))
                except ValueError:
                    self.errors += 1
                self.buffer.clear()
            elif len(self.buffer) > 255:
                self.errors += 1
                self.buffer.clear()
        return records

    def finish(self):
        if self.buffer:
            self.buffer.clear()
            self.errors += 1
            raise ValueError('incomplete capture tail')

class Session:
    def __init__(self, source):
        self.source = source
        self.connected = source == 'live'
        self.last = None
        self.last_received = None
        self.blocked = False
        self.warning = ''
        self.history = []
        self.missed = 0
        self.anchor = None
        self.device_elapsed = 0

    def fail(self, message):
        self.blocked = True
        self.warning = message
        raise ValueError(message)

    def accept(self, r, now):
        if self.blocked:
            raise ValueError('session frozen; reconnect explicitly')
        if self.last is not None:
            if self.source == 'live' and now - self.last_received > 1:
                self.fail('live reception pause; queued data cannot prove continuity')
            delta = (r['time'] - self.last['time']) & 0xffffffff
            if not delta:
                if r != self.last:
                    self.fail('contradictory duplicate timestamp')
                return
            if delta >= 0x80000000:
                self.fail('device time rollback; possible reset')
            self.device_elapsed += delta
            if self.source == 'live':
                host_elapsed = now - self.anchor
                # 750 ms buffering allowance plus 2% oscillator/host drift.
                if host_elapsed - self.device_elapsed / 1000 > 0.75 + host_elapsed * 0.02:
                    self.fail('device progress lags host; reconnect to discard backlog')
            for key in ('completed', 'rejected', 'gaps', 'tx_errors'):
                if r[key] < self.last[key]:
                    self.fail('counter rollback; possible reset')
            if self.last['state'] == 2 and r['state'] != 2:
                self.fail('terminal fault disappeared')
            if r['completed'] - self.last['completed'] > (delta + 399) // 400:
                self.fail('too many completed turns for elapsed time')
            if self.last['state'] == 2:
                for key in ('completed', 'rejected', 'gaps', 'travel', 'follower'):
                    if r[key] != self.last[key]:
                        self.fail('acquisition changed after terminal fault')
            if r['completed'] == self.last['completed']:
                for key in ('duration', 'closed', 'transitions', 'bins'):
                    if r[key] != self.last[key]:
                        self.fail('complete map changed without new turn')
        old = self.last['completed'] if self.last else 0
        if r['completed'] > old:
            self.missed += r['completed'] - old - 1
            self.history.append(r.copy())
            self.history = self.history[-100:]
        if self.anchor is None:
            self.anchor = now
        self.last = r
        self.last_received = now

    def export(self, path, now, errors):
        # O_EXCL avoids accidental replacement; data is local and private.
        fd = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
        with os.fdopen(fd, 'w', newline='') as f:
            w = csv.writer(f)
            for key, value in [('author', 'jayis1'), ('source', self.source),
                               ('connected', self.connected), ('blocked', self.blocked),
                               ('warning', self.warning), ('parser_errors', errors),
                               ('missed_turns', self.missed),
                               ('host_age_seconds', '' if self.last_received is None else now-self.last_received)]:
                w.writerow([key, value])
            w.writerow(FIELDS + ['bin_counts_0_to_39'])
            # Export latest status even when no turn completed.
            if self.last:
                w.writerow([self.last[k] for k in FIELDS] + [' '.join(map(str, self.last['bins']))])
            w.writerow(['retained_completed_turns'])
            for r in self.history:
                w.writerow([r[k] for k in FIELDS] + [' '.join(map(str, r['bins']))])

class Serial:
    def __init__(self, path):
        self.fd = os.open(path, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
        self.old = None
        try:
            if not os.isatty(self.fd):
                raise ValueError('not a terminal device')
            self.old = termios.tcgetattr(self.fd)
            settings = termios.tcgetattr(self.fd)
            settings[0:4] = [0, 0, termios.CS8 | termios.CREAD | termios.CLOCAL, 0]
            settings[4:6] = [termios.B115200, termios.B115200]
            settings[6][termios.VMIN] = 0
            settings[6][termios.VTIME] = 0
            termios.tcsetattr(self.fd, termios.TCSANOW, settings)
            termios.tcflush(self.fd, termios.TCIFLUSH)
        except Exception:
            self.close()
            raise
        self.poller = select.poll()
        self.poller.register(self.fd, select.POLLIN | select.POLLHUP | select.POLLERR)
        self.hangup = False

    def read(self):
        if self.hangup or self.fd is None:
            raise OSError('serial disconnected')
        events = self.poller.poll(0)
        if not events:
            return b''
        flags = events[0][1]
        self.hangup = bool(flags & (select.POLLHUP | select.POLLERR | select.POLLNVAL))
        if flags & select.POLLIN:
            try:
                data = os.read(self.fd, 4096)
            except BlockingIOError:
                data = b''
            if data:
                return data
        if self.hangup:
            raise OSError('serial disconnected')
        return b''

    def close(self):
        if self.fd is not None:
            try:
                if self.old is not None:
                    termios.tcsetattr(self.fd, termios.TCSANOW, self.old)
            except (OSError, termios.error):
                pass
            os.close(self.fd)
            self.fd = None

def read_replay(path):
    fd = os.open(path, os.O_RDONLY | os.O_NONBLOCK)
    try:
        info = os.fstat(fd)
        if not stat.S_ISREG(info.st_mode) or info.st_size > LIMIT:
            raise ValueError('capture must be a regular file at most 2 MiB')
        with os.fdopen(fd, 'rb', closefd=False) as f:
            data = f.read(LIMIT + 1)
        if len(data) > LIMIT:
            raise ValueError('capture grew beyond limit')
    finally:
        os.close(fd)
    stream = Stream()
    records = []
    for line in data.splitlines(keepends=True):
        if not line.startswith(b'#'):
            records.extend(stream.feed(line))
    stream.finish()
    if stream.errors or not records:
        raise ValueError('capture contains malformed frames or no observations')
    return records

class App:
    def __init__(self, root):
        self.root = root
        root.title('Cam Dwell Map — jayis1')
        self.port = None
        self.stream = Stream()
        self.session = Session('none')
        self.status = tk.StringVar(value='Disconnected; no measurements')
        self.path = tk.StringVar()
        self.tabs = ttk.Notebook(root)
        self.tabs.pack(fill='both', expand=True)
        connection = ttk.Frame(self.tabs, padding=12)
        map_page = ttk.Frame(self.tabs, padding=12)
        history_page = ttk.Frame(self.tabs, padding=12)
        self.tabs.add(connection, text='Connect / Replay')
        self.tabs.add(map_page, text='Angular map')
        self.tabs.add(history_page, text='Turns / Export')
        # User moment: attach a bench instrument or inspect an explicit replay.
        ttk.Label(connection, text='115200 8N1 · fixed 400 counts/turn · receive only').pack()
        self.ports = ttk.Combobox(connection, textvariable=self.path, width=55)
        self.ports.pack(pady=8)
        for text, command in [('Refresh ports', self.refresh), ('Connect', self.connect),
                              ('Disconnect', self.disconnect), ('Replay capture', self.choose_replay)]:
            ttk.Button(connection, text=text, command=command).pack(pady=3)
        ttk.Label(connection, text='Use a 100 P/R NPN encoder; swap A/B if direction is reversed.\n'
                  'No motor control. Never wire a powered switch circuit.').pack(pady=10)
        # User moment: see the latest complete turn, not partial coverage.
        self.summary = tk.StringVar(value='No complete map')
        ttk.Label(map_page, textvariable=self.summary).pack()
        self.canvas = tk.Canvas(map_page, width=720, height=240, background='white')
        self.canvas.pack(fill='both', expand=True)
        # User moment: compare trials and retain evidence with its limitations.
        self.table = ttk.Treeview(history_page, columns=('id','ms','closed','transitions'), show='headings', height=12)
        for key, title in [('id','Turn'),('ms','Duration ms'),('closed','Closed / 400'),('transitions','Edge-state changes')]:
            self.table.heading(key, text=title)
        self.table.pack(fill='both', expand=True)
        ttk.Button(history_page, text='Export private CSV (new file)', command=self.choose_export).pack()
        ttk.Label(root, textvariable=self.status, wraplength=740).pack(pady=8)
        root.protocol('WM_DELETE_WINDOW', self.close)
        self.refresh()
        self.timer = root.after(25, self.poll)

    def refresh(self):
        self.ports['values'] = sorted(glob.glob('/dev/ttyACM*') + glob.glob('/dev/ttyUSB*'))

    def disconnect(self):
        if self.port:
            self.port.close()
        self.port = None
        self.session.connected = False
        self.render()

    def connect(self):
        self.disconnect()
        try:
            self.port = Serial(self.path.get())
            self.stream = Stream()
            self.session = Session('live')
            self.render()
        except (OSError, ValueError, termios.error) as exc:
            self.status.set(str(exc))

    def load(self, path):
        records = read_replay(path)
        candidate = Session('replay')
        for r in records:
            candidate.accept(r, time.monotonic())
        self.disconnect()
        self.stream = Stream()
        self.session = candidate
        self.render()

    def choose_replay(self):
        path = filedialog.askopenfilename()
        if path:
            try:
                self.load(path)
            except (OSError, ValueError) as exc:
                messagebox.showerror('Replay rejected', str(exc))

    def choose_export(self):
        path = filedialog.asksaveasfilename(defaultextension='.csv')
        if path:
            try:
                self.session.export(path, time.monotonic(), self.stream.errors)
            except (OSError, ValueError) as exc:
                messagebox.showerror('Export failed', str(exc))

    def render(self):
        s = self.session
        age = None if s.last_received is None else time.monotonic()-s.last_received
        freshness = 'no data' if age is None else ('STALE' if age > 1 else 'recent')
        self.status.set(f'{s.source} | connected={s.connected} | {freshness} | '
                        f'parser errors={self.stream.errors} | missed turns={s.missed} | {s.warning}')
        self.canvas.delete('all')
        self.table.delete(*self.table.get_children())
        if not s.last:
            self.summary.set('No complete map')
            return
        r = s.last
        self.summary.set(f'{STATES[r["state"]]} | travel {r["travel"]}/400 | '
                         f'complete {r["completed"]} | rejected {r["rejected"]} | '
                         f'gaps {r["gaps"]} | last reason: {REASONS[r["reason"]]}')
        if r['completed']:
            for i, value in enumerate(r['bins']):
                x = 10 + i*17
                self.canvas.create_rectangle(x, 215-value*18, x+14, 215, fill='#2563a6')
                if i % 5 == 0:
                    self.canvas.create_text(x+7, 230, text=f'{i*9}°')
            self.canvas.create_text(350, 15, text=f'LAST COMPLETE TURN {r["completed"]}: closed {r["closed"]}/400 angular samples')
        for row in s.history:
            self.table.insert('', 'end', values=(row['completed'],row['duration'],row['closed'],row['transitions']))

    def poll(self):
        if self.port:
            try:
                data = self.port.read()
                now = time.monotonic()
                for r in self.stream.feed(data):
                    self.session.accept(r, now)
            except (OSError, ValueError, termios.error) as exc:
                self.session.warning = str(exc)
                self.disconnect()
        self.render()
        self.timer = self.root.after(25, self.poll)

    def close(self):
        self.root.after_cancel(self.timer)
        self.disconnect()
        self.root.destroy()

if __name__ == '__main__':
    window = tk.Tk()
    App(window)
    window.mainloop()
