#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Linux/Tk companion; local serial observation, bounded replay and CSV."""
import csv
import os
import select
import termios
import time
import tkinter as tk
from tkinter import ttk, filedialog, messagebox
from protocol import FIELDS, PHASES, REASONS, Model, Stream, read_replay

class Serial:
    def __init__(self,path):
        self.fd=os.open(path,os.O_RDONLY|os.O_NOCTTY|os.O_NONBLOCK)
        self.original=None
        try:
            self.original=termios.tcgetattr(self.fd)
            attrs=termios.tcgetattr(self.fd)
            attrs[0]=0; attrs[1]=0; attrs[3]=0
            attrs[2]=termios.CS8|termios.CREAD|termios.CLOCAL
            attrs[4]=termios.B115200; attrs[5]=termios.B115200
            attrs[6][termios.VMIN]=0; attrs[6][termios.VTIME]=0
            termios.tcsetattr(self.fd,termios.TCSANOW,attrs)
            termios.tcflush(self.fd,termios.TCIFLUSH)
            self.poller=select.poll()
            self.poller.register(self.fd,select.POLLIN|select.POLLERR|select.POLLHUP)
        except BaseException:
            self.close()
            raise
    def read(self):
        if self.fd is None: raise OSError('serial port closed')
        events=self.poller.poll(0)
        if not events: return b'',False
        flags=events[0][1]
        data=b''
        if flags&select.POLLIN:
            try: data=os.read(self.fd,8192)
            except BlockingIOError: pass
        return data,bool(flags&(select.POLLHUP|select.POLLERR|select.POLLNVAL))
    def close(self):
        if self.fd is not None:
            if self.original is not None:
                try: termios.tcsetattr(self.fd,termios.TCSANOW,self.original)
                except (OSError,termios.error): pass
            os.close(self.fd); self.fd=None

class Application:
    def __init__(self,root):
        self.root=root
        self.serial=None
        self.model=Model(); self.stream=Stream()
        self.provenance='none'; self.job=None
        root.title('Pipette Path — jayis1')
        root.geometry('960x600')
        self.tabs=ttk.Notebook(root); self.tabs.pack(fill='both',expand=True)
        self.connection=ttk.Frame(self.tabs); self.live=ttk.Frame(self.tabs); self.history=ttk.Frame(self.tabs)
        for frame,title in [(self.connection,'Connection'),(self.live,'Live witnesses'),(self.history,'History / export')]:
            self.tabs.add(frame,text=title)
        ttk.Label(self.connection,text='Pipette Path / Author: jayis1\nPlunger sequence and declared source immersion, NOT volume measurement. Training only.').pack(pady=15)
        self.path=tk.StringVar(value='/dev/ttyACM0')
        ttk.Entry(self.connection,textvariable=self.path,width=50).pack()
        ttk.Button(self.connection,text='Connect and flush',command=lambda:self.action(lambda:self.connect(self.path.get()))).pack(pady=5)
        ttk.Button(self.connection,text='Disconnect (retain evidence)',command=self.disconnect).pack()
        ttk.Button(self.connection,text='Replay PP1 text file',command=self.choose_replay).pack(pady=5)
        self.status=tk.StringVar(value='Disconnected')
        ttk.Label(self.connection,textvariable=self.status,wraplength=800).pack(pady=20)
        self.summary=tk.StringVar(value='No evidence')
        ttk.Label(self.live,textvariable=self.summary,wraplength=900,font=('TkDefaultFont',13)).pack(pady=20)
        self.witness=tk.StringVar(value='Home / first / second / source pedal: unknown')
        ttk.Label(self.live,textvariable=self.witness).pack(pady=10)
        ttk.Label(self.live,text='First stop dry → declare source → release home → leave source → first → second → home.\nCompletion means this sequence was observed, not correct liquid delivery.\nThe pedal is a human declaration, not a liquid sensor.').pack(pady=20)
        self.table=ttk.Treeview(self.history,columns=('now','phase','starts','completed','aborted','last_ms','last_reason'),show='headings',height=16)
        for key in self.table['columns']: self.table.heading(key,text=key); self.table.column(key,width=130)
        self.table.pack(fill='both',expand=True)
        ttk.Button(self.history,text='Export retained snapshots and provenance',command=self.choose_export).pack(pady=10)
        root.protocol('WM_DELETE_WINDOW',self.shutdown)
        self.poll()
    def action(self,fn):
        try: fn()
        except (OSError,ValueError,RuntimeError,termios.error) as error:
            messagebox.showerror('Pipette Path',str(error),parent=self.root)
    def connect(self,path):
        if self.serial: raise RuntimeError('disconnect before changing ports')
        port=Serial(path) # Preserve old evidence if opening fails.
        self.serial=port; self.model=Model(); self.stream=Stream()
        self.provenance='live'; self.render()
    def disconnect(self):
        if self.serial: self.serial.close(); self.serial=None
        self.render()
    def replay(self,path):
        if self.serial: raise RuntimeError('disconnect before replay')
        rows=read_replay(path)
        replacement=Model()
        for i,row in enumerate(rows): replacement.accept(row,i*.1,live=False)
        self.model=replacement; self.stream=Stream(); self.provenance='replay'
        self.render()
    def choose_replay(self):
        path=filedialog.askopenfilename(parent=self.root)
        if path: self.action(lambda:self.replay(path))
    def choose_export(self):
        path=filedialog.asksaveasfilename(parent=self.root,defaultextension='.csv')
        if path: self.action(lambda:self.export(path))
    def export(self,path):
        # Exclusive creation avoids silently replacing another experiment.
        with open(path,'x',newline='',opener=lambda p,f:os.open(p,f,0o600)) as f:
            writer=csv.writer(f)
            writer.writerow(['author','jayis1','provenance',self.provenance,'connected',bool(self.serial),'fresh',bool(self.serial) and self.model.fresh(time.monotonic()),'parser_errors',self.stream.errors,'blocked',self.model.blocked,'warning',self.model.warning])
            writer.writerow(FIELDS+['host_age_seconds'])
            now=time.monotonic()
            for row,received in zip(self.model.rows,self.model.received):
                age=max(0,now-received) if self.provenance=='live' else 'replay'
                writer.writerow([row[k] for k in FIELDS]+[age])
    def render(self):
        live=bool(self.serial) and self.model.fresh(time.monotonic())
        state='LIVE' if live else ('STALE / WAITING' if self.serial else 'DISCONNECTED')
        self.status.set(f'{state}; source={self.provenance}; framing errors={self.stream.errors}; {self.model.warning}')
        row=self.model.last
        self.table.delete(*self.table.get_children())
        for item in self.model.rows:
            self.table.insert('', 'end',values=[item[k] for k in self.table['columns']])
        if row is None:
            self.summary.set('No evidence'); self.witness.set('All witnesses unknown'); return
        acquisition='HALTED' if row['faults'] else 'qualified-state evidence'
        lines=[f"{state}; {acquisition}; faults={row['faults']}; gaps={row['gaps']}",
               PHASES[row['phase']],
               f"starts={row['starts']}; completed={row['completed']}; aborted={row['aborted']}",
               f"current={row['elapsed']} ms; last={row['last_ms']} ms; last reason: {REASONS[row['last_reason']]}"]
        self.summary.set('\n'.join(lines))
        self.witness.set(' / '.join(f'{label}: '+('closed' if row['levels']&(1<<i) else 'open') if row['qualified']&(1<<i) else f'{label}: unknown' for i,label in enumerate(['Home','First threshold','Second threshold','Source pedal'])))
    def poll(self):
        if self.job is not None:
            self.root.after_cancel(self.job); self.job=None
        if self.serial:
            try:
                data,hangup=self.serial.read()
                for row in self.stream.feed(data): self.model.accept(row,time.monotonic())
                if hangup: self.disconnect()
            except (OSError,ValueError,termios.error) as error:
                self.model.warning=str(error); self.disconnect()
        self.render()
        self.job=self.root.after(100,self.poll)
    def close(self):
        if self.job is not None: self.root.after_cancel(self.job); self.job=None
        if self.serial: self.serial.close(); self.serial=None
    def shutdown(self):
        self.close(); self.root.destroy()

if __name__=='__main__':
    window=tk.Tk(); Application(window); window.mainloop()
