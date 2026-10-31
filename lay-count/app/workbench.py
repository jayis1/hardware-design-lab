#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Linux desktop companion: local serial, traverse history and evidence export."""
import glob
import tkinter as tk
from tkinter import ttk, filedialog, messagebox
from model import Session, STATES
from transport import Decoder, Serial, load_replay

class Workbench:
    def __init__(self,root):
        self.root=root
        root.title('Lay Count — jayis1')
        root.geometry('980x600')
        self.port=None
        self.decoder=Decoder()
        self.session=Session('live')
        self.path=tk.StringVar()
        self.status=tk.StringVar(value='Disconnected — no evidence')
        self.summary=tk.StringVar(value='No telemetry')
        self.tabs=ttk.Notebook(root);self.tabs.pack(fill='both',expand=True)
        connection=ttk.Frame(self.tabs,padding=12)
        traverses=ttk.Frame(self.tabs,padding=12)
        export=ttk.Frame(self.tabs,padding=12)
        for frame,title in ((connection,'Connection / Replay'),(traverses,'Traverses'),(export,'Evidence / Export')):
            self.tabs.add(frame,text=title)
        ttk.Label(connection,text='USB serial device (115200 baud, receive only)').pack(anchor='w')
        self.ports=ttk.Combobox(connection,textvariable=self.path,width=65)
        self.ports.pack(anchor='w');self.scan()
        for title,action in [('Refresh ports',self.scan),('Connect',self.connect),('Disconnect',self.disconnect),('Load replay',self.choose_replay)]:
            ttk.Button(connection,text=title,command=lambda f=action:self.guard(f)).pack(anchor='w',pady=4)
        ttk.Label(connection,text='Replay is not live. Reset inference is heuristic. Export before replacing a session.').pack(anchor='w',pady=20)
        ttk.Label(traverses,textvariable=self.summary).pack(anchor='w')
        columns=('id','end','pulses','duration','tension')
        self.table=ttk.Treeview(traverses,columns=columns,show='headings',height=16)
        for name,title in zip(columns,('Completion','Destination','Index pulses','Duration ms','Tension indication')):
            self.table.heading(name,text=title);self.table.column(name,width=170)
        self.table.pack(fill='both',expand=True)
        ttk.Label(export,text=('Export includes retained snapshots and session provenance, not only completed traverses.\n'
             'A tension dropout is an indication, not a calibrated force measurement.\n'
             'Index pulses are not signed turns. Backward winding is not detected.\n'
             'Only the most recent result exists in each frame; skipped completions cannot be reconstructed.\n'
             'Files are private (0600); existing paths are never overwritten.\n'
             'Author and creator: jayis1'),justify='left').pack(anchor='w',pady=10)
        ttk.Button(export,text='Export new CSV',command=lambda:self.guard(self.choose_export)).pack(anchor='w')
        ttk.Label(root,textvariable=self.status,wraplength=950).pack(fill='x',padx=12,pady=8)
        root.protocol('WM_DELETE_WINDOW',self.close)
        self.timer=root.after(25,self.poll)
    def guard(self,action):
        try:action()
        except (OSError,ValueError) as exc:messagebox.showerror('Lay Count',str(exc),parent=self.root)
    def scan(self):
        self.ports['values']=sorted(glob.glob('/dev/ttyACM*')+glob.glob('/dev/ttyUSB*'))
    def new_session(self,source):
        self.session=Session(source);self.decoder=Decoder()
    def connect(self):
        if self.port is not None:raise ValueError('Disconnect before opening another port')
        # Preserve old evidence if opening the new transport fails.
        port=Serial(self.path.get())
        self.new_session('live');self.port=port;self.session.connected=True
        self.render()
    def disconnect(self):
        if self.port is not None:
            self.port.close();self.port=None
            if self.decoder.buffer:
                self.decoder.errors+=1;self.decoder.buffer=b''
        self.session.errors=self.decoder.errors
        self.session.connected=False
        self.render()
    def replay(self,path):
        replacement=load_replay(path)
        self.disconnect();self.session=replacement;self.decoder=Decoder()
        self.render()
    def choose_replay(self):
        path=filedialog.askopenfilename(parent=self.root,title='Open bounded LC1 recording')
        if path:self.replay(path)
    def choose_export(self):
        path=filedialog.asksaveasfilename(parent=self.root,title='Create new evidence CSV',defaultextension='.csv')
        if path:self.session.export(path)
    def render(self):
        s=self.session
        state='REPLAY — NOT LIVE' if s.source=='replay' else ('LIVE' if s.fresh() else 'STALE / disconnected / awaiting progress')
        self.status.set(f'{state}; parser errors {s.errors}; {s.warning}')
        children=self.table.get_children()
        if children:self.table.delete(*children)
        if not s.rows:
            self.summary.set('No telemetry');return
        last=s.rows[-1]
        self.summary.set(f"{STATES[last['state']]} | completed {last['completed']} | aborted {last['aborted']} | active pulses {last['active_pulses']} | flagged traverses {last['bad_total']} | inputs {last['raw']:04b}")
        seen=set()
        results=[]
        for r in s.rows:
            if r['completed'] and r['completed'] not in seen:
                seen.add(r['completed']);results.append(r)
        for r in results[-200:]:
            self.table.insert('', 'end',values=(r['completed'],'left' if r['last_end']==1 else 'right',
                r['last_pulses'],r['last_ms'],'DROPOUT' if r['last_bad'] else 'held indication'))
    def poll(self):
        if self.port is not None:
            try:
                data=self.port.read()
                rows=self.decoder.feed(data)
                self.session.errors=self.decoder.errors
                for row in rows:self.session.accept(row)
            except (OSError,ValueError) as exc:
                self.session.errors=self.decoder.errors
                self.session.warning=str(exc)
                self.disconnect()
        self.render()
        self.timer=self.root.after(25,self.poll)
    def close(self):
        self.root.after_cancel(self.timer)
        self.disconnect();self.root.destroy()

if __name__=='__main__':
    root=tk.Tk();Workbench(root);root.mainloop()
