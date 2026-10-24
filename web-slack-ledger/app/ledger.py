#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""WebSlack Ledger: Linux USB serial or explicitly labelled file replay."""
import binascii
from collections import deque
import csv
import glob
import math
import os
from pathlib import Path
import re
import stat
import termios
import time

FIELDS = ('sequence','time','up','down','difference','invalid_up','invalid_down',
          'gaps','reversals','peak','leg_ms','idle_ms','flags','transport_errors')

def parse(line):
    if len(line) > 256:
        raise ValueError('Oversized frame')
    try:
        body, check = line.strip().rsplit(b'*', 1)
        if not re.fullmatch(b'[0-9A-F]{4}', check):
            raise ValueError('Bad checksum syntax')
        if binascii.crc_hqx(body, 65535) != int(check, 16):
            raise ValueError('CRC mismatch')
        words = body.decode('ascii').split(',')
        if len(words) != 15 or words[0] != 'WSL1':
            raise ValueError('Unknown protocol or field count')
        if any(not re.fullmatch(r'-?(0|[1-9][0-9]*)', w) for w in words[1:]):
            raise ValueError('Noncanonical number')
        row = dict(zip(FIELDS, map(int, words[1:])))
    except (UnicodeError, TypeError) as error:
        raise ValueError('Non-ASCII frame') from error
    for key, value in row.items():
        if key in ('up','down'):
            low, high = -2147483648, 2147483647
        elif key == 'difference':
            low, high = -4294967295, 4294967295
        elif key == 'peak':
            low, high = 0, 8589934590
        else:
            low, high = 0, 4294967295
        if not low <= value <= high:
            raise ValueError('Field out of range: ' + key)
    if row['difference'] != row['up'] - row['down'] or row['flags'] > 15:
        raise ValueError('Inconsistent telemetry')
    expected = bool(row['gaps']) | (bool(row['invalid_up']) << 1) | (bool(row['invalid_down']) << 2)
    if row['flags'] & 7 != expected:
        raise ValueError('Inconsistent quality flags')
    return row

class Stream:
    def __init__(self):
        self.pending = b''
        self.errors = 0

    def feed(self, chunk):
        self.pending += chunk
        rows = []
        while b'\n' in self.pending:
            line, self.pending = self.pending.split(b'\n', 1)
            # The entire marker cannot occur inside decimal fields or CRC.
            offset = line.rfind(b'WSL1,')
            try:
                if offset < 0:
                    raise ValueError('No frame marker')
                rows.append(parse(line[offset:]))
            except ValueError:
                self.errors += 1
        if len(self.pending) > 256:
            self.pending = self.pending[-256:]
            self.errors += 1
        return rows

class Model:
    def __init__(self):
        self.rows = deque(maxlen=6000)
        self.last_received = None
        self.missing = 0

    def accept(self, row, received):
        if self.rows:
            previous = self.rows[-1]
            sequence_delta = (row['sequence'] - previous['sequence']) & 0xffffffff
            time_delta = (row['time'] - previous['time']) & 0xffffffff
            if not 0 < sequence_delta < 0x80000000 or not 0 < time_delta < 0x80000000:
                raise ValueError('Duplicate/reset/reordered data: reconnect to start a new session')
            self.missing += sequence_delta - 1
        self.rows.append(dict(row, received=received))
        self.last_received = received

    def stale(self, now):
        return self.last_received is None or now - self.last_received > 1.0

    def export(self, path, source, connected, up_scale, down_scale):
        # Exclusive creation prevents silent destruction; exports are user-private.
        descriptor = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
        with os.fdopen(descriptor, 'w', newline='') as file:
            keys = ('author','source','connected','age_seconds','up_um_per_count',
                    'down_um_per_count','differential_mm') + FIELDS
            writer = csv.DictWriter(file, fieldnames=keys)
            writer.writeheader()
            now = time.monotonic()
            for row in self.rows:
                record = {k: row[k] for k in FIELDS}
                record.update(author='jayis1', source=source, connected=connected,
                              age_seconds=max(0, now-row['received']),
                              up_um_per_count=up_scale, down_um_per_count=down_scale,
                              differential_mm=(row['up']*up_scale-row['down']*down_scale)/1000)
                writer.writerow(record)

class App:
    def __init__(self, root):
        import tkinter as tk
        from tkinter import ttk
        self.tk = tk
        self.root = root
        self.fd = None
        self.model = Model()
        self.stream = Stream()
        self.source = 'NO DATA'
        self.error = ''
        self.up_scale = 250.0
        self.down_scale = 250.0
        root.title('WebSlack Ledger — jayis1')
        root.geometry('900x650')
        self.status = tk.StringVar(value='Disconnected')
        ttk.Label(root, textvariable=self.status).pack(fill='x')
        tabs = ttk.Notebook(root)
        tabs.pack(fill='both', expand=True)
        self.tabs = tabs
        connect, live, settings = [ttk.Frame(tabs) for _ in range(3)]
        for frame, title in zip((connect, live, settings), ('Connect / Replay','Differential travel','Calibration / Export')):
            tabs.add(frame, text=title)
        ttk.Label(connect, text='USB serial, 115200 8N1. Only read telemetry; no motor control.').pack()
        self.port = tk.StringVar(value='')
        self.ports = ttk.Combobox(connect, textvariable=self.port, width=65)
        self.ports.pack()
        ttk.Button(connect, text='Refresh ports', command=self.refresh).pack()
        ttk.Button(connect, text='Connect (new session)', command=self.connect).pack()
        ttk.Button(connect, text='Disconnect', command=self.disconnect).pack()
        ttk.Button(connect, text='Replay capture (NOT LIVE)', command=self.replay_dialog).pack()
        ttk.Label(connect, text='Reconnect clears in-memory history. Export before reconnecting.').pack()
        self.summary = tk.StringVar(value='No samples')
        ttk.Label(live, textvariable=self.summary, justify='left').pack(fill='x')
        self.canvas = tk.Canvas(live, width=840, height=390, bg='#101820')
        self.canvas.pack(fill='both', expand=True)
        ttk.Label(settings, text='Scale is micrometres per quadrature edge, not per full pulse.\nNominal 100 mm circumference / 400 edges = 250 µm/count.\nCalibration affects display/export only; firmware remains in raw counts.').pack()
        self.scale_up = tk.StringVar(value='250')
        self.scale_down = tk.StringVar(value='250')
        for label, var in [('Upstream µm/count',self.scale_up), ('Downstream µm/count',self.scale_down)]:
            ttk.Label(settings, text=label).pack()
            ttk.Entry(settings, textvariable=var).pack()
        ttk.Button(settings, text='Apply scales', command=self.calibrate).pack()
        ttk.Button(settings, text='Export private CSV (new filename)', command=self.export_dialog).pack()
        ttk.Label(settings, text='Apparent accumulation is not proof of slack, strain, or slip.\nAny quality flag invalidates quantitative interpretation until hardware reset.').pack()
        self.refresh()
        root.protocol('WM_DELETE_WINDOW', self.close)
        root.after(50, self.poll)

    def refresh(self):
        self.ports['values'] = sorted(glob.glob('/dev/ttyACM*') + glob.glob('/dev/ttyUSB*'))

    def disconnect(self):
        if self.fd is not None:
            os.close(self.fd)
            self.fd = None
        self.draw()

    def connect(self):
        self.disconnect()
        descriptor = None
        try:
            path = self.port.get()
            if not stat.S_ISCHR(os.stat(path).st_mode):
                raise ValueError('Select a serial character device')
            descriptor = os.open(path, os.O_RDONLY | os.O_NOCTTY | os.O_NONBLOCK)
            attrs = termios.tcgetattr(descriptor)
            attrs[0] = attrs[1] = attrs[3] = 0
            attrs[2] = termios.CS8 | termios.CREAD | termios.CLOCAL
            attrs[4] = attrs[5] = termios.B115200
            attrs[6][termios.VMIN] = 1
            attrs[6][termios.VTIME] = 0
            termios.tcsetattr(descriptor, termios.TCSANOW, attrs)
            termios.tcflush(descriptor, termios.TCIFLUSH)
            self.fd = descriptor
            self.model = Model()
            self.stream = Stream()
            self.source = 'LIVE USB'
            self.error = ''
        except (OSError, ValueError, termios.error) as error:
            if descriptor is not None:
                os.close(descriptor)
            self.error = str(error)
        self.draw()

    def replay_dialog(self):
        from tkinter import filedialog
        path = filedialog.askopenfilename()
        if path:
            try:
                self.load_replay(path)
            except (OSError, ValueError) as error:
                self.error = str(error)
                self.draw()

    def load_replay(self, path):
        if Path(path).stat().st_size > 2_000_000:
            raise ValueError('Replay limited to 2 MB')
        data = Path(path).read_bytes()
        stream, model = Stream(), Model()
        for row in stream.feed(data):
            model.accept(row, time.monotonic())
        if not model.rows:
            raise ValueError('No valid frames in replay')
        self.disconnect()
        self.model, self.stream = model, stream
        self.source = 'REPLAY — NOT LIVE'
        self.error = ''
        self.draw()

    def calibrate(self):
        try:
            up, down = float(self.scale_up.get()), float(self.scale_down.get())
            if not all(math.isfinite(x) and 1 <= x <= 10000 for x in (up, down)):
                raise ValueError('Scales must be finite, 1..10000 µm/count')
            self.up_scale, self.down_scale = up, down
            self.error = ''
        except ValueError as error:
            self.error = str(error)
        self.draw()

    def export_dialog(self):
        from tkinter import filedialog
        path = filedialog.asksaveasfilename(defaultextension='.csv')
        if path:
            try:
                self.model.export(path, self.source, self.fd is not None, self.up_scale, self.down_scale)
                self.error = 'Exported: ' + path
            except OSError as error:
                self.error = str(error)
            self.draw()

    def poll(self):
        if self.fd is not None:
            try:
                data = os.read(self.fd, 4096)
                if not data:
                    raise OSError('Serial device disconnected')
                for row in self.stream.feed(data):
                    self.model.accept(row, time.monotonic())
            except BlockingIOError:
                pass
            except (OSError, ValueError) as error:
                self.error = str(error)
                self.disconnect()
        self.draw()
        self.root.after(50, self.poll)

    def draw(self):
        stale = self.model.stale(time.monotonic())
        state = 'STALE' if stale else 'receiving'
        if self.fd is None:
            state = 'not connected'
        self.status.set(f'{self.source} | {state} | bad frames {self.stream.errors} | {self.error}')
        self.canvas.delete('all')
        if not self.model.rows:
            self.summary.set('No samples')
            return
        row = self.model.rows[-1]
        differential = (row['up']*self.up_scale-row['down']*self.down_scale)/1000
        self.summary.set(f"Upstream {row['up']} edges; downstream {row['down']} edges\n"
                         f"Apparent accumulation {differential:.3f} mm; quality flags {row['flags']}\n"
                         f"Reversals {row['reversals']}; previous-leg peak {row['peak']} raw differential counts\n"
                         f"Invalid transitions {row['invalid_up']}/{row['invalid_down']}; sampling gaps {row['gaps']}; "
                         f"missing frames {self.model.missing}; transport errors {row['transport_errors']}")
        for x, label in [(130, 'UPSTREAM'),(700, 'DOWNSTREAM')]:
            self.canvas.create_oval(x-35,30,x+35,100,outline='white',width=3)
            self.canvas.create_text(x,115,text=label,fill='white')
        self.canvas.create_line(165,65,665,65,fill='#67d5ca',width=3,arrow='last')
        self.canvas.create_text(420,35,text=f'Accumulation estimate: {differential:+.3f} mm',fill='white')
        rows = list(self.model.rows)[-200:]
        values = [(r['up']*self.up_scale-r['down']*self.down_scale)/1000 for r in rows]
        extent = max(1.0, max(abs(v) for v in values))
        self.canvas.create_line(40,250,800,250,fill='#777777')
        self.canvas.create_text(150,150,text=f'Window scale ±{extent:.3f} mm',fill='white')
        coordinates = []
        for index, value in enumerate(values):
            coordinates.extend((40+760*index/max(1,len(values)-1),250-85*value/extent))
        if len(coordinates) >= 4:
            self.canvas.create_line(*coordinates,fill='#ffb454' if row['flags'] else '#67d5ca',width=2)

    def close(self):
        self.disconnect()
        self.root.destroy()

if __name__ == '__main__':
    import tkinter as tk
    root = tk.Tk()
    App(root)
    root.mainloop()
