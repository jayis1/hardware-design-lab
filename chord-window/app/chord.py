#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Chord Window: local Linux/Tk instrument notebook, no third-party packages."""
import argparse
import collections
import csv
import os
from pathlib import Path
import re
import select
import stat
import termios
import time
import tkinter as tk
from tkinter import ttk, filedialog, messagebox

FIELDS = ['ms', 'state', 'mask', 'gaps', 'tx_errors', 'id', 'reason',
          'keys', 'spread_ms', 'overlap_ms', 'release_ms', 'duration_ms']
REASONS = ['none', 'complete', 'retap', 'timeout', 'acquisition gap']
STATES = ['release all / baseline', 'ready', 'capturing', 'terminal fault']
LIMIT = 4 * 1024 * 1024


def crc(data):
    value = 0xffff
    for byte in data:
        value ^= byte << 8
        for _ in range(8):
            value = ((value << 1) ^ (0x1021 if value & 0x8000 else 0)) & 0xffff
    return value


def decode(line):
    if not re.fullmatch(rb'CW1(?:,[0-9]{1,10}){12}\*[0-9A-F]{4}', line):
        raise ValueError('Invalid version, framing or field count')
    body, checksum = line.split(b'*')
    if crc(body) != int(checksum, 16):
        raise ValueError('CRC mismatch')
    v = tuple(int(x) for x in body.split(b',')[1:])
    ms, state, mask, gaps, tx, ident, why, keys, spread, overlap, release, duration = v
    if any(x > 0xffffffff for x in v):
        raise ValueError('Integer overflow')
    if state > 3 or mask > 15 or why > 4 or keys > 15:
        raise ValueError('Invalid enum or bitmask')
    if (state == 1 and mask) or (state == 2 and not mask):
        raise ValueError('Impossible state')
    if ident == 0:
        if any(v[6:]):
            raise ValueError('Result without identity')
    else:
        if not why or not keys or not 1 <= duration <= 10000:
            raise ValueError('Invalid trial')
        if why == 1:
            if duration >= 10000 or spread > duration or release > duration:
                raise ValueError('Impossible timing')
            if overlap != max(0, duration-release-spread):
                raise ValueError('Inconsistent shared hold')
            if keys.bit_count() == 1 and (spread or release):
                raise ValueError('Single key cannot have timing spread')
        elif any((spread, overlap, release)):
            raise ValueError('Invalid trial contains timing estimates')
        if why == 3 and duration != 10000:
            raise ValueError('Incorrect timeout')
        if why == 4 and not gaps:
            raise ValueError('Gap result without gap counter')
    return v


class Model:
    def __init__(self, provenance):
        self.provenance = provenance
        self.buffer = bytearray()
        self.rows = collections.deque(maxlen=1000)
        self.last = None
        self.received = None
        self.errors = 0
        self.missed = 0
        self.evicted = 0
        self.blocked = False
        self.warning = ''

    def accept(self, v, now):
        if self.blocked:
            return
        if self.last is not None:
            old = self.last
            dt = (v[0]-old[0]) & 0xffffffff
            if dt == 0:
                if v != old:
                    self.blocked = True
                    self.warning = 'Contradictory same-time frame; reconnect explicitly'
                return
            if (dt >= 0x80000000 or v[3] < old[3] or v[4] < old[4]
                    or v[5] < old[5] or (old[1] == 3 and v[1] != 3)):
                self.blocked = True
                self.warning = 'Possible restart or counter rollback; reconnect explicitly'
                return
            if v[5] == old[5] and v[6:] != old[6:]:
                self.blocked = True
                self.warning = 'Contradictory result identity; reconnect explicitly'
                return
            self.missed += max(0, v[5]-old[5]-1)
        # A late join retains the latest completed result, explicitly without
        # claiming it happened after the user connected.
        if v[5] and (self.last is None or v[5] != self.last[5]):
            if len(self.rows) == self.rows.maxlen:
                self.evicted += 1
            self.rows.append(v)
        self.last = v
        self.received = now

    def feed(self, data, now):
        # Per-byte bound resynchronizes on the full marker, including after a
        # timed-out truncated UART transmission with no terminating newline.
        for byte in data:
            self.buffer.append(byte)
            if self.buffer.endswith(b'CW1') and len(self.buffer) > 3:
                self.errors += 1
                self.buffer[:] = b'CW1'
            if byte == 10:
                try:
                    self.accept(decode(bytes(self.buffer[:-1])), now)
                except ValueError as exc:
                    self.errors += 1
                    self.warning = str(exc)
                self.buffer.clear()
            elif len(self.buffer) > 255:
                self.errors += 1
                self.buffer.clear()
                self.warning = 'Oversized frame discarded'

    def finish(self):
        if self.buffer:
            self.errors += 1
            self.warning = 'Truncated recording tail'
            raise ValueError(self.warning)

    def age(self, now):
        return None if self.received is None else max(0, now-self.received)


def replay(path):
    fd = os.open(path, os.O_RDONLY | os.O_NONBLOCK)
    try:
        info = os.fstat(fd)
        if not stat.S_ISREG(info.st_mode) or info.st_size > LIMIT:
            raise ValueError('Replay must be a regular file of at most 4 MiB')
        data = bytearray()
        while len(data) <= LIMIT:
            part = os.read(fd, min(65536, LIMIT+1-len(data)))
            if not part:
                break
            data.extend(part)
        if len(data) > LIMIT:
            raise ValueError('Replay grew beyond 4 MiB')
    finally:
        os.close(fd)
    model = Model('replay (origin not authenticated)')
    model.feed(data, time.monotonic())
    model.finish()
    if model.last is None:
        raise ValueError('Recording contains no accepted snapshot')
    return model


def export(model, path, now, connected):
    fd = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
    with os.fdopen(fd, 'w', newline='') as output:
        writer = csv.writer(output)
        writer.writerow(['record_type', 'author', 'provenance', 'connected', 'age_seconds',
                         'parser_errors', 'missed_results', 'evicted_rows',
                         'blocked', 'warning'] + FIELDS)
        metadata = ['jayis1', model.provenance, connected, model.age(now),
                    model.errors, model.missed, model.evicted, model.blocked, model.warning]
        writer.writerow(['session'] + metadata + [''] * len(FIELDS))
        for row in model.rows:
            writer.writerow(['trial'] + metadata + list(row))


class Serial:
    def __init__(self, path):
        self.fd = os.open(path, os.O_RDONLY | os.O_NOCTTY | os.O_NONBLOCK)
        self.saved = None
        try:
            self.saved = termios.tcgetattr(self.fd)
            settings = termios.tcgetattr(self.fd)
            settings[0] = 0
            settings[1] = 0
            settings[2] = termios.CS8 | termios.CREAD | termios.CLOCAL
            settings[3] = 0
            settings[4] = termios.B115200
            settings[5] = termios.B115200
            settings[6][termios.VMIN] = 0
            settings[6][termios.VTIME] = 0
            termios.tcsetattr(self.fd, termios.TCSANOW, settings)
            termios.tcflush(self.fd, termios.TCIFLUSH)
            self.poller = select.poll()
            self.poller.register(self.fd, select.POLLIN | select.POLLHUP | select.POLLERR)
        except BaseException:
            self.close()
            raise

    def read(self):
        if self.fd is None:
            return b'', True
        events = self.poller.poll(0)
        data = bytearray()
        disconnected = False
        for _, flags in events:
            if flags & (select.POLLHUP | select.POLLERR | select.POLLNVAL):
                disconnected = True
            if flags & (select.POLLIN | select.POLLHUP):
                # Drain preserved tail bytes before honoring HUP. Bound each
                # GUI turn; if the budget is consumed, defer final close until
                # the next poll so continuous input cannot starve the UI.
                for _ in range(8):
                    try:
                        part = os.read(self.fd, 8192)
                    except BlockingIOError:
                        break
                    except OSError:
                        disconnected = True
                        break
                    if not part:
                        break
                    data.extend(part)
                else:
                    disconnected = False
        return bytes(data), disconnected

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


class Window:
    def __init__(self, root):
        self.root = root
        self.serial = None
        self.model = Model('none')
        self.root.title('Chord Window — jayis1')
        self.root.geometry('1040x620')
        self.path = tk.StringVar(value='/dev/ttyACM0')
        self.status = tk.StringVar(value='Disconnected')
        self.summary = tk.StringVar(value='No retained trials')
        self.target = tk.StringVar(value='3')
        self.spread = tk.StringVar(value='80')
        self.hold = tk.StringVar(value='30')
        self.live = tk.StringVar(value='No data')
        bar = ttk.Frame(root); bar.pack(fill='x')
        ttk.Label(bar, text='Serial path').pack(side='left')
        ttk.Entry(bar, textvariable=self.path, width=24).pack(side='left')
        for title, callback in [('Connect / new session', self.connect),
                                ('Disconnect', self.disconnect),
                                ('Replay', self.load), ('Export CSV', self.save)]:
            ttk.Button(bar, text=title, command=callback).pack(side='left')
        ttk.Label(root, textvariable=self.status, wraplength=1000).pack(fill='x')
        self.tabs = ttk.Notebook(root); self.tabs.pack(expand=True, fill='both')
        pages = [ttk.Frame(self.tabs) for _ in range(4)]
        for page, title in zip(pages, ['Live contacts', 'Trials', 'Compare policy', 'Setup / limits']):
            self.tabs.add(page, text=title)
        ttk.Label(pages[0], textvariable=self.live, font=('TkDefaultFont', 18),
                  wraplength=950).pack(padx=20, pady=30)
        ttk.Label(pages[0], text='A=1 B=2 C=4 D=8. Release all for 100 ms before each trial.\n'
                  'No medical interpretation. A complete trial is not a recommended gesture.').pack()
        self.table = ttk.Treeview(pages[1], columns=FIELDS[5:], show='headings')
        for name in FIELDS[5:]:
            self.table.heading(name, text=name)
            self.table.column(name, width=110)
        scroll = ttk.Scrollbar(pages[1], orient='vertical', command=self.table.yview)
        self.table.configure(yscrollcommand=scroll.set)
        scroll.pack(side='right', fill='y'); self.table.pack(expand=True, fill='both')
        for label, variable in [('Target mask (1..15)', self.target),
                                ('Maximum press spread ms', self.spread),
                                ('Minimum shared hold ms', self.hold)]:
            ttk.Label(pages[2], text=label).pack()
            ttk.Entry(pages[2], textvariable=variable).pack()
        ttk.Button(pages[2], text='Recalculate retained trials', command=self.compare).pack(pady=15)
        ttk.Label(pages[2], textvariable=self.summary, wraplength=950).pack()
        ttk.Label(pages[3], wraplength=950, justify='left', text=(
            'Author: jayis1\nConnect four voltage-free switches, not a powered keyboard matrix. '
            'J3..J6 pin 1 is A..D, pin 2 ground. J1 is 3.3 V only. NUCLEO-F401RE provides USB.\n\n'
            '5 ms independent qualification, 1 ms nominal sampling, 10 s trial timeout. '
            'Retaps and acquisition gaps invalidate timing. A press during baseline is not a trial. '
            'No authentication, storage, remote commands or patient scoring.\n\n'
            'Connect/replay replaces retained data; export first. The first result may predate attachment. '
            'Replay is historical, never live. Only 1000 results are retained. A lost result is detectable '
            'only between observed IDs. Reset inference is heuristic without persistent boot identity. '
            'CSV contains raw timing and provenance, not the editable policy verdict.')).pack(padx=20,pady=20)
        self.timer = self.root.after(25, self.poll)
        self.root.protocol('WM_DELETE_WINDOW', self.close)

    def disconnect(self):
        if self.serial:
            self.serial.close()
            self.serial = None
        self.refresh()

    def connect(self):
        self.disconnect()
        try:
            serial = Serial(self.path.get())
        except (OSError, termios.error) as exc:
            self.status.set('Cannot connect: ' + str(exc)); return
        self.serial = serial
        self.model = Model('live')
        self.refresh()

    def load(self):
        path = filedialog.askopenfilename()
        if not path:
            return
        try:
            model = replay(path)
        except (OSError, ValueError) as exc:
            messagebox.showerror('Replay rejected', str(exc)); return
        self.disconnect()
        self.model = model
        self.refresh()

    def save(self):
        path = filedialog.asksaveasfilename(defaultextension='.csv')
        if path:
            try:
                export(self.model, path, time.monotonic(), self.serial is not None)
            except (OSError, ValueError) as exc:
                messagebox.showerror('Export rejected', str(exc))

    def compare(self):
        try:
            target, spread, hold = int(self.target.get()), int(self.spread.get()), int(self.hold.get())
            if not 1 <= target <= 15 or not 0 <= spread <= 10000 or not 0 <= hold <= 10000:
                raise ValueError('Policy outside range')
        except ValueError:
            self.summary.set('Enter mask 1..15 and timing limits 0..10000'); return
        rows = list(self.model.rows)
        good = [r for r in rows if r[6] == 1 and r[7] == target and r[8] <= spread and r[9] >= hold]
        complete = [r for r in rows if r[6] == 1]
        self.summary.set(f'{len(good)} / {len(rows)} retained trials meet this local policy; '
                         f'{len(complete)} complete, {len(rows)-len(complete)} rejected. '
                         'Not a medical score or a firmware setting.')

    def refresh(self):
        model = self.model
        age = model.age(time.monotonic())
        connection = 'Connected' if self.serial else 'Disconnected'
        freshness = 'no data' if age is None else ('STALE' if age > 1 else 'recent receipt')
        if model.provenance != 'live':
            freshness = 'historical / not live'
        self.status.set(f'{connection} | {model.provenance} | {freshness} | '
                        f'errors={model.errors} missed={model.missed} evicted={model.evicted} '
                        f'blocked={model.blocked} {model.warning}')
        if model.last:
            r = model.last
            self.live.set(f'{STATES[r[1]]}\nA B C D: ' + ' '.join(str((r[2] >> i) & 1) for i in range(4))
                          + f'\nDevice ms: {r[0]}   acquisition gaps: {r[3]}   TX errors: {r[4]}')
        else:
            self.live.set('No data')
        # All retained rows are rendered, including batched UART reads.
        current = self.table.get_children()
        if current:
            self.table.delete(*current)
        for row in model.rows:
            values = list(row[5:]); values[1] = REASONS[row[6]]
            self.table.insert('', 'end', values=values)
        self.compare()

    def poll(self):
        if self.serial:
            try:
                data, hungup = self.serial.read()
                self.model.feed(data, time.monotonic())
                if hungup:
                    self.disconnect()
            except OSError as exc:
                self.model.warning = 'Serial error: ' + str(exc)
                self.disconnect()
        self.refresh()
        self.timer = self.root.after(50, self.poll)

    def close(self):
        self.root.after_cancel(self.timer)
        self.disconnect()
        self.root.destroy()


def main():
    parser = argparse.ArgumentParser(description='Chord Window by jayis1')
    parser.add_argument('--check', type=Path, help='Validate a recording without opening a window')
    args = parser.parse_args()
    if args.check:
        model = replay(args.check)
        print(f'author=jayis1 rows={len(model.rows)} errors={model.errors} blocked={model.blocked}')
        return 1 if model.errors or model.blocked else 0
    root = tk.Tk()
    Window(root)
    root.mainloop()
    return 0

if __name__ == '__main__':
    raise SystemExit(main())
