#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Offline-first Linux/Tk companion. No serial writes, cloud or dependencies."""
import argparse
import binascii
import collections
import json
import os
from pathlib import Path
import select
import stat
import termios
import time
from typing import Any

NEVER = 4294967295
STATES = ('Dry baseline', 'Ready', 'Active', 'Downstream observed', 'Expired', 'Invalid')
NAMES = ('Upstream', 'Downstream', 'Left edge', 'Right edge')
FIELDS = ('seq', 'now', 'state', 'mask', 'faults', 'elapsed')
LIMIT = 1024 * 1024


def crc(data):
    return binascii.crc_hqx(data, 0xffff)


def parse(line):
    """Validate framing and necessary snapshot invariants, not physical truth."""
    if len(line) > 255:
        raise ValueError('frame too long')
    pieces = line.split(b',')
    if len(pieces) != 16 or pieces[0] != b'W1':
        raise ValueError('unsupported protocol or field count')
    body, checksum = line.rsplit(b',', 1)
    if len(checksum) != 4 or any(c not in b'0123456789ABCDEF' for c in checksum):
        raise ValueError('CRC syntax')
    if crc(body) != int(checksum, 16):
        raise ValueError('CRC mismatch')
    numbers = []
    for field in pieces[1:-1]:
        if not field or len(field) > 10 or any(c not in b'0123456789' for c in field):
            raise ValueError('unsigned decimal required')
        value = int(field)
        if value > NEVER:
            raise ValueError('integer range')
        numbers.append(value)
    r: dict[str, Any] = dict(zip(FIELDS, numbers[:6]))
    r['adc'] = numbers[6:10]
    r['first'] = numbers[10:14]
    state, elapsed, first = r['state'], r['elapsed'], r['first']
    if state > 5 or r['mask'] > 15 or r['faults'] > 127 or elapsed > 600000:
        raise ValueError('status range')
    if (state == 5) != bool(r['faults']):
        raise ValueError('fault/state conflict')
    if any(a > 4095 for a in r['adc']):
        raise ValueError('ADC range')
    if any(t != NEVER and t > elapsed for t in first):
        raise ValueError('event after elapsed time')
    if first[0] not in (0, NEVER):
        raise ValueError('upstream origin')
    if state in (0, 1) and (elapsed or any(t != NEVER for t in first)):
        raise ValueError('unstarted trial has evidence')
    if state in (2, 3, 4) and first[0] != 0:
        raise ValueError('missing upstream')
    if state in (2, 3, 4) and any(t != NEVER for t in first[2:]):
        raise ValueError('flank evidence without invalidation')
    if state == 2 and (r['mask'] != 1 or first[1] != NEVER):
        raise ValueError('invalid active trial')
    if state == 3 and r['mask'] & 12:
        raise ValueError('flank wet during success')
    if state == 3 and (first[1] != elapsed or not 0 < elapsed < 600000):
        raise ValueError('invalid breakthrough latency')
    if state == 4 and elapsed != 600000:
        raise ValueError('invalid timeout')
    r['adc_valid'] = not bool(r['faults'] & 16)
    return r


class Stream:
    def __init__(self):
        self.buffer = bytearray()
        self.errors = 0

    def feed(self, data):
        rows = []
        # Per-byte bounded buffer also resynchronizes a truncated frame when
        # the next complete marker appears without an intervening newline.
        for byte in data:
            self.buffer.append(byte)
            if len(self.buffer) > 3 and self.buffer.endswith(b'W1,'):
                self.errors += 1
                self.buffer[:] = b'W1,'
            if byte == 10:
                line = bytes(self.buffer[:-1])
                self.buffer.clear()
                try:
                    rows.append(parse(line))
                except ValueError:
                    self.errors += 1
            elif len(self.buffer) > 255:
                self.errors += 1
                self.buffer.clear()
        return rows

    def finish(self):
        if self.buffer:
            raise ValueError('truncated final frame')


class Model:
    def __init__(self, provenance):
        self.provenance = provenance
        self.rows = collections.deque(maxlen=5000)
        self.last_host = None
        self.anchor_host = None
        self.device_elapsed = 0
        self.blocked = False
        self.warning = ''
        self.errors = 0
        self.discarded = 0
        self.connected = False

    def stop(self, message):
        self.warning = message
        self.blocked = True
        return False

    def accept(self, row, host):
        if self.blocked:
            return False
        if self.rows:
            old = self.rows[-1]
            if row['seq'] == old['seq'] or row['now'] == old['now']:
                if row == old:
                    return False
                return self.stop('Contradictory duplicate; reconnect required')
            dt = (row['now'] - old['now']) & NEVER
            ds = (row['seq'] - old['seq']) & NEVER
            if dt >= 0x80000000 or ds >= 0x80000000:
                return self.stop('Reset or reordered data; reconnect required')
            if row['faults'] | old['faults'] != row['faults']:
                return self.stop('Sticky fault disappeared')
            allowed = ({0,1,2,3,4,5}, {1,2,3,4,5}, {2,3,4,5}, {3,5}, {4,5}, {5})
            if row['state'] not in allowed[old['state']]:
                return self.stop('State rollback')
            if old['state'] in (3,4,5) and (row['elapsed'] != old['elapsed'] or row['first'] != old['first']):
                return self.stop('Terminal evidence changed')
            if row['elapsed'] < old['elapsed']:
                return self.stop('Trial clock rollback')
            if row['elapsed'] - old['elapsed'] > dt:
                return self.stop('Impossible trial clock advance')
            for before, after in zip(old['first'], row['first']):
                if before != NEVER and before != after:
                    return self.stop('Arrival time changed')
            if self.provenance == 'live':
                if host - self.last_host > 2:
                    return self.stop('Reception pause; queued data cannot restore freshness')
                self.device_elapsed += dt / 1000
                age = host - self.anchor_host
                if abs(age - self.device_elapsed) > 1 + age * 0.02:
                    return self.stop('Host/device clock progress diverged')
        else:
            self.anchor_host = host
        if len(self.rows) == self.rows.maxlen:
            self.discarded += 1
        self.rows.append(row)
        self.last_host = host
        return True

    def freshness(self, host):
        if self.blocked:
            return 'blocked'
        if self.provenance == 'replay':
            return 'replay — not live'
        if self.last_host is None:
            return 'no data'
        return 'fresh' if host - self.last_host <= 1.5 else 'stale'

    def export(self, path, host):
        document = {'author': 'jayis1', 'device': 'wick-escape', 'protocol': 'W1',
                    'provenance': self.provenance, 'connected': self.connected,
                    'freshness': self.freshness(host), 'warning': self.warning,
                    'parser_errors': self.errors, 'discarded_rows': self.discarded,
                    'age_seconds': None if self.last_host is None else max(0, host-self.last_host),
                    'records': list(self.rows)}
        fd = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
        with os.fdopen(fd, 'w') as output:
            json.dump(document, output, indent=2)
            output.write('\n')


def replay(path):
    fd = os.open(path, os.O_RDONLY | os.O_NONBLOCK)
    with os.fdopen(fd, 'rb') as file:
        info = os.fstat(file.fileno())
        if not stat.S_ISREG(info.st_mode) or info.st_size > LIMIT:
            raise ValueError('Replay must be a regular file no larger than 1 MiB')
        data = file.read(LIMIT + 1)
        if len(data) > LIMIT:
            raise ValueError('Replay grew beyond limit')
    stream = Stream()
    model = Model('replay')
    for row in stream.feed(data):
        if not model.accept(row, time.monotonic()) and model.blocked:
            raise ValueError(model.warning)
    stream.finish()
    if stream.errors or not model.rows:
        raise ValueError('Replay contains corrupt frames or no snapshots')
    return model


class Port:
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
            self.poller.register(self.fd, select.POLLIN | select.POLLERR | select.POLLHUP)
        except Exception:
            self.close()
            raise

    def read(self):
        if self.fd is None:
            raise OSError('Port is closed')
        data = bytearray()
        hungup = False
        for _, flags in self.poller.poll(0):
            if flags & select.POLLIN:
                try:
                    data.extend(os.read(self.fd, 4096))
                except BlockingIOError:
                    pass
            if flags & (select.POLLERR | select.POLLHUP | select.POLLNVAL):
                hungup = True
        return bytes(data), hungup

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
        import tkinter as tk
        from tkinter import ttk
        self.root = root
        self.port = None
        self.stream = Stream()
        self.model = Model('live')
        self.status = tk.StringVar(value='Disconnected — no data')
        self.path = tk.StringVar(value='/dev/ttyACM0')
        self.detail = tk.StringVar(value='No coupon observations')
        root.title('Wick Escape — jayis1')
        root.geometry('960x580')
        self.tabs = ttk.Notebook(root)
        self.tabs.pack(fill='both', expand=True)
        self.pages = []
        for title in ('Connection', 'Live coupon', 'History', 'Method'):
            page = ttk.Frame(self.tabs, padding=12)
            self.tabs.add(page, text=title)
            self.pages.append(page)
        ttk.Label(self.pages[0], text='Linux ST-LINK serial device (115200, read-only)').pack()
        ttk.Entry(self.pages[0], textvariable=self.path, width=60).pack()
        self.connect_button = ttk.Button(self.pages[0], text='Connect / new session', command=self.connect)
        self.connect_button.pack()
        ttk.Button(self.pages[0], text='Disconnect, keep evidence', command=self.disconnect).pack()
        ttk.Button(self.pages[0], text='Open raw W1 replay', command=self.open_replay).pack()
        ttk.Button(self.pages[0], text='Export evidence JSON (new file)', command=self.export_dialog).pack()
        ttk.Label(root, textvariable=self.status, wraplength=920).pack(fill='x')
        ttk.Label(self.pages[1], textvariable=self.detail, font=('TkDefaultFont', 13), justify='left').pack(anchor='w')
        self.bars = []
        for name in NAMES:
            ttk.Label(self.pages[1], text=name+' ADC count (wet ≤1200; dry ≥3000)').pack(anchor='w')
            bar = ttk.Progressbar(self.pages[1], maximum=4095, length=700)
            bar.pack(anchor='w', pady=8)
            self.bars.append(bar)
        columns = ('sequence','uptime','state','wet mask','faults','latency ms')
        self.history = ttk.Treeview(self.pages[2], columns=columns, show='headings', height=18)
        for column in columns:
            self.history.heading(column, text=column)
            self.history.column(column, width=135)
        self.history.pack(fill='both', expand=True)
        ttk.Label(self.pages[3], wraplength=850, justify='left', text=(
            'Author: jayis1\n\nUse inert conductive-water coupons only. Four isolated graphite electrode pairs '
            'sit upstream, downstream, and outside the left/right ends of the wax barrier. '
            'Reset with a dry coupon; wait for Ready before adding liquid.\n\n'
            'The device needs five threshold samples per transition at 20 ms intervals. '
            'Downstream observed is provisional: later flank wetting invalidates it. '
            'A ten-minute timeout is NOT proof of a sound barrier.\n\n'
            'Exports retain provenance, faults and freshness. No remote commands, calibration changes, '
            'cloud upload or medical conclusions. Power off between trials. See README for assembly '
            'and mandatory resistor substitution tests before wet work.')).pack(anchor='w')
        root.protocol('WM_DELETE_WINDOW', self.close)
        self.after = root.after(40, self.tick)

    def connect(self):
        from tkinter import messagebox
        self.disconnect()
        try:
            self.port = Port(self.path.get())
            self.stream = Stream()
            self.model = Model('live')
            self.model.connected = True
            self.render()
        except (OSError, termios.error) as error:
            messagebox.showerror('Connection failed', str(error))

    def disconnect(self):
        if self.port:
            self.port.close()
            self.port = None
        self.model.connected = False
        self.render()

    def open_replay(self):
        from tkinter import filedialog, messagebox
        path = filedialog.askopenfilename(title='Raw W1 recording, maximum 1 MiB')
        if not path:
            return
        try:
            loaded = replay(path)
            self.disconnect()
            self.model = loaded
            self.render()
        except (OSError, ValueError) as error:
            messagebox.showerror('Replay rejected', str(error))

    def export_dialog(self):
        from tkinter import filedialog, messagebox
        path = filedialog.asksaveasfilename(defaultextension='.json')
        if path:
            try:
                self.model.export(path, time.monotonic())
            except (OSError, ValueError) as error:
                messagebox.showerror('Export failed (will not overwrite)', str(error))

    def render(self):
        connection = 'Connected' if self.model.connected else 'Disconnected'
        self.status.set(f'{connection} | {self.model.provenance} | '
                        f'{self.model.freshness(time.monotonic())} | '
                        f'parser errors {self.model.errors} | {self.model.warning}')
        self.history.delete(*self.history.get_children())
        for r in list(self.model.rows)[-200:]:
            self.history.insert('', 'end', values=(r['seq'], r['now'], STATES[r['state']],
                                                  r['mask'], r['faults'], r['elapsed']))
        if not self.model.rows:
            self.detail.set('No coupon observations')
            for bar in self.bars:
                bar['value'] = 0
            return
        r = self.model.rows[-1]
        first = ', '.join('unseen' if t == NEVER else str(t)+' ms' for t in r['first'])
        self.detail.set(f'{STATES[r["state"]]} — trial clock {r["elapsed"]} ms\n'
                        f'First arrivals U/D/L/R: {first}\n'
                        f'Wet mask {r["mask"]:04b}; sticky faults {r["faults"]}; provisional, not certified\n'
                        + ('' if r['adc_valid'] else 'ADC INVALID: bars suppressed; exported counts are last-known, not fresh measurements'))
        for bar, value in zip(self.bars, r['adc']):
            bar['value'] = value if r['adc_valid'] else 0

    def tick(self):
        try:
            if self.port:
                data, hungup = self.port.read()
                for row in self.stream.feed(data):
                    self.model.accept(row, time.monotonic())
                self.model.errors = self.stream.errors
                if hungup:
                    self.disconnect()
                    self.model.warning = 'Serial hangup; evidence retained'
        except (OSError, termios.error) as error:
            self.disconnect()
            self.model.warning = str(error)
        self.render()
        self.after = self.root.after(40, self.tick)

    def close(self):
        self.root.after_cancel(self.after)
        self.disconnect()
        self.root.destroy()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--replay', type=Path)
    parser.add_argument('--export', type=Path)
    args = parser.parse_args()
    if args.replay:
        model = replay(args.replay)
        if args.export:
            model.export(args.export, time.monotonic())
        print(f'Author: jayis1; validated {len(model.rows)} replay snapshots')
        return
    if args.export:
        parser.error('--export requires --replay')
    import tkinter as tk
    root = tk.Tk()
    App(root)
    root.mainloop()


if __name__ == '__main__':
    main()
