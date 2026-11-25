# Author: jayis1. SPDX-License-Identifier: MIT
"""Linux Tk companion: live position planes, evidence table and private CSV."""
import csv
import json
import os
import sys
import time
import tkinter as tk
from tkinter import ttk, filedialog, messagebox
import termios
from model import Session, FIELDS, STATES, REASONS
from transport import Serial, Decoder, load_replay

class App:
    def __init__(self, root):
        self.root=root
        root.title('Sweep Memory — jayis1')
        self.serial=None
        self.session=Session('none')
        self.decoder=Decoder()
        self.last_progress=None
        self.origin_host=None
        self.origin_device=None
        self.elapsed_device=0
        self.warning='No device connected; no physical result.'
        self.status=tk.StringVar(value=self.warning)
        self.port=tk.StringVar(value='/dev/ttyACM0')
        toolbar=ttk.Frame(root); toolbar.pack(fill='x')
        ttk.Entry(toolbar,textvariable=self.port,width=30).pack(side='left')
        for label,callback in [('Connect',self.connect_dialog),('Disconnect',self.disconnect),('Replay',self.replay_dialog),('Export CSV',self.export_dialog)]:
            ttk.Button(toolbar,text=label,command=callback).pack(side='left')
        ttk.Label(root,textvariable=self.status,wraplength=850).pack(fill='x')
        self.tabs=ttk.Notebook(root); self.tabs.pack(fill='both',expand=True)
        self.map_page=ttk.Frame(self.tabs)
        self.table_page=ttk.Frame(self.tabs)
        self.tabs.add(self.map_page,text='Position comparison')
        self.tabs.add(self.table_page,text='Acquisition history')
        self.summary=tk.StringVar(value='No retained capture')
        ttk.Label(self.map_page,textvariable=self.summary).pack()
        self.canvas=tk.Canvas(self.map_page,width=850,height=250,bg='white')
        self.canvas.pack(fill='both',expand=True)
        ttk.Label(self.map_page,text='16 relative phase cells, not calibrated angle. Yellow = unobserved. Red = directional disagreement.\nOne shot per reset. Export before resetting; return to your marked start by hand.').pack()
        columns=['now','state','position','age','compared','mismatch_x','mismatch_y']
        self.table=ttk.Treeview(self.table_page,columns=columns,show='headings',height=18)
        for key in columns:
            self.table.heading(key,text=key); self.table.column(key,width=105)
        self.table.pack(fill='both',expand=True)
        root.protocol('WM_DELETE_WINDOW',self.close)
        self.timer=root.after(50,self.poll)
    def disconnect(self):
        if self.serial: self.serial.close()
        self.serial=None
        self.warning='Disconnected; retained evidence is not live.'
        self.status.set(self.warning)
    def connect(self,path):
        if self.serial: raise ValueError('Disconnect before opening another port')
        # Construct first: an invalid replacement cannot erase old evidence.
        serial=Serial(path)
        self.serial=serial
        self.session=Session('live')
        self.decoder=Decoder()
        self.last_progress=None
        self.origin_host=None
        self.origin_device=None
        self.elapsed_device=0
        self.warning='Waiting for advancing acquisition'
        self.render()
    def connect_dialog(self):
        try: self.connect(self.port.get())
        except (OSError,ValueError,termios.error) as exc: messagebox.showerror('Connection',str(exc))
    def replay(self,path):
        candidate=load_replay(path)
        self.disconnect()
        self.session=candidate
        self.decoder=Decoder()
        self.last_progress=None
        self.origin_host=None
        self.origin_device=None
        self.elapsed_device=0
        self.warning='Replay; historical or synthetic evidence, never live'
        self.render()
    def replay_dialog(self):
        path=filedialog.askopenfilename()
        if path:
            try: self.replay(path)
            except (OSError,ValueError) as exc: messagebox.showerror('Replay',str(exc))
    def fresh(self):
        return bool(self.serial and self.last_progress is not None and not self.session.blocked and time.monotonic()-self.last_progress<1.0)
    def receive(self,data):
        for row in self.decoder.feed(data):
            previous=self.session.rows[-1] if self.session.rows else None
            accepted=self.session.accept(row)
            if not accepted: continue
            host=time.monotonic()
            if self.origin_host is None:
                self.origin_host=host
                self.origin_device=row['now']
            elif previous:
                self.elapsed_device+=(row['now']-previous['now'])&0xffffffff
                if abs(self.elapsed_device/1000-(host-self.origin_host))>1.0:
                    raise ValueError('Device / host time divergence; reconnect and inspect buffering')
                if row['samples']>previous['samples'] and not row['fault']:
                    self.last_progress=host
    def poll(self):
        try:
            if self.serial:
                # Never renew live status from an old queue after a host pause.
                if self.last_progress is not None and time.monotonic()-self.last_progress>1.0:
                    raise ValueError('Acquisition stale; reconnect required')
                self.receive(self.serial.read())
                self.warning='Acquisition advancing' if self.fresh() else 'Waiting / no verified acquisition advance'
        except (OSError,ValueError,termios.error) as exc:
            self.disconnect()
            self.warning=str(exc)
        self.render()
        self.timer=self.root.after(50,self.poll)
    def render(self):
        self.status.set(f'{self.session.provenance} | {self.warning} | framing errors {self.decoder.errors}')
        self.canvas.delete('all')
        for item in self.table.get_children(): self.table.delete(item)
        if not self.session.rows:
            self.summary.set('No retained capture'); return
        r=self.session.rows[-1]
        self.summary.set(f"{STATES[r['state']]} | position {r['position']}/16 | age {r['age']} ms | {REASONS[r['reason']]}")
        for j,(key,label) in enumerate([('outward_x','X outward'),('return_x','X return'),('outward_y','Y outward'),('return_y','Y return')]):
            y=30+j*45
            self.canvas.create_text(5,y+12,text=label,anchor='w')
            for p in range(16):
                outward_complete=bool(r['compared'] or r['position']==16 or r['completed'])
                observed=(r['starts'] and (outward_complete or p<=r['position'])) if key.startswith('outward') else bool(r['compared']&(1<<p))
                color=('#376eb4' if r[key]&(1<<p) else '#eeeeee') if observed else '#e9cf6b'
                x=115+p*43
                self.canvas.create_rectangle(x,y,x+38,y+25,fill=color)
                self.canvas.create_text(x+19,y+12,text=str(p),fill='red' if r['mismatch_'+key[-1]]&(1<<p) else 'black')
        for row in self.session.rows[-200:]:
            self.table.insert('','end',values=[STATES[row[k]] if k=='state' else row[k] for k in self.table['columns']])
    def export(self,path):
        metadata={'author':'jayis1','provenance':self.session.provenance,'connected':bool(self.serial),'fresh':self.fresh(),'blocked':self.session.blocked,'warning':self.warning,'framing_errors':self.decoder.errors,'initial_fragments':self.decoder.initial_fragments,'retained_limit':2000}
        # Exclusive creation prevents accidental overwrite, permissions protect local sessions.
        fd=os.open(path,os.O_WRONLY|os.O_CREAT|os.O_EXCL,0o600)
        with os.fdopen(fd,'w',newline='') as handle:
            handle.write('# '+json.dumps(metadata,sort_keys=True)+'\n')
            writer=csv.DictWriter(handle,fieldnames=FIELDS)
            writer.writeheader(); writer.writerows(self.session.rows)
    def export_dialog(self):
        path=filedialog.asksaveasfilename(defaultextension='.csv')
        if path:
            try: self.export(path)
            except OSError as exc: messagebox.showerror('Export',str(exc))
    def close(self):
        self.root.after_cancel(self.timer)
        self.disconnect()
        self.root.destroy()

if __name__=='__main__':
    root=tk.Tk(); app=App(root)
    if len(sys.argv)==2: app.replay(sys.argv[1])
    root.mainloop()
