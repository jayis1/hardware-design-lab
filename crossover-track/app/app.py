#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Linux Tk companion; local-only, receive-only serial and bounded replay."""
import argparse
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

U32 = 0xffffffff
FIELDS = 'seq now phase levels trial good bad gaps loss id ok reason sl sr fl fr'.split()
REASONS = ['none', 'unordered gates', 'returned to start', 'release skew',
           'trial timeout', 'acquisition gap', 'UART timeout', 'saturation']


def decode(line):
    body, checksum = line.rstrip(b'\r\n').split(b'*')
    if len(checksum) != 4 or binascii.crc_hqx(body, 0xffff) != int(checksum, 16):
        raise ValueError('CRC')
    parts = body.decode('ascii').split(',')
    if parts[0] != 'CT1' or len(parts) != 17:
        raise ValueError('protocol/version')
    if any(not p.isascii() or not p.isdecimal() or len(p) > 10 for p in parts[1:]):
        raise ValueError('integer syntax')
    row = dict(zip(FIELDS, map(int, parts[1:])))
    if any(v > U32 for v in row.values()):
        raise ValueError('integer range')
    if row['phase'] > 2 or row['levels'] > 15 or row['ok'] > 1 or row['reason'] > 7:
        raise ValueError('enumeration')
    if row['good'] + row['bad'] != row['id'] or row['id'] > row['trial']:
        raise ValueError('result counters')
    if row['trial'] - row['id'] != int(row['phase'] == 2):
        raise ValueError('active trial')
    if row['ok']:
        if not row['id'] or row['reason']:
            raise ValueError('validity')
        if min(row['sl'], row['sr']) != 0 or max(row['sl'], row['sr']) > 2000:
            raise ValueError('release offsets')
        if not (row['sl'] < row['fl'] <= 30000 and row['sr'] < row['fr'] <= 30000):
            raise ValueError('travel offsets')
    elif row['id'] and not row['reason']:
        raise ValueError('missing rejection reason')
    return row


class Model:
    def __init__(self):
        self.buffer = bytearray()
        self.current = None
        self.received = None
        self.results = []
        self.errors = 0
        self.blocked = False
        self.warning = ''
        self.study = []
        self.collecting = False
        self.study_after = 0
        self.provenance = 'none'

    def begin_study(self):
        if self.blocked:
            raise ValueError('Reconnect after suspected reset')
        if self.current and self.current['phase'] == 2:
            raise ValueError('Wait for the active trial to finish')
        self.study = []
        self.study_after = self.current['id'] if self.current else 0
        self.collecting = True
        self.warning = ''

    def stale(self, now):
        return self.received is None or now - self.received > 1.0

    def stop_study(self, reason):
        self.collecting = False
        self.warning = reason

    def accept(self, row, now):
        old = self.current
        if self.blocked:
            return
        if self.collecting and self.provenance == 'live' and self.received is not None and self.stale(now):
            self.stop_study('Stale live telemetry; restart study')
        if old:
            dt = (row['now'] - old['now']) & U32
            ds = (row['seq'] - old['seq']) & U32
            if not dt:
                if row != old:
                    self.blocked = True
                    self.stop_study('Conflicting same-time frame; reconnect')
                return
            if dt >= 0x80000000 or not ds or ds >= 0x80000000 or any(
                    row[k] < old[k] for k in ('trial','good','bad','gaps','loss','id')):
                self.blocked = True
                self.stop_study('Possible reset or reordered stream; reconnect')
                return
            if row['id'] == old['id'] and any(row[k] != old[k] for k in ('ok','reason','sl','sr','fl','fr')):
                self.blocked = True
                self.stop_study('Terminal result changed; reconnect')
                return
            if row['gaps'] != old['gaps'] or row['loss'] != old['loss']:
                self.stop_study('Continuity lost; restart four-run study')
        self.current = row
        self.received = now
        if row['id'] and (not old or row['id'] != old['id']):
            self.results.append(row.copy())
            self.results = self.results[-1000:]
            if self.collecting:
                if row['id'] != self.study_after + len(self.study) + 1 or not row['ok']:
                    self.stop_study('Missing/rejected trial; restart study')
                else:
                    self.study.append(row.copy())
                    if len(self.study) == 4:
                        self.collecting = False

    def feed(self, data, now):
        # Search complete marker so a partial UART frame cannot consume its successor.
        for c in data:
            self.buffer.append(c)
            if self.buffer.endswith(b'CT1,') and len(self.buffer) > 4:
                self.errors += 1
                self.buffer[:] = b'CT1,'
            if len(self.buffer) > 512:
                self.errors += 1
                self.buffer.clear()
            elif c == 10:
                try:
                    self.accept(decode(bytes(self.buffer)), now)
                except (ValueError, UnicodeError):
                    self.errors += 1
                self.buffer.clear()

    def analysis(self):
        if len(self.study) != 4:
            return None
        d = [(r['fl']-r['sl'])-(r['fr']-r['sr']) for r in self.study]
        # ABBA refers to the identity in the LEFT lane. Positive cart effect
        # means A takes longer; positive lane effect means left takes longer.
        return ((d[0]-d[1]-d[2]+d[3])/4, sum(d)/4)

    def export(self, path, connected, now):
        # Exclusive creation prevents a mistaken chooser from overwriting data.
        fd = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
        with os.fdopen(fd, 'w', newline='') as f:
            writer = csv.writer(f)
            writer.writerow(['author','provenance','connected','stale','blocked','parser_errors','warning']+FIELDS)
            for row in self.results:
                writer.writerow(['jayis1',self.provenance,connected,self.stale(now),self.blocked,self.errors,self.warning]+[row[k] for k in FIELDS])


def read_replay(path):
    fd = os.open(path, os.O_RDONLY | os.O_NONBLOCK)
    try:
        info = os.fstat(fd)
        if not stat.S_ISREG(info.st_mode) or info.st_size > 2_000_000:
            raise ValueError('Replay must be a regular file no larger than 2 MB')
        with os.fdopen(fd, 'rb', closefd=False) as f:
            data = f.read(2_000_001)
        if len(data) > 2_000_000:
            raise ValueError('Replay grew past limit')
        return data
    finally:
        os.close(fd)


class Serial:
    def __init__(self, path):
        self.fd = os.open(path, os.O_RDONLY | os.O_NONBLOCK | os.O_NOCTTY)
        self.original = None
        try:
            self.original = termios.tcgetattr(self.fd)
            attr = termios.tcgetattr(self.fd)
            attr[0] = attr[1] = attr[3] = 0
            attr[2] = termios.CS8 | termios.CLOCAL | termios.CREAD
            attr[4] = attr[5] = termios.B115200
            attr[6][termios.VMIN] = 0
            attr[6][termios.VTIME] = 0
            termios.tcsetattr(self.fd, termios.TCSANOW, attr)
            termios.tcflush(self.fd, termios.TCIFLUSH)
            self.poller = select.poll()
            self.poller.register(self.fd, select.POLLIN | select.POLLHUP | select.POLLERR)
        except Exception:
            self.close()
            raise

    def read(self):
        if self.fd is None:
            raise OSError('Serial closed')
        events = self.poller.poll(0)
        if any(mask & (select.POLLHUP | select.POLLERR | select.POLLNVAL) for _, mask in events):
            raise OSError('Serial disconnected')
        if events:
            try:
                return os.read(self.fd, 8192)
            except BlockingIOError:
                pass
        return b''

    def close(self):
        if self.fd is not None:
            try:
                if self.original is not None:
                    termios.tcsetattr(self.fd, termios.TCSANOW, self.original)
            except (OSError, termios.error):
                pass
            finally:
                os.close(self.fd)
                self.fd = None


class Window:
    def __init__(self, root):
        self.root = root
        self.model = Model()
        self.port = None
        self.arm_after = None
        root.title('Crossover Track — jayis1')
        root.geometry('1080x620')
        self.tabs = ttk.Notebook(root)
        self.tabs.pack(fill='both', expand=True)
        setup, live, study = [ttk.Frame(self.tabs) for _ in range(3)]
        for page, title in zip((setup,live,study), ('Connection / replay','Gate status / trials','Crossover study')):
            self.tabs.add(page, text=title)
        ttk.Label(setup, text='Receive-only Linux serial. Four voltage-free contacts; no powered sensors.').pack(pady=12)
        self.path = tk.StringVar(value='/dev/ttyACM0')
        ttk.Entry(setup, textvariable=self.path, width=60).pack()
        for label, command in [('Connect (new session)',self.connect),('Disconnect',self.disconnect),('Replay synthetic or recorded CT1 file',self.replay),('Export visible trial evidence',self.export)]:
            ttk.Button(setup, text=label, command=command).pack(pady=6)
        self.status = tk.StringVar(value='Disconnected; no evidence')
        ttk.Label(root, textvariable=self.status, wraplength=1040).pack(fill='x')
        self.gates = tk.StringVar(value='No gate data')
        ttk.Label(live,textvariable=self.gates).pack(pady=8)
        self.table = ttk.Treeview(live,columns=('id','ok','left','right','skew','reason'),show='headings')
        for name in self.table['columns']:
            self.table.heading(name,text=name)
            self.table.column(name,width=140)
        self.table.pack(fill='both',expand=True)
        ttk.Label(study, text='Label carts A and B. Left lane order A, B, B, A; right lane holds the other cart.\nKeep track geometry unchanged. Each run requires BOTH starts closed and finishes clear.\nCart identity is an operator declaration, not sensed. Do not substitute carts mid-study.',wraplength=1000).pack(pady=20)
        ttk.Button(study,text='Begin new four-run study',command=self.begin).pack()
        self.guidance = tk.StringVar(value='No study started')
        ttk.Label(study,textvariable=self.guidance,wraplength=1000).pack(pady=20)
        root.protocol('WM_DELETE_WINDOW',self.close)
        self.timer = root.after(25,self.tick)

    def disconnect(self):
        self.arm_after = None
        if self.port:
            self.port.close()
            self.port = None
            self.model.stop_study('Disconnected; evidence retained, study stopped')

    def connect(self):
        self.disconnect()
        try:
            self.port = Serial(self.path.get())
            self.model = Model()
            self.model.provenance = 'live'
        except (OSError,termios.error) as exc:
            messagebox.showerror('Connection failed',str(exc))

    def replay(self):
        path = filedialog.askopenfilename()
        if not path:
            return
        try:
            data = read_replay(path)
            self.disconnect()
            self.model = Model()
            self.model.provenance = 'replay (not live)'
            self.model.begin_study()
            self.model.feed(data,time.monotonic())
        except (OSError,ValueError) as exc:
            messagebox.showerror('Replay rejected',str(exc))

    def begin(self):
        try:
            if not self.port or self.model.stale(time.monotonic()):
                raise ValueError('Connect to fresh live telemetry before starting a study')
            if self.model.blocked:
                raise ValueError('Reconnect after suspected reset')
            if self.port.fd is None:
                raise OSError('Serial closed')
            termios.tcflush(self.port.fd, termios.TCIFLUSH)
            self.model.buffer.clear()
            self.model.study = []
            self.model.stop_study('Synchronizing: leave both carts at starts for one second')
            self.arm_after = time.monotonic() + 1.0
        except ValueError as exc:
            messagebox.showerror('Study not started',str(exc))
        except (OSError, termios.error) as exc:
            self.disconnect()
            messagebox.showerror('Serial synchronization failed',str(exc))

    def export(self):
        path = filedialog.asksaveasfilename(defaultextension='.csv')
        if path:
            try:
                self.model.export(path,self.port is not None,time.monotonic())
            except OSError as exc:
                messagebox.showerror('Export failed (existing files are not replaced)',str(exc))

    def redraw(self):
        m = self.model
        fresh = self.port is not None and not m.stale(time.monotonic()) and not m.blocked
        self.status.set(f'{m.provenance} | {"LIVE" if fresh else "NOT LIVE / stale or disconnected"} | parser errors {m.errors} | {m.warning}')
        if m.current:
            r = m.current
            names = ['L start','L finish','R start','R finish']
            self.gates.set(' | '.join(f'{name}: {"closed" if r["levels"] & (1<<i) else "open"}' for i,name in enumerate(names)) + f' | phase {r["phase"]} | gaps {r["gaps"]} | UART losses {r["loss"]}')
        else:
            self.gates.set('No gate data')
        existing = self.table.get_children()
        if existing:
            self.table.delete(*existing)
        for r in m.results:
            left = r['fl']-r['sl'] if r['ok'] else 'unknown'
            right = r['fr']-r['sr'] if r['ok'] else 'unknown'
            skew = r['sr']-r['sl'] if r['ok'] else 'unknown'
            self.table.insert('', 'end', values=(r['id'],bool(r['ok']),left,right,skew,REASONS[r['reason']]))
        result = m.analysis()
        if result is not None:
            self.guidance.set(f'Four-run contrast (ms): cart A minus B = {result[0]:.2f}; left minus right lane = {result[1]:.2f}.\nDescriptive only; not a confidence interval. Requires declared ABBA identities and unchanged geometry.')
        elif m.collecting:
            left = ['A','B','B','A'][len(m.study)]
            self.guidance.set(f'Next run {len(m.study)+1}/4: LEFT cart {left}; RIGHT cart {"B" if left == "A" else "A"}. Return both carts to starts.')
        else:
            self.guidance.set(f'Study stopped/not started; {len(m.study)}/4 retained. {m.warning}')

    def tick(self):
        if self.port:
            try:
                self.model.feed(self.port.read(),time.monotonic())
            except OSError as exc:
                self.disconnect()
                self.model.warning = str(exc)
        now = time.monotonic()
        if self.port and self.model.collecting and self.model.stale(now):
            self.model.stop_study('Stale live telemetry; restart study')
        if self.arm_after is not None and now >= self.arm_after:
            r = self.model.current
            if self.model.blocked:
                self.arm_after = None
            elif r and r['phase'] == 1 and self.model.received is not None and self.model.received >= self.arm_after:
                self.model.begin_study()
                self.arm_after = None
        self.redraw()
        self.timer = self.root.after(50,self.tick)

    def close(self):
        self.root.after_cancel(self.timer)
        self.disconnect()
        self.root.destroy()


def replay_valid(model):
    return model.current is not None and not model.errors and not model.blocked


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check-replay',type=Path)
    args = parser.parse_args()
    if args.check_replay:
        model = Model()
        model.provenance = 'replay'
        model.begin_study()
        model.feed(read_replay(args.check_replay),time.monotonic())
        print(f'Author: jayis1; trials={len(model.results)} errors={model.errors} blocked={model.blocked} contrast={model.analysis()}')
        return int(not replay_valid(model))
    root = tk.Tk()
    Window(root)
    root.mainloop()
    return 0

if __name__ == '__main__':
    raise SystemExit(main())
