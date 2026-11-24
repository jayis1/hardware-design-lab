# Author: jayis1. SPDX-License-Identifier: MIT
"""Local Linux/Tk companion; no network, cloud, commands, or dependencies."""
import csv
import os
import termios
import time
import tkinter as tk
from tkinter import ttk, filedialog
from model import FIELDS, LABELS, STATES, Session
from transport import Decoder, Serial, load_replay

class App:
    def __init__(self,root):
        self.root=root
        root.title('Insert Code — jayis1')
        self.port=None
        self.session=Session('replay')
        self.decoder=Decoder()
        self.path=tk.StringVar(value='/dev/ttyACM0')
        self.status=tk.StringVar(value='No evidence loaded')
        bar=ttk.Frame(root); bar.pack(fill='x')
        ttk.Entry(bar,textvariable=self.path,width=36).pack(side='left')
        for text,fn in [('Connect',self.connect),('Disconnect',self.disconnect),
                        ('Replay',self.replay),('Export',self.export)]:
            ttk.Button(bar,text=text,command=fn).pack(side='left')
        ttk.Label(root,textvariable=self.status,wraplength=900).pack(fill='x')
        tabs=ttk.Notebook(root); tabs.pack(fill='both',expand=True)
        self.summary=ttk.Frame(tabs); self.history=ttk.Frame(tabs)
        tabs.add(self.summary,text='Insert identities'); tabs.add(self.history,text='Acquisition history')
        self.info=tk.StringVar(value='Waiting for IC1 records')
        ttk.Label(self.summary,textvariable=self.info).pack(fill='x')
        columns=('metric','seats')
        self.table=ttk.Treeview(self.summary,columns=columns,show='headings',height=6)
        for c in columns:
            self.table.heading(c,text=c)
            self.table.column(c,width=300 if c=='metric' else 100)
        self.table.pack(fill='both',expand=True)
        self.log=ttk.Treeview(self.history,columns=('time','raw','state','identity','completed'),show='headings')
        for c in self.log['columns']: self.log.heading(c,text=c)
        self.log.pack(fill='both',expand=True)
        ttk.Label(root,text='Codes are not authenticated identities or seating proof. Time units: nominal ms.').pack()
        root.protocol('WM_DELETE_WINDOW',self.close)
        self.timer=root.after(25,self.poll)
    def render(self):
        for table in (self.table,self.log):
            table.delete(*table.get_children())
        if not self.session.rows:
            self.info.set('Waiting for IC1 records'); return
        row=self.session.rows[-1]
        self.info.set(f"{STATES[row['state']]} | raw {row['raw']:04b} | qualified {row['qualified']:04b}\n"
                      f"Identity {row['identity']} / active age {row['age']} / latest duration {row['latest_age']}\n"
                      f"Started {row['starts']} / completed {row['completed']} / abandoned {row['abandoned']}\n"
                      f"Changed-code episodes {row['changed']} / invalid-code episodes {row['invalid']} / "
                      f"unidentified {row['unidentified']} / missed {row['missed']} / fault {row['fault']}")
        for i,label in enumerate(LABELS):
            self.table.insert('', 'end',values=[label,row[f'seats{i}']])
        for r in self.session.rows:
            self.log.insert('','end',values=[r[k] for k in ('now','raw','state','identity','completed')])
    def connect(self):
        if self.port is not None:
            self.status.set('Already connected; disconnect first'); return
        try:
            port=Serial(self.path.get())
        except (OSError,termios.error) as exc:
            self.status.set(str(exc)); return
        self.port=port
        self.decoder=Decoder()
        self.session=Session('live')
        self.render()
        self.status.set('Connected; awaiting advancing acquisition')
    def disconnect(self):
        if self.port is not None:
            self.port.close(); self.port=None
        self.session.connected=False
        if self.decoder.buffer:
            self.decoder.errors+=1; self.decoder.buffer=b''
        self.session.parser_errors=self.decoder.errors
        self.status.set('Disconnected; retained evidence is historical')
    def replay(self,path=None):
        if self.port is not None:
            self.status.set('Disconnect before replacing evidence'); return
        path=path or filedialog.askopenfilename(title='Open IC1 recording')
        if not path: return
        try: replacement=load_replay(path)
        except (OSError,ValueError) as exc:
            self.status.set(str(exc)); return
        self.session=replacement
        self.decoder=Decoder()
        self.render(); self.status.set('Replay — not live acquisition')
    def export(self,path=None):
        path=path or filedialog.asksaveasfilename(defaultextension='.csv')
        if not path: return
        try:
            # Exclusive creation prevents silent overwrite; private by default.
            fd=os.open(path,os.O_WRONLY|os.O_CREAT|os.O_EXCL,0o600)
            with os.fdopen(fd,'w',newline='') as handle:
                writer=csv.writer(handle)
                writer.writerow(['author','jayis1','source',self.session.source,
                    'connected',self.session.connected,'fresh',self.session.fresh(),
                    'parser_errors',self.session.parser_errors,'error',self.session.error])
                writer.writerow(FIELDS)
                for row in self.session.rows: writer.writerow([row[k] for k in FIELDS])
            self.status.set('Exported retained history (maximum 2000 snapshots)')
        except OSError as exc: self.status.set(str(exc))
    def poll(self):
        if self.port is not None:
            try:
                data=self.port.read()
                rows=self.decoder.feed(data)
                self.session.parser_errors=self.decoder.errors
                for row in rows: self.session.accept(row)
                if rows: self.render()
                if self.session.fresh():
                    self.status.set(f'Advancing acquisition; parser errors {self.decoder.errors}')
                else:
                    self.status.set('Not live: awaiting progress, stale, or halted acquisition')
            except (OSError,ValueError,termios.error) as exc:
                self.session.parser_errors=self.decoder.errors
                self.session.error=str(exc)
                self.render()
                self.disconnect(); self.status.set(str(exc))
        self.timer=self.root.after(25,self.poll)
    def close(self):
        self.root.after_cancel(self.timer)
        self.disconnect(); self.root.destroy()

if __name__=='__main__':
    root=tk.Tk(); App(root); root.mainloop()
