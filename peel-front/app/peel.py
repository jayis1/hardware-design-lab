#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Local Linux/Tk peel-front acquisition; standard library only."""
import argparse
import binascii
import csv
import os
import re
import select
import stat
import termios
import time
import tkinter as tk
from tkinter import filedialog, messagebox, ttk

ABSENT = 4294967295
REASONS = ('NONE', 'COMPLETE', 'ORDER', 'RECLOSE', 'TIMEOUT', 'GAP')
STATES = ('WAIT', 'READY', 'ACTIVE', 'FAULT')
LIMIT = 4 * 1024 * 1024


def decode(data):
    match = re.fullmatch(rb'PF1,([0-9,]+)\*([0-9A-F]{4})\n', data)
    if not match or len(data) > 255:
        raise ValueError('Invalid PF1 framing')
    body = data[:data.index(b'*')]
    if binascii.crc_hqx(body, 65535) != int(match[2], 16):
        raise ValueError('CRC mismatch')
    fields = match[1].split(b',')
    if len(fields) != 15 or any(len(f) > 10 or not f for f in fields):
        raise ValueError('Field count or width')
    v = tuple(map(int, fields))
    if any(n > ABSENT for n in v):
        raise ValueError('Unsigned overflow')
    ms, state, mask, known, gaps, tx, faults, ident, reason, seen, *rest = v
    at, duration = rest[:4], rest[4]
    if state > 3 or mask > 15 or known > 15 or faults > 7:
        raise ValueError('State range')
    if (state == 3) != bool(faults):
        raise ValueError('Fault state inconsistency')
    if reason > 5 or seen > 15 or duration > 30000:
        raise ValueError('Result range')
    if bool(ident) != bool(reason) or (not reason and (seen or duration)):
        raise ValueError('Result identity')
    observed = []
    for i, offset in enumerate(at):
        if bool(seen & (1 << i)) != (offset != ABSENT):
            raise ValueError('Missing offset')
        if offset != ABSENT:
            if offset > duration:
                raise ValueError('Offset beyond duration')
            observed.append(offset)
    if reason and (not observed or min(observed) != 0):
        raise ValueError('No initial event')
    if reason == 1 and (seen != 15 or max(observed) != duration or
                        at[2] < at[0] or at[3] < at[1]):
        raise ValueError('Impossible complete peel')
    if reason == 4 and duration != 30000:
        raise ValueError('Timeout duration')
    if reason == 2 and not ((seen & 4 and not seen & 1) or
                            (seen & 8 and not seen & 2)):
        raise ValueError('Order failure without missing upstream')
    return v


def metrics(v):
    """Signed right-minus-left skews and same-side station transit times."""
    if v[8] != 1:
        return ('', '', '', '')
    a, b, c, d = v[10:14]
    return b-a, d-c, c-a, d-b


def read_recording(path):
    fd = os.open(path, os.O_RDONLY | os.O_NONBLOCK)
    try:
        info = os.fstat(fd)
        if not stat.S_ISREG(info.st_mode) or info.st_size > LIMIT:
            raise ValueError('Replay requires a regular file under four MiB')
        result = bytearray()
        while True:
            chunk = os.read(fd, min(65536, LIMIT + 1 - len(result)))
            if not chunk:
                break
            result.extend(chunk)
            if len(result) > LIMIT:
                raise ValueError('Recording grew beyond limit')
        return bytes(result)
    finally:
        os.close(fd)


class Model:
    def __init__(self):
        self.buffer = b''
        self.latest = None
        self.rows = []
        self.errors = self.missing = self.evicted = 0
        self.blocked = False
        self.warning = ''
        self.received = None
        self.source = 'none'
        self.connected = False
        self.anchor = None
        self.progress = 0

    def freeze(self, message):
        self.blocked = True
        self.warning = message + '; export then reconnect'

    def accept(self, v, host):
        if self.blocked:
            return
        old = self.latest
        if old is not None:
            elapsed = (v[0] - old[0]) & ABSENT
            if not elapsed:
                if v != old:
                    self.freeze('Contradictory timestamp')
                return
            if elapsed >= 0x80000000 or any(v[i] < old[i] for i in (4,5,7)):
                self.freeze('Suspected device restart')
                return
            if old[6] & ~v[6] or (v[7] == old[7] and v[8:] != old[8:]):
                self.freeze('Evidence changed without identity')
                return
            if self.source == 'live':
                if host - self.received > 1.0:
                    self.freeze('Reception pause; queued data is not fresh')
                    return
                self.progress += elapsed
                host_progress = (host - self.anchor) * 1000
                if abs(self.progress - host_progress) > 500 + host_progress * .03:
                    self.freeze('Host/device progress mismatch')
                    return
            if v[7] > old[7] + 1:
                self.missing += v[7] - old[7] - 1
        else:
            self.anchor = host
        if v[7] and (old is None or v[7] != old[7]):
            self.rows.append(v)
            if len(self.rows) > 1000:
                self.rows.pop(0)
                self.evicted += 1
        self.latest = v
        self.received = host

    def feed(self, data, host=None):
        host = time.monotonic() if host is None else host
        self.buffer += data
        while self.buffer:
            start = self.buffer.find(b'PF1,')
            if start < 0:
                if len(self.buffer) > 3:
                    self.errors += 1
                    self.buffer = self.buffer[-3:]
                return
            if start:
                self.errors += 1
                self.buffer = self.buffer[start:]
            end = self.buffer.find(b'\n')
            another = self.buffer.find(b'PF1,', 4)
            if another >= 0 and (end < 0 or another < end):
                self.errors += 1
                self.buffer = self.buffer[another:]
                continue
            if end < 0:
                if len(self.buffer) > 255:
                    self.errors += 1
                    self.buffer = self.buffer[-3:]
                return
            line, self.buffer = self.buffer[:end+1], self.buffer[end+1:]
            try:
                self.accept(decode(line), host)
            except ValueError:
                self.errors += 1

    def replay(self, path):
        self.source = 'replay'
        self.feed(read_recording(path))
        if self.buffer or self.errors or self.latest is None or self.blocked:
            raise ValueError('Incomplete, corrupt, empty or inconsistent recording')

    def export(self, path):
        fd = os.open(path, os.O_CREAT | os.O_EXCL | os.O_WRONLY, 0o600)
        with os.fdopen(fd, 'w', newline='') as output:
            writer = csv.writer(output)
            age = '' if self.received is None else time.monotonic()-self.received
            writer.writerow(['author','source','connected','age_s','errors','missing',
                             'evicted','blocked','warning'])
            writer.writerow(['jayis1',self.source,self.connected,age,self.errors,
                             self.missing,self.evicted,self.blocked,self.warning])
            writer.writerow(['ms','state','closed','known','gaps','tx_errors','faults',
                             'id','reason','seen','up_left','up_right','down_left',
                             'down_right','duration','up_skew','down_skew','left_dt','right_dt'])
            for row in self.rows:
                writer.writerow([*row,*metrics(row)])


class Serial:
    def __init__(self, path):
        self.fd = os.open(path, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
        self.saved = None
        try:
            self.saved = termios.tcgetattr(self.fd)
            mode = termios.tcgetattr(self.fd)
            mode[0] = mode[1] = mode[3] = 0
            mode[2] = termios.CS8 | termios.CREAD | termios.CLOCAL
            mode[4] = mode[5] = termios.B115200
            mode[6][termios.VMIN] = mode[6][termios.VTIME] = 0
            termios.tcsetattr(self.fd, termios.TCSANOW, mode)
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
        data = b''
        gone = False
        for _, flags in events:
            if flags & select.POLLIN:
                try:
                    data += os.read(self.fd, 4096)
                except BlockingIOError:
                    pass
                except OSError:
                    gone = True
            gone |= bool(flags & (select.POLLHUP | select.POLLERR | select.POLLNVAL))
        return data, gone

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
        root.title('Peel Front — jayis1')
        self.model = Model()
        self.serial = None
        self.path = tk.StringVar(value='/dev/ttyACM0')
        self.summary = tk.StringVar(value='Disconnected; no evidence')
        bar = ttk.Frame(root); bar.pack(fill='x')
        ttk.Entry(bar, textvariable=self.path, width=30).pack(side='left')
        for text, callback in [('Connect',self.connect),('Disconnect',self.disconnect),
                               ('Replay',self.replay),('Export',self.export)]:
            ttk.Button(bar,text=text,command=callback).pack(side='left')
        self.tabs = ttk.Notebook(root); self.tabs.pack(fill='both',expand=True)
        live = ttk.Frame(self.tabs); self.tabs.add(live,text='Live fixture')
        ttk.Label(live,textvariable=self.summary,wraplength=760).pack(padx=15,pady=20)
        trials = ttk.Frame(self.tabs); self.tabs.add(trials,text='Trials')
        names = ('id','reason','up skew','down skew','left transit','right transit','duration')
        self.table = ttk.Treeview(trials,columns=names,show='headings',height=15)
        for name in names:
            self.table.heading(name,text=name)
            self.table.column(name,width=105)
        self.table.pack(fill='both',expand=True)
        setup = ttk.Frame(self.tabs); self.tabs.add(setup,text='Setup and limits')
        ttk.Label(setup,wraplength=730,justify='left',text=(
            'Author: jayis1\nFour dry-contact lift flags: J3 upstream left, J4 upstream right, '
            'J5 downstream left, J6 downstream right. Closed means attached.\n'
            'Wait for READY with all four closed for 100 ms. Peel once; restore all flags '
            'between trials. Five-ms qualification suppresses short openings.\n'
            'Skews are right minus left, in milliseconds. Transit times are downstream '
            'minus upstream on each side. Blank metrics mean rejected trial, not zero.\n'
            'This is NOT a peel-force, fracture-energy or safety instrument. Flag lift may '
            'lag the actual crack; calibrate with video. No physical prototype tested.\n'
            'Connect replaces evidence after successful port open. Export first. Replay is '
            'historical. No device commands, cloud, network, or automatic updates.')).pack(padx=15,pady=20)
        self.job = root.after(25,self.tick)
        root.protocol('WM_DELETE_WINDOW',self.close)

    def connect(self):
        if self.serial is not None:
            return
        try:
            port = Serial(self.path.get())
        except (OSError, termios.error) as exc:
            messagebox.showerror('Connection failed',str(exc)); return
        self.serial = port
        self.model = Model()
        self.model.source = 'live'
        self.model.connected = True
        self.refresh()

    def disconnect(self):
        if self.serial is not None:
            self.serial.close()
            self.serial = None
        self.model.connected = False
        self.refresh()

    def replay(self):
        if self.serial is not None:
            messagebox.showerror('Busy','Disconnect before replay'); return
        path = filedialog.askopenfilename()
        if not path: return
        try:
            replacement = Model(); replacement.replay(path)
        except (OSError, ValueError) as exc:
            messagebox.showerror('Replay rejected',str(exc)); return
        self.model = replacement
        self.refresh()

    def export(self):
        path = filedialog.asksaveasfilename(defaultextension='.csv')
        if not path: return
        try: self.model.export(path)
        except OSError as exc: messagebox.showerror('Export failed',str(exc))

    def refresh(self):
        m = self.model
        v = m.latest
        status = 'no evidence'
        if v is not None:
            assert m.received is not None
            age = time.monotonic() - m.received
            fresh = 'historical' if m.source == 'replay' else (
                'fresh' if m.connected and age <= 1 and not m.blocked else 'STALE')
            status = (f'{STATES[v[1]]}; closed mask {v[2]:04b}, qualified {v[3]:04b}; '
                      f'latest {REASONS[v[8]]} #{v[7]}; {fresh}; '
                      f'gaps {v[4]}, TX errors {v[5]}, faults {v[6]}')
        self.summary.set(f'{m.source} / {"connected" if m.connected else "disconnected"}: '
                         f'{status}\nparser errors {m.errors}; missing {m.missing}; '
                         f'evicted {m.evicted}. {m.warning}')
        self.table.delete(*self.table.get_children())
        for row in m.rows:
            self.table.insert('', 'end', values=(row[7],REASONS[row[8]],*metrics(row),row[14]))

    def tick(self):
        if self.serial is not None:
            data, gone = self.serial.read()
            if data: self.model.feed(data)
            if gone: self.disconnect()
        self.refresh()
        self.job = self.root.after(25,self.tick)

    def close(self):
        self.root.after_cancel(self.job)
        self.disconnect()
        self.root.destroy()


def main():
    args = argparse.ArgumentParser(description=__doc__)
    args.add_argument('--check', metavar='RECORDING')
    options = args.parse_args()
    if options.check:
        model = Model(); model.replay(options.check)
        print(f'PASS PF1 recording: {len(model.rows)} retained trials; author jayis1')
    else:
        root = tk.Tk(); App(root); root.mainloop()


if __name__ == '__main__':
    main()
