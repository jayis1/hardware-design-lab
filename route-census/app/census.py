#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Route Census: offline Linux serial instrument and bounded evidence notebook."""
import argparse
import binascii

import json
import os
from pathlib import Path
from typing import Any
import select
import stat
import termios
import time
import tkinter as tk
from tkinter import ttk, filedialog, messagebox

AUTHOR = 'jayis1'
STATES = ('Clearing', 'Ready for ONE ball', 'In flight', 'Confirming clear', 'Fault — reset required')
REASONS = ('none', 'overlapping inlet', 'inlet/outlet simultaneous',
           'multiple outlets', '30s deadline', 'sample gap', 'gate blocked', 'limit/startup fault')
FIELDS = ('sequence', 'time', 'state', 'levels', 'entries', 'accepted', 'rejected',
          'orphans', 'gaps', 'blocks', 'last_route', 'last_ms', 'reason', 'flags', 'tx_errors')
ROUTE_FIELDS = ('count', 'minimum', 'maximum', 'sum', 'b0', 'b1', 'b2', 'b3', 'b4', 'b5')
LIMIT = 4 * 1024 * 1024


def crc(data):
    return binascii.crc_hqx(data, 0xffff)


def decode(line):
    if len(line) > 640:
        raise ValueError('oversized frame')
    try:
        body, check = line.rsplit(b'*', 1)
        if len(check) != 4 or int(check, 16) != crc(body):
            raise ValueError('checksum')
        parts = body.split(b',')
        if parts[0] != b'RTC1' or len(parts) != 46:
            raise ValueError('incompatible version/field count')
        if any(not p or not p.isdigit() or len(p) > 20 for p in parts[1:]):
            raise ValueError('noncanonical unsigned integer')
        numbers = [int(p) for p in parts[1:]]
        result: dict[str, Any] = dict(zip(FIELDS, numbers[:15]))
        routes: list[dict[str, int]] = [dict(zip(ROUTE_FIELDS, numbers[15+i*10:25+i*10])) for i in range(3)]
        result['routes'] = routes
        if any(v > 0xffffffff for v in numbers[:15]):
            raise ValueError('header range')
        if result['state'] > 4 or result['levels'] > 15 or result['reason'] > 7:
            raise ValueError('state range')
        if result['flags'] & ~27 or result['last_route'] > 3 or result['last_ms'] >= 30000:
            raise ValueError('result range')
        if bool(result['last_route']) != bool(result['last_ms']):
            raise ValueError('partial result')
        if result['last_route'] and result['reason']:
            raise ValueError('rejected result marked accepted')
        if result['state'] == 1 and result['levels']:
            raise ValueError('ready while blocked')
        if result['entries'] != result['accepted'] + result['rejected'] + (result['state'] in (2,3)):
            raise ValueError('trial conservation')
        for r in routes:
            if any(v > (0xffffffffffffffff if k == 'sum' else 0xffffffff) for k,v in r.items()):
                raise ValueError('route range')
            if sum(r[f'b{i}'] for i in range(6)) != r['count']:
                raise ValueError('histogram conservation')
            if r['count']:
                if not 0 < r['minimum'] <= r['maximum'] < 30000:
                    raise ValueError('duration range')
                if not r['minimum'] * r['count'] <= r['sum'] <= r['maximum'] * r['count']:
                    raise ValueError('sum range')
            elif any(r.values()):
                raise ValueError('nonempty zero-count route')
        if sum(r['count'] for r in routes) != result['accepted']:
            raise ValueError('route conservation')
        return result
    except (ValueError, IndexError) as exc:
        raise ValueError(f'invalid RTC1 frame: {exc}') from exc


class Model:
    def __init__(self):
        self.buffer = bytearray()
        self.rows = []
        self.errors = 0
        self.received = None
        self.frozen = False
        self.warning = ''
        self.source = 'none'
        self.connected = False

    def stale(self, now=None):
        if now is None:
            now = time.monotonic()
        return self.received is None or now - self.received > 1.0

    def accept(self, row, now):
        if self.frozen:
            return
        if self.rows:
            previous = self.rows[-1]
            dt = (row['time'] - previous['time']) & 0xffffffff
            ds = (row['sequence'] - previous['sequence']) & 0xffffffff
            if row == previous:
                return  # retransmission is not fresh device progress
            rollback = not 0 < dt < 0x80000000 or not 0 < ds < 0x80000000
            for key in ('entries', 'accepted', 'rejected', 'orphans', 'gaps', 'blocks', 'tx_errors'):
                rollback |= row[key] < previous[key]
            rollback |= row['flags'] & previous['flags'] != previous['flags']
            for before, after in zip(previous['routes'], row['routes']):
                for key in ('count', 'sum', 'maximum', 'b0', 'b1', 'b2', 'b3', 'b4', 'b5'):
                    rollback |= after[key] < before[key]
                if before['count']:
                    rollback |= after['minimum'] > before['minimum']
            if rollback:
                self.frozen = True
                self.warning = 'Conflicting/rolled-back evidence: reconnect starts a new notebook.'
                return
        self.rows.append(row)
        self.rows = self.rows[-2000:]
        self.received = now

    def feed(self, data, now=None):
        if now is None:
            now = time.monotonic()
        # Bytewise marker recovery handles a timeout-truncated frame immediately
        # followed by a fresh RTC1 frame, even without an intervening newline.
        for byte in data:
            self.buffer.append(byte)
            if self.buffer.endswith(b'RTC1,') and len(self.buffer) > 5:
                self.buffer = bytearray(b'RTC1,')
                self.errors += 1
            if byte == 10:
                try:
                    self.accept(decode(bytes(self.buffer).strip()), now)
                except ValueError:
                    self.errors += 1
                self.buffer.clear()
            elif len(self.buffer) > 640:
                self.buffer = self.buffer[-4:]
                self.errors += 1

    def finish(self):
        # EOF is meaningful only for a completed replay, never a live read chunk.
        if self.buffer:
            self.errors += 1
            self.buffer.clear()

    def export(self):
        return {'author': AUTHOR, 'protocol': 'RTC1', 'source': self.source,
                'connected': self.connected, 'stale': self.stale(),
                'age_seconds': None if self.received is None else time.monotonic()-self.received,
                'frozen': self.frozen, 'warning': self.warning, 'parse_errors': self.errors,
                'retention': 'last 2000 snapshots; device counters cumulative since reset',
                'rows': self.rows}


class Serial:
    def __init__(self, path):
        self.fd = os.open(path, os.O_RDONLY | os.O_NOCTTY | os.O_NONBLOCK)
        self.original = None
        try:
            self.original = termios.tcgetattr(self.fd)
            settings = termios.tcgetattr(self.fd)
            settings[0] = settings[1] = settings[3] = 0
            settings[2] = termios.CS8 | termios.CREAD | termios.CLOCAL
            settings[4] = settings[5] = termios.B115200
            settings[6][termios.VMIN] = settings[6][termios.VTIME] = 0
            termios.tcsetattr(self.fd, termios.TCSANOW, settings)
            termios.tcflush(self.fd, termios.TCIFLUSH)
            self.poller = select.poll()
            self.poller.register(self.fd, select.POLLIN | select.POLLERR | select.POLLHUP)
        except Exception:
            self.close()
            raise

    def read(self):
        if self.fd is None:
            raise OSError('serial closed')
        events = self.poller.poll(0)
        if not events:
            return b''
        mask = events[0][1]
        if mask & select.POLLIN:
            data = os.read(self.fd, 4096)
            if data:
                return data
        if mask & (select.POLLHUP | select.POLLERR | select.POLLNVAL):
            raise OSError('serial disconnected')
        return b''

    def close(self):
        if self.fd is not None:
            if self.original is not None:
                try:
                    termios.tcsetattr(self.fd, termios.TCSANOW, self.original)
                except (OSError, termios.error):
                    pass
            os.close(self.fd)
            self.fd = None


def read_replay(path):
    fd = os.open(path, os.O_RDONLY | os.O_NONBLOCK)
    try:
        info = os.fstat(fd)
        if not stat.S_ISREG(info.st_mode) or info.st_size > LIMIT:
            raise ValueError('Replay must be a regular file no larger than 4 MiB')
        with os.fdopen(fd, 'rb', closefd=False) as stream:
            data = stream.read(LIMIT + 1)
        if len(data) > LIMIT:
            raise ValueError('Replay grew beyond limit')
        return data
    finally:
        os.close(fd)


class App:
    def __init__(self, root):
        self.root = root
        root.title('Route Census — jayis1')
        root.geometry('1040x640')
        self.model = Model()
        self.link = None
        self.path = tk.StringVar(value='/dev/ttyACM0')
        self.summary = tk.StringVar(value='No evidence loaded')
        self.status = tk.StringVar(value='Disconnected')
        controls = ttk.Frame(root); controls.pack(fill='x')
        ttk.Entry(controls, textvariable=self.path, width=35).pack(side='left')
        for label, callback in [('Connect / new notebook', self.connect), ('Disconnect', self.disconnect),
                                ('Replay', self.replay), ('Export', self.export_dialog)]:
            ttk.Button(controls, text=label, command=callback).pack(side='left')
        ttk.Label(root, textvariable=self.status).pack(fill='x')
        pages = ttk.Notebook(root); pages.pack(fill='both', expand=True)
        live = ttk.Frame(pages); pages.add(live, text='Route census')
        ttk.Label(live, textvariable=self.summary, font=('', 14), justify='left').pack(anchor='w', padx=15, pady=15)
        self.routes = ttk.Treeview(live, columns=('route','count','share','min','mean','max'), show='headings', height=3)
        for column in self.routes['columns']:
            self.routes.heading(column, text=column)
            self.routes.column(column, width=130)
        self.routes.pack(fill='x', padx=15)
        ttk.Label(live, text='Use ONE opaque ball. Wait for Ready. A result is not proof of the ball’s identity.\n'
                  'Rejections and orphan outlets must be reported alongside accepted route shares.\n'
                  'Timing bins: <250, <500, <1000, <2000, <5000, <30000 ms.').pack(anchor='w', padx=15, pady=15)
        hist = ttk.Frame(pages); pages.add(hist, text='Evidence snapshots')
        self.history = ttk.Treeview(hist, columns=('seq','time','state','accepted','rejected','last'), show='headings')
        for c in self.history['columns']:
            self.history.heading(c,text=c); self.history.column(c,width=130)
        self.history.pack(fill='both',expand=True)
        guide = tk.Text(pages, wrap='word'); pages.add(guide, text='Setup / limits')
        guide.insert('1.0', 'Author: jayis1\n\nJ3 inlet, J4 left, J5 middle, J6 right. Signal pin 1, ground pin 2. '
                     'Use only low-active open-collector 2167 receivers and a common ground. Sensor power is external. '
                     'No 5V signal may enter J2. Check each gate before introducing a ball.\n\n'
                     'Three consecutive 1ms samples qualify each input independently. All gates must remain '
                     'qualified clear for 100ms before a new trial. Exit before 30s; a 2s obstruction aborts. '
                     'After rejected trials physically empty the track before resuming.\n\n'
                     'Replay is synthetic or historical, never live. Exports are local JSON with source, freshness, '
                     'errors and bounded snapshots. No network, accounts, remote commands or persistent configuration. '
                     'Counters are volatile. Reset inference cannot detect every lost reboot. This is not a safety device.')
        guide.configure(state='disabled')
        root.protocol('WM_DELETE_WINDOW',self.close)
        self.timer = root.after(50,self.tick)

    def disconnect(self):
        if self.link:
            self.link.close()
        self.link = None
        self.model.connected = False
        self.status.set('Disconnected — retained evidence')

    def connect(self):
        self.disconnect()
        try:
            link = Serial(self.path.get())
        except (OSError, termios.error) as exc:
            self.status.set(f'Connection failed: {exc}')
            return
        self.link = link
        self.model = Model(); self.model.source = 'live'; self.model.connected = True
        self.render()

    def load(self, path):
        data = read_replay(path)
        self.disconnect()
        self.model = Model(); self.model.source = 'replay'
        self.model.feed(data)
        self.model.finish()
        self.render()

    def replay(self):
        path = filedialog.askopenfilename(title='Load RTC1 recording (max 4 MiB)')
        if path:
            try:
                self.load(path)
            except (OSError, ValueError) as exc:
                messagebox.showerror('Replay failed', str(exc))

    def save(self, path):
        # Exclusive create, private permissions, never silently overwrite evidence.
        fd = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
        with os.fdopen(fd,'w') as stream:
            json.dump(self.model.export(), stream, indent=2)
            stream.write('\n')

    def export_dialog(self):
        path = filedialog.asksaveasfilename(defaultextension='.json')
        if path:
            try:
                self.save(path)
            except OSError as exc:
                messagebox.showerror('Export failed',str(exc))

    def render(self):
        for tree in (self.history,self.routes):
            tree.delete(*tree.get_children())
        if not self.model.rows:
            self.summary.set('No evidence loaded')
            return
        row = self.model.rows[-1]
        self.summary.set(f"{STATES[row['state']]}\nAccepted: {row['accepted']}   Rejected: {row['rejected']}   "
                         f"Orphans: {row['orphans']}\nGates: {row['levels']:04b}   Gaps: {row['gaps']}   "
                         f"Blocked episodes: {row['blocks']}\nLast route: {row['last_route']}   "
                         f"Transit: {row['last_ms']}ms   Reason: {REASONS[row['reason']]}\n"
                         f"Sticky flags: {row['flags']}   UART errors: {row['tx_errors']}")
        for name,r in zip(('Left','Middle','Right'),row['routes']):
            share = f"{100*r['count']/row['accepted']:.1f}%" if row['accepted'] else '—'
            mean = f"{r['sum']/r['count']:.1f}" if r['count'] else '—'
            self.routes.insert('', 'end', values=(name,r['count'],share,r['minimum'],mean,r['maximum']))
        for r in self.model.rows:
            self.history.insert('','end',values=(r['sequence'],r['time'],STATES[r['state']],r['accepted'],r['rejected'],r['last_route']))

    def tick(self):
        if self.link:
            try:
                data = self.link.read()
                if data:
                    self.model.feed(data); self.render()
            except OSError as exc:
                self.disconnect(); self.model.warning = str(exc)
        freshness = 'STALE' if self.model.stale() else 'recent snapshot'
        self.status.set(f'{self.model.source} / {"connected" if self.model.connected else "offline"} / '
                        f'{freshness} / parse errors {self.model.errors} / {self.model.warning}')
        self.timer = self.root.after(50,self.tick)

    def close(self):
        self.root.after_cancel(self.timer)
        self.disconnect()
        self.root.destroy()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', type=Path, help='Validate a raw RTC1 recording without a display')
    args=parser.parse_args()
    if args.check:
        model=Model(); model.source='replay'; model.feed(read_replay(args.check)); model.finish()
        print(json.dumps(model.export(),indent=2))
        return 0 if model.rows and not model.errors and not model.frozen else 1
    root=tk.Tk(); App(root); root.mainloop()
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
