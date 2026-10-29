#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Rest Quartet Linux desktop: pocket ledger, snapshots, and setup."""
import copy
import csv
import os
import select
import stat
import termios
import time
import tkinter as tk
from tkinter import ttk, filedialog, messagebox
from protocol import Parser, Session, NAMES

class Serial:
    def __init__(self,path):
        self.fd=os.open(path,os.O_RDWR|os.O_NOCTTY|os.O_NONBLOCK)
        self.old=None
        try:
            self.old=termios.tcgetattr(self.fd)
            settings=copy.deepcopy(self.old)
            settings[0]=settings[1]=settings[3]=0
            settings[2]=termios.CS8|termios.CREAD|termios.CLOCAL
            settings[4]=settings[5]=termios.B115200
            settings[6][termios.VMIN]=0
            settings[6][termios.VTIME]=0
            termios.tcsetattr(self.fd,termios.TCSANOW,settings)
            termios.tcflush(self.fd,termios.TCIFLUSH)
            self.poller=select.poll()
            self.poller.register(self.fd,select.POLLIN|select.POLLHUP|select.POLLERR)
        except BaseException:
            self.close()
            raise

    def read(self):
        if self.fd is None:
            return b'',True
        data=bytearray()
        lost=False
        for _,event in self.poller.poll(0):
            if event & select.POLLIN:
                try:
                    data.extend(os.read(self.fd,8192))
                except BlockingIOError:
                    pass
            lost=bool(event & (select.POLLHUP|select.POLLERR|select.POLLNVAL))
        return bytes(data),lost

    def close(self):
        if self.fd is None:
            return
        if self.old is not None:
            try:
                termios.tcsetattr(self.fd,termios.TCSANOW,self.old)
            except (OSError,termios.error):
                pass
        os.close(self.fd)
        self.fd=None

def replay(path):
    fd=os.open(path,os.O_RDONLY|os.O_NONBLOCK)
    try:
        info=os.fstat(fd)
        if not stat.S_ISREG(info.st_mode) or info.st_size>1048576:
            raise ValueError('Replay must be a regular file no larger than 1 MiB')
        with os.fdopen(fd,'rb',closefd=False) as stream:
            data=stream.read(1048577)
        if len(data)>1048576:
            raise ValueError('Replay grew past limit')
    finally:
        os.close(fd)
    parser=Parser()
    session=Session('replay')
    for f in parser.feed(data):
        session.accept(f,time.monotonic())
    parser.finish()
    if parser.errors or session.blocked or not session.rows:
        raise ValueError('Replay incomplete, malformed, empty, or internally inconsistent')
    return session,parser

def export(path,session,parser,connected):
    fd=os.open(path,os.O_CREAT|os.O_EXCL|os.O_WRONLY,0o600)
    with os.fdopen(fd,'w',newline='') as stream:
        writer=csv.writer(stream)
        now=time.monotonic()
        age='' if session.last_host is None else now-session.last_host
        writer.writerow(['author','jayis1','source',session.source,'connected',connected,
                         'fresh',session.fresh(now,connected),'age_s',age,
                         'blocked',session.blocked,'parser_errors',parser.errors,'evicted',session.evicted])
        writer.writerow(['time','gaps','tx','fault','qualify','goal','limit']+
                        [letter+'_'+n for letter in 'ABCD' for n in NAMES])
        writer.writerows(session.rows)

class App:
    def __init__(self,root):
        self.root=root
        root.title('Rest Quartet — jayis1')
        root.geometry('1150x550')
        self.serial=None
        self.session=Session('none')
        self.parser=Parser()
        self.port=tk.StringVar(value='/dev/ttyACM0')
        self.status=tk.StringVar(value='DISCONNECTED — no evidence')
        bar=ttk.Frame(root)
        bar.pack(fill='x')
        ttk.Label(bar,text='Linux serial port:').pack(side='left')
        ttk.Entry(bar,textvariable=self.port,width=28).pack(side='left')
        for title,callback in [('Connect',self.connect),('Disconnect',self.disconnect),
                               ('Replay',self.load),('Export CSV',self.save)]:
            ttk.Button(bar,text=title,command=callback).pack(side='left')
        ttk.Label(root,textvariable=self.status).pack(fill='x')
        self.tabs=ttk.Notebook(root)
        self.tabs.pack(fill='both',expand=True)
        live=ttk.Frame(self.tabs)
        history=ttk.Frame(self.tabs)
        setup=ttk.Frame(self.tabs)
        self.tabs.add(live,text='Pocket residence')
        self.tabs.add(history,text='Snapshot notebook')
        self.tabs.add(setup,text='Setup and limitations')
        columns=('pocket',)+NAMES
        self.table=ttk.Treeview(live,columns=columns,show='headings',height=4)
        for name in columns:
            self.table.heading(name,text=name)
            self.table.column(name,width=85,stretch=True)
        self.table.pack(fill='x')
        ttk.Label(live,text='Durations are milliseconds. rested = removal after ≥60 s of qualified occupancy.\n'
                  'Not cure quality, material identity, adhesive safety, or a permission to handle.').pack(anchor='w',pady=20)
        self.history=ttk.Treeview(history,columns=('time','gaps','tx','fault','starts','finished'),show='headings')
        for name in self.history['columns']:
            self.history.heading(name,text=name)
        self.history.pack(fill='both',expand=True)
        text=('Author: jayis1\n\nConnect only detached voltage-free pocket switches. J3..J6 = A..D.\n'
              'Empty each pocket after power-up or acquisition gap before inserting a labeled coupon.\n'
              'A pocket tracks placement episodes, NOT specimen identity. Replacement ends an episode.\n'
              'Qualification: 8 ms independently; sampling: 1 ms. Goal: 60 s; timeout: 1 hour.\n'
              'Open contacts, broken wires and removed coupons are indistinguishable.\n'
              'Start adhesion timing separately: placement may occur long after adhesive application.\n'
              'CSV retains at most 500 snapshots, not all events. Export before reconnecting.\n'
              'Reception pauses, resets, or clock/backlog drift require explicit reconnect.\n'
              'Replays are offline. No physical measurements have validated this prototype.\n'
              'No heaters, battery, mains wiring, actuators, command receiver or cloud service.')
        ttk.Label(setup,text=text,justify='left',wraplength=1000).pack(anchor='nw',padx=15,pady=15)
        root.protocol('WM_DELETE_WINDOW',self.close)
        self.job=root.after(25,self.poll)

    def connect(self):
        if self.serial is not None:
            return
        try:
            serial=Serial(self.port.get())
        except (OSError,termios.error) as error:
            messagebox.showerror('Connection failed',str(error))
            return
        self.serial=serial
        self.session=Session('live')
        self.parser=Parser()
        self.render()

    def disconnect(self):
        if self.serial:
            self.serial.close()
        self.serial=None
        self.render()

    def load(self):
        if self.serial:
            messagebox.showerror('Replay','Disconnect before replacing live evidence')
            return
        path=filedialog.askopenfilename()
        if not path:
            return
        try:
            session,parser=replay(path)
        except (OSError,ValueError) as error:
            messagebox.showerror('Replay rejected',str(error))
            return
        self.session,self.parser=session,parser
        self.render()

    def save(self):
        path=filedialog.asksaveasfilename(defaultextension='.csv',confirmoverwrite=False)
        if not path:
            return
        try:
            export(path,self.session,self.parser,self.serial is not None)
        except OSError as error:
            messagebox.showerror('Export failed',str(error))

    def poll(self):
        if self.serial:
            try:
                data,lost=self.serial.read()
                for f in self.parser.feed(data):
                    self.session.accept(f,time.monotonic())
                if lost:
                    self.disconnect()
            except (OSError,termios.error):
                self.disconnect()
        self.render()
        self.job=self.root.after(25,self.poll)

    def render(self):
        connected=self.serial is not None
        label='LIVE' if self.session.fresh(time.monotonic(),connected) else ('WAIT/STALE' if connected else 'DISCONNECTED')
        self.status.set(f'{label} | source={self.session.source} | parser errors={self.parser.errors} | '
                        f'evicted={self.session.evicted} | {self.session.blocked}')
        for table in (self.table,self.history):
            children=table.get_children()
            if children: table.delete(*children)
        if self.session.rows:
            f=self.session.rows[-1]
            for i,letter in enumerate('ABCD'):
                values=list(f[7+11*i:18+11*i])
                values[0]=('unknown','empty','occupied')[values[0]]
                self.table.insert('','end',values=[letter]+values)
            for row in self.session.rows:
                starts=sum(row[8+11*i] for i in range(4))
                finished=sum(row[9+11*i]+row[10+11*i] for i in range(4))
                self.history.insert('','end',values=list(row[:4])+[starts,finished])

    def close(self):
        if self.serial:
            self.serial.close()
            self.serial=None
        self.root.after_cancel(self.job)
        self.root.destroy()

if __name__=='__main__':
    root=tk.Tk()
    App(root)
    root.mainloop()
