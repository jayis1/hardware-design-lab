#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Linux/Tk receive-only Merge Rhythm notebook. No third-party packages."""
import csv
import os
import select
import termios
import time
import tkinter as tk
from tkinter import ttk, filedialog, messagebox
from protocol import FIELDS, Stream, Session, replay

class Serial:
    def __init__(self, path):
        self.fd = os.open(path, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
        self.original = None
        try:
            self.original = termios.tcgetattr(self.fd)
            settings = termios.tcgetattr(self.fd)
            settings[0] = settings[1] = settings[3] = 0
            settings[2] = termios.CLOCAL | termios.CREAD | termios.CS8
            settings[4] = settings[5] = termios.B115200
            settings[6][termios.VMIN] = settings[6][termios.VTIME] = 0
            termios.tcsetattr(self.fd, termios.TCSANOW, settings)
            termios.tcflush(self.fd, termios.TCIFLUSH)
            self.poller = select.poll()
            self.poller.register(self.fd, select.POLLIN | select.POLLHUP | select.POLLERR)
        except Exception:
            self.close()
            raise
    def read(self):
        if self.fd is None:
            return b'', True
        data, dead = b'', False
        for _, flags in self.poller.poll(0):
            if flags & select.POLLIN:
                try:
                    data = os.read(self.fd, 8192)
                except BlockingIOError:
                    pass
                except OSError:
                    dead = True
            dead |= bool(flags & (select.POLLHUP | select.POLLERR | select.POLLNVAL))
        return data, dead
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

class Workbench:
    def __init__(self, root):
        self.root = root
        root.title('Merge Rhythm — jayis1')
        root.geometry('1060x600')
        self.serial = None
        self.session = Session('none')
        self.stream = Stream()
        self.path = tk.StringVar(value='/dev/ttyACM0')
        self.status = tk.StringVar(value='Disconnected')
        self.live = tk.StringVar(value='No observations')
        self.tabs = ttk.Notebook(root)
        self.tabs.pack(fill='both', expand=True)
        setup, live, history = [ttk.Frame(self.tabs, padding=12) for _ in range(3)]
        self.tabs.add(setup, text='Connection / baseline')
        self.tabs.add(live, text='Interleaving / transit')
        self.tabs.add(history, text='History / export')
        ttk.Label(setup, text='Author: jayis1\n'
                  'J3 LEFT, J4 RIGHT, J5 EXIT; J6 CLEAR pushbutton.\n'
                  'Only disconnected voltage-free contacts. No motor control.\n'
                  'Empty the chute physically. Release all gates and CLEAR.\n'
                  'Hold CLEAR for at least 120 ms, then release it before feeding.\n'
                  'No overtaking is essential: source identity is inferred FIFO, not measured.\n'
                  'Armed does not prove that a hidden chute section is empty.',
                  justify='left').pack(anchor='w', pady=15)
        ttk.Entry(setup, textvariable=self.path, width=70).pack(anchor='w')
        for text, action in [('Connect', lambda:self.action(self.connect,self.path.get())),
                             ('Disconnect', self.disconnect), ('Replay MR1 file', self.choose_replay)]:
            ttk.Button(setup, text=text, command=action).pack(anchor='w', pady=4)
        ttk.Label(live, textvariable=self.live, justify='left', font=('TkDefaultFont',14)).pack(anchor='w')
        ttk.Label(live, text='Cumulative data survive rearming but not power loss.\n'
                  'Streaks restart after correlation loss. Alternation is not randomness or fairness.\n'
                  'Only accepted transits contribute latency; failures remain separate.').pack(anchor='w',pady=20)
        self.columns = ('now','state','pending','left','right','longest','abandoned','gaps')
        self.history = ttk.Treeview(history, columns=self.columns, show='headings')
        for name in self.columns:
            self.history.heading(name, text=name)
            self.history.column(name,width=110)
        self.history.pack(fill='both',expand=True)
        ttk.Button(history,text='Export new CSV',command=self.choose_export).pack(anchor='w')
        ttk.Label(root,textvariable=self.status,wraplength=1000).pack(fill='x')
        root.protocol('WM_DELETE_WINDOW',self.close)
        self.timer=root.after(25,self.poll)
    def action(self, function, *args):
        try:
            function(*args)
        except (ValueError,OSError,termios.error) as error:
            messagebox.showerror('Merge Rhythm',str(error),parent=self.root)
    def connect(self,path):
        if self.serial:
            raise ValueError('Disconnect before another connection')
        port=Serial(path)
        self.serial=port
        self.session=Session('live')
        self.stream=Stream()
        self.refresh()
    def disconnect(self):
        if self.serial:
            if self.stream.buffer:
                self.stream.errors+=1
                self.stream.buffer=b''
            self.session.errors=self.stream.errors
            self.serial.close()
            self.serial=None
        self.session.connected=False
        self.refresh()
    def load(self,path):
        if self.serial:
            raise ValueError('Disconnect before replay')
        candidate=replay(path)
        self.session=candidate
        self.refresh()
    def choose_replay(self):
        path=filedialog.askopenfilename(parent=self.root)
        if path:
            self.action(self.load,path)
    def choose_export(self):
        path=filedialog.asksaveasfilename(parent=self.root,defaultextension='.csv')
        if path:
            self.action(self.export,path)
    def export(self,path):
        fd=os.open(path,os.O_WRONLY|os.O_CREAT|os.O_EXCL,0o600)
        with os.fdopen(fd,'w',newline='') as file:
            writer=csv.writer(file)
            writer.writerow(['author','jayis1','provenance',self.session.provenance,
                             'connected',self.session.connected,
                             'fresh',self.session.fresh(time.monotonic()),
                             'errors',self.session.errors,'evicted',self.session.evicted,
                             'warning',self.session.warning])
            writer.writerow(FIELDS)
            for row in self.session.rows:
                writer.writerow([row[k] for k in FIELDS])
    def refresh(self):
        s=self.session
        self.status.set(f'{s.provenance}: connected={s.connected}; live progress={s.fresh(time.monotonic())}; '
                        f'errors={s.errors}; history evicted={s.evicted}; {s.warning}')
        if s.rows:
            r=s.rows[-1]
            done=r['left']+r['right']
            mean=f"{r['latency_sum']/done:.1f}" if done else 'unavailable'
            self.live.set(f"State: {('WAIT','ARMED','HALT')[r['state']]}  Contacts: {r['mask']:04b}  Known: {r['known']:04b}\n"
                          f"Delivered L: {r['left']}   R: {r['right']}   Pending: {r['pending']}\n"
                          f"Alternations: {r['alternations']}   Current streak: {r['streak']}   Longest: {r['longest']}\n"
                          f"Accepted latency mean: {mean} ms   Maximum: {r['latency_max']} ms\n"
                          f"Admitted: {r['admitted']}   Abandoned: {r['abandoned']}   Rejected inputs: {r['rejected']}\n"
                          f"Ambiguous: {r['ambiguous']}   Orphan exits: {r['orphans']}   Full queue: {r['full']}\n"
                          f"Timeouts: {r['timeouts']}   Segments: {r['segments']}   Gaps: {r['gaps']}\n"
                          f"TX errors: {r['tx']}   Fault mask: {r['fault']}")
        else:
            self.live.set('No observations')
        self.history.delete(*self.history.get_children())
        for row in s.rows:
            self.history.insert('','end',values=[row[k] for k in self.columns])
    def poll(self):
        if self.serial:
            try:
                data,dead=self.serial.read()
                rows=self.stream.feed(data)
                self.session.errors=self.stream.errors
                for row in rows:
                    self.session.accept(row,time.monotonic())
                if dead:
                    self.session.warning=self.session.warning or 'Serial disconnected'
                    self.disconnect()
            except (OSError,ValueError,termios.error) as error:
                self.session.warning=str(error)
                self.disconnect()
        self.refresh()
        self.timer=self.root.after(50,self.poll)
    def close(self):
        self.root.after_cancel(self.timer)
        self.disconnect()
        self.root.destroy()

if __name__=='__main__':
    root=tk.Tk()
    Workbench(root)
    root.mainloop()
