# Author: jayis1. SPDX-License-Identifier: MIT
"""Local receive-only Parity Sled workbench. No network or keyboard injection."""
import csv
import os
import sys
import termios
import tkinter as tk
from tkinter import ttk, filedialog, messagebox
from model import FIELDS, Session, parity_odd
from transport import Decoder, Serial, load_replay


class Workbench:
    def __init__(self, root):
        self.root = root
        self.port = None
        self.session = Session('none')
        self.decoder = Decoder()
        self.warning = ''
        self.root.title('Parity Sled — jayis1')
        self.path = tk.StringVar(value='/dev/ttyACM0')
        self.status = tk.StringVar(value='Disconnected; no evidence')
        bar = ttk.Frame(root); bar.pack(fill='x')
        ttk.Entry(bar, textvariable=self.path, width=32).pack(side='left')
        for text, action in [('Connect', self.connect), ('Disconnect', self.disconnect),
                             ('Replay', self.replay_dialog), ('Export CSV', self.export_dialog)]:
            ttk.Button(bar, text=text, command=action).pack(side='left')
        ttk.Label(root, textvariable=self.status, wraplength=850).pack(fill='x')
        book = ttk.Notebook(root); book.pack(fill='both', expand=True)
        live = ttk.Frame(book); history = ttk.Frame(book); about = ttk.Frame(book)
        book.add(live, text='Parity and recovery')
        book.add(history, text='Exposure histogram')
        book.add(about, text='Protocol and limits')
        self.cells = []
        for i in range(4):
            label = tk.Label(live, text=f'{("DATA0","DATA1","DATA2","PARITY")[i]}: unknown', width=34, height=5,
                             relief='groove', bg='#cccccc')
            label.grid(row=i//2, column=i%2, padx=5, pady=5)
            self.cells.append(label)
        self.summary = tk.StringVar(value='No samples')
        ttk.Label(live, textvariable=self.summary, wraplength=700).grid(row=2, column=0, columnspan=2)
        self.table = ttk.Treeview(history, columns=('word','data','parity','total','segments','peak'), show='headings', height=16)
        for col in self.table['columns']:
            self.table.heading(col, text=col)
            self.table.column(col, width=110)
        self.table.pack(fill='both', expand=True)
        ttk.Label(about, justify='left', wraplength=800, text=(
            'Author: jayis1\nPY1 / 115200 baud / 8N1 / five unchanged intervals.\n'
            'Green: physical closure. Gray: open or unqualified.\n'
            'Only detached independent dry contacts. NEVER attach powered machine wiring.\n'
            'Three data lanes plus EVEN parity. Odd words are detected, never corrected.\n'
            'Two-bit errors and missing entire positions can remain invisible.\n'
            'Recovery same/changed compares even codewords, not physical strip identity.\n'
            'Histograms count clean intervals; episode age includes unsettled transitions.\n'
            'Replay is synthetic/offline evidence, never live. Export retains provenance\n'
            'and errors; filenames are never overwritten. Reset hardware and reconnect\n'
            'for a new volatile experiment. Read README before wiring.')).pack(padx=15,pady=15)
        root.protocol('WM_DELETE_WINDOW', self.close)
        self.timer = root.after(25, self.poll)

    def report(self, exc):
        self.warning = str(exc)
        self.render()

    def connect(self):
        if self.port is not None:
            self.report('Already connected; disconnect first')
            return
        try:
            port = Serial(self.path.get())
        except (OSError, termios.error) as exc:
            self.report(exc)
            return
        self.port = port
        self.session = Session('live')
        self.decoder = Decoder()
        self.warning = ''
        self.render()

    def disconnect(self):
        if self.port is not None:
            self.port.close()
            self.port = None
        self.render()

    def replay(self, path):
        # Parse fully before replacing the previous evidence or closing serial.
        session = load_replay(path)
        self.disconnect()
        self.session = session
        self.decoder = Decoder()
        self.warning = ''
        self.render()

    def replay_dialog(self):
        path = filedialog.askopenfilename()
        if path:
            try:
                self.replay(path)
            except (OSError, ValueError) as exc:
                self.report(exc)

    def export(self, path):
        # O_EXCL protects existing files and mode 0600 keeps local captures private.
        fd = os.open(path, os.O_WRONLY|os.O_CREAT|os.O_EXCL, 0o600)
        with os.fdopen(fd, 'w', newline='') as handle:
            writer = csv.writer(handle)
            writer.writerow(['author','jayis1','source',self.session.source,
                             'connected',self.port is not None,
                             'fresh',self.session.fresh(connected=self.port is not None),
                             'parser_errors',self.decoder.errors,
                             'initial_fragments',self.decoder.initial_fragments,
                             'warning',self.warning,'session_error',self.session.error])
            writer.writerow(FIELDS)
            writer.writerows([row[k] for k in FIELDS] for row in self.session.rows)

    def export_dialog(self):
        path = filedialog.asksaveasfilename(defaultextension='.csv')
        if path:
            try:
                self.export(path)
            except OSError as exc:
                self.report(exc)

    def render(self):
        fresh = self.session.fresh(connected=self.port is not None)
        state = 'LIVE acquisition' if fresh else ('CONNECTED / unconfirmed or stale' if self.port else 'DISCONNECTED / offline')
        self.status.set(f'{state} | source={self.session.source} | errors={self.decoder.errors} | {self.warning or self.session.error}')
        self.table.delete(*self.table.get_children())
        if not self.session.rows:
            self.summary.set('No samples')
            for i, label in enumerate(self.cells):
                label.config(text=f'{("DATA0","DATA1","DATA2","PARITY")[i]}: unknown', bg='#cccccc')
            return
        row = self.session.rows[-1]
        trusted = row['valid'] and not row['fault']
        physical = row['qualified']
        classification = ('ODD / detected' if parity_odd(physical) else 'EVEN / not proof of correctness') if trusted else 'UNKNOWN / halted or settling'
        for i, label in enumerate(self.cells):
            closed = physical & (1 << i)
            text = ('CLOSED' if closed else 'open') if trusted else 'unqualified / halted'
            color = ('#a8e6a1' if closed else '#dddddd') if trusted else '#cccccc'
            label.config(text=f'{("DATA0","DATA1","DATA2","PARITY")[i]}: {text}', bg=color)
        self.summary.set(f"Retained device ms={row['now']} | acquired={row['samples']} | unknown={row['unknown']} | missed={row['missed']}\n"
                         f"raw={row['raw']:04b} | qualified={physical:04b} | current clean run={row['run']} | fault={row['fault']}\n"
                         f"{classification}\n"
                         f"episodes={row['starts']} | same={row['same']} | changed={row['changed']} | unanchored={row['orphan']} | aborted={row['aborted']}\n"
                         f"active={row['active']} age={row['age']} | latest recovery kind={row['last_kind']} duration={row['last_age']} ms\n"
                         'Latest retained evidence, not proof of present strip position.')
        for i in range(16):
            self.table.insert('', 'end', values=(f'{i:04b}',i & 7,'ODD' if parity_odd(i) else 'EVEN',
                              row[f'bins{i}'],row[f'segments{i}'],row[f'peaks{i}']))

    def poll(self):
        if self.port is not None:
            try:
                data = self.port.read()
                for row in self.decoder.feed(data):
                    self.session.accept(row)
            except (OSError, ValueError, termios.error) as exc:
                self.warning = str(exc)
                self.disconnect()
        self.render()
        self.timer = self.root.after(25, self.poll)

    def close(self):
        self.root.after_cancel(self.timer)
        self.disconnect()
        self.root.destroy()


if __name__ == '__main__':
    window = tk.Tk()
    app = Workbench(window)
    if len(sys.argv) == 2:
        try:
            app.replay(sys.argv[1])
        except (OSError, ValueError) as exc:
            messagebox.showerror('Replay rejected', str(exc))
    window.mainloop()
