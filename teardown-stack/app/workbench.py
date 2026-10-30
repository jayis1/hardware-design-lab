#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Linux/Tk local companion; no dependencies, network, or device commands."""
import csv
import json
import os
import select
import termios
import time
import tkinter as tk
from tkinter import ttk, filedialog, messagebox
from protocol import FIELDS, Session, read_replay

class Serial:
    def __init__(self, path):
        self.fd = os.open(path, os.O_RDONLY | os.O_NOCTTY | os.O_NONBLOCK)
        self.old = None
        try:
            self.old = termios.tcgetattr(self.fd)
            cfg = termios.tcgetattr(self.fd)
            cfg[0] = cfg[1] = cfg[3] = 0
            cfg[2] = termios.CS8 | termios.CREAD | termios.CLOCAL
            cfg[4] = cfg[5] = termios.B115200
            cfg[6][termios.VMIN] = 0
            cfg[6][termios.VTIME] = 0
            termios.tcsetattr(self.fd, termios.TCSANOW, cfg)
            termios.tcflush(self.fd, termios.TCIFLUSH)
            self.poller = select.poll()
            self.poller.register(self.fd, select.POLLIN | select.POLLHUP | select.POLLERR)
        except Exception:
            self.close()
            raise

    def read(self):
        if self.fd is None:
            raise OSError('Serial port is closed')
        data = b''
        hungup = False
        for _, flags in self.poller.poll(0):
            if flags & select.POLLIN:
                try:
                    data += os.read(self.fd, 8192)
                except BlockingIOError:
                    pass
            hungup |= bool(flags & (select.POLLHUP | select.POLLERR | select.POLLNVAL))
        return data, hungup

    def close(self):
        if self.fd is None:
            return
        try:
            if self.old is not None:
                termios.tcsetattr(self.fd, termios.TCSANOW, self.old)
        except (OSError, termios.error):
            pass
        finally:
            os.close(self.fd)
            self.fd = None

def export(session, path):
    """Exclusive creation prevents silent overwrite. Session facts survive disconnect."""
    fd = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
    with os.fdopen(fd, 'w', newline='') as stream:
        writer = csv.writer(stream)
        writer.writerow(['author', 'jayis1'])
        writer.writerow(['session', json.dumps({
            'provenance': session.provenance, 'connected': session.connected,
            'fresh': session.fresh(), 'errors': session.errors,
            'blocked': session.blocked, 'warning': session.warning,
            'age_seconds': None if session.last_host is None else max(0,time.monotonic()-session.last_host),
            'retained_limit': 1000})])
        writer.writerow(FIELDS)
        for r in session.rows:
            writer.writerow([r[k] for k in FIELDS])

class Workbench:
    def __init__(self, root):
        self.root = root
        self.serial = None
        self.session = Session('none')
        self.root.title('Teardown Stack — jayis1')
        self.root.geometry('1000x630')
        self.tabs = ttk.Notebook(root)
        self.tabs.pack(fill='both', expand=True)
        self.setup = ttk.Frame(self.tabs, padding=12)
        self.live = ttk.Frame(self.tabs, padding=12)
        self.history = ttk.Frame(self.tabs, padding=12)
        for frame, name in [(self.setup,'Connect / fixture'),(self.live,'Live stack'),(self.history,'History / export')]:
            self.tabs.add(frame, text=name)
        ttk.Label(self.setup, text='Four labelled removable parts cups; closed = cup docked.\n'
                  'Start with all docks empty. Deposit cups in disassembly order; retrieve in reverse.\n'
                  'This records cup movement, not part identity, screw count or correct reassembly.\n'
                  'J3=A / PA0, J4=B / PA1, J5=C / PA4, J6=D / PB0.\n'
                  'Only voltage-free contacts; shared USB ground; no powered equipment.\n'
                  'Fixed 10 ms qualification. No calibration or control commands.').pack(anchor='w', pady=12)
        self.path = tk.StringVar(value='/dev/ttyACM0')
        ttk.Entry(self.setup, textvariable=self.path, width=65).pack(anchor='w')
        ttk.Button(self.setup,text='Connect (new evidence session)',command=self.connect_dialog).pack(anchor='w',pady=5)
        ttk.Button(self.setup,text='Disconnect (retain evidence)',command=self.disconnect).pack(anchor='w')
        ttk.Button(self.setup,text='Open recording (replay, never live)',command=self.replay_dialog).pack(anchor='w',pady=5)
        self.status = tk.StringVar()
        ttk.Label(root,textvariable=self.status,wraplength=970).pack(fill='x',padx=10,pady=10)
        self.summary = tk.StringVar(value='No evidence')
        ttk.Label(self.live,textvariable=self.summary,font=('TkDefaultFont',14),justify='left').pack(anchor='w')
        self.tree = ttk.Treeview(self.history,columns=('time','state','stack','done','bad','loss'),show='headings')
        for column in self.tree['columns']:
            self.tree.heading(column,text=column)
            self.tree.column(column,width=125)
        scroll = ttk.Scrollbar(self.history,orient='vertical',command=self.tree.yview)
        self.tree.configure(yscrollcommand=scroll.set)
        scroll.pack(side='right',fill='y')
        self.tree.pack(fill='both',expand=True)
        ttk.Button(self.history,text='Export retained evidence CSV',command=self.export_dialog).pack(pady=5)
        root.protocol('WM_DELETE_WINDOW',self.close)
        self.timer = root.after(25,self.tick)
        self.render()

    def connect(self, path):
        if self.serial is not None:
            raise ValueError('Disconnect before opening another port')
        port = Serial(path)  # Preserve retained evidence if opening fails.
        self.serial = port
        self.session = Session('live')
        self.session.connected = True
        self.render()

    def connect_dialog(self):
        try: self.connect(self.path.get())
        except (ValueError,OSError,termios.error) as exc: messagebox.showerror('Connect failed',str(exc))

    def disconnect(self):
        if self.serial:
            self.serial.close()
        self.serial = None
        self.session.connected = False
        self.render()

    def replay(self, path):
        if self.serial:
            raise ValueError('Disconnect before replay')
        candidate = Session('replay')
        candidate.feed(read_replay(path))
        candidate.finish()
        self.session = candidate
        self.render()

    def replay_dialog(self):
        path = filedialog.askopenfilename()
        if not path: return
        try: self.replay(path)
        except (OSError,ValueError) as exc: messagebox.showerror('Replay rejected',str(exc))

    def export_dialog(self):
        path = filedialog.asksaveasfilename(defaultextension='.csv')
        if not path: return
        try: export(self.session,path)
        except OSError as exc: messagebox.showerror('Export failed',str(exc))

    def tick(self):
        if self.serial:
            try:
                data, hungup = self.serial.read()
                if data: self.session.feed(data)
                if hungup:
                    self.disconnect()
                    self.session.warning = 'Serial disconnected; evidence retained'
            except (OSError,termios.error) as exc:
                self.disconnect()
                self.session.warning = str(exc)
        self.render()
        self.timer = self.root.after(25,self.tick)

    def render(self):
        s = self.session
        freshness = 'LIVE progress' if s.fresh() else 'NOT LIVE / retained evidence'
        self.status.set(f'{s.provenance} | {freshness} | parser errors {s.errors} | {s.warning}')
        if not s.rows:
            self.summary.set('No evidence — connect and establish an empty-dock baseline')
        else:
            r = s.rows[-1]
            stack = ' → '.join('ABCD'[r[f'stack{i}']] for i in range(r['depth'])) or '(empty)'
            top = 'ABCD'[r[f'stack{r["depth"]-1}']] if r['depth'] else 'none'
            self.summary.set(f'State: {("WAIT", "READY", "ACTIVE", "HALT")[r["state"]]}\n'
                             f'Deposit stack: {stack}\nNext reverse retrieval: {top}\n'
                             f'Completed cup sequences: {r["completed"]}\n'
                             f'Aborted sequences: {r["aborted"]}\nOrder errors: {r["order_errors"]}\n'
                             f'Ambiguous movements: {r["collisions"]}\nSampling gaps: {r["gaps"]}\n'
                             f'Fault mask: {r["fault"]} — not an assembly-quality certificate')
        # Bounded history includes every accepted snapshot, including batched reads.
        signature = (id(s),len(s.rows),s.rows[-1]['now'] if s.rows else None)
        if getattr(self,'signature',None) != signature:
            self.signature = signature
            self.tree.delete(*self.tree.get_children())
            for r in s.rows:
                stack = ''.join('ABCD'[r[f'stack{i}']] for i in range(r['depth']))
                self.tree.insert('', 'end', values=(r['now'],r['state'],stack,r['completed'],r['aborted'],r['gaps']))

    def close(self):
        self.root.after_cancel(self.timer)
        self.disconnect()
        self.root.destroy()

if __name__ == '__main__':
    root = tk.Tk()
    Workbench(root)
    root.mainloop()
