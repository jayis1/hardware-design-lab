# Author: jayis1. SPDX-License-Identifier: MIT
"""Local receive-only Foot Rock contact and transition workbench."""
import csv
import os
import sys
import termios
import tkinter as tk
from tkinter import ttk, filedialog, messagebox
from model import FIELDS, NAMES, Session
from transport import Decoder, Serial, load_replay

class Workbench:
    def __init__(self,root):
        self.root=root
        self.port=None
        self.session=Session('none')
        self.decoder=Decoder()
        self.warning=''
        root.title('Foot Rock — jayis1')
        self.path=tk.StringVar(value='/dev/ttyACM0')
        self.status=tk.StringVar(value='Disconnected; no evidence')
        bar=ttk.Frame(root); bar.pack(fill='x')
        ttk.Entry(bar,textvariable=self.path,width=32).pack(side='left')
        for text,action in [('Connect',self.connect),('Disconnect',self.disconnect),
                            ('Replay',self.replay_dialog),('Export CSV',self.export_dialog)]:
            ttk.Button(bar,text=text,command=action).pack(side='left')
        ttk.Label(root,textvariable=self.status,wraplength=850).pack(fill='x')
        book=ttk.Notebook(root); book.pack(fill='both',expand=True)
        live=ttk.Frame(book); history=ttk.Frame(book); about=ttk.Frame(book)
        book.add(live,text='Foot contacts')
        book.add(history,text='Missing-foot transitions')
        book.add(about,text='Experiment and limits')
        self.cells=[]
        for i,name in enumerate(NAMES):
            label=tk.Label(live,text=f'{name}: unknown',width=34,height=5,relief='groove',bg='#cccccc')
            label.grid(row=i//2,column=i%2,padx=5,pady=5)
            self.cells.append(label)
        self.summary=tk.StringVar(value='No samples')
        ttk.Label(live,textvariable=self.summary,wraplength=750).grid(row=2,column=0,columnspan=2)
        self.table=ttk.Treeview(history,columns=('from','to','kind','count'),show='headings',height=16)
        for col in self.table['columns']:
            self.table.heading(col,text=col); self.table.column(col,width=160)
        self.table.pack(fill='both',expand=True)
        ttk.Label(about,justify='left',wraplength=800,text=(
            'Author: jayis1\nFR1 / 115200 baud / 8N1 / whole-vector 5 ms qualification.\n'
            'NW NE / SW SE are labels on your lightweight model, not compass bearings.\n'
            'Green means contact closed, not measured load. Orange means contact open.\n'
            'Transitions connect qualified missing-foot endpoints. They do not measure\n'
            'pivot direction, force, gap, level, or furniture safety. No cutting advice.\n'
            'Any raw two-or-fewer contact pattern or acquisition gap breaks the chain.\n'
            '1000 clean all-four intervals end a chain. A raw glitch restarts that timer.\n'
            'Compare shims with the same gentle handling procedure and witness settings.\n'
            'Only disconnected voltage-free switches; no powered machinery.\n'
            'Export before reset; storage is volatile. Reset MCU and reconnect for a new run.\n'
            'Replay is offline evidence. Connection is not proof of acquisition freshness.\n'
            'Read README before wiring; never put a person or heavy object on this fixture.'
        )).pack(padx=15,pady=15)
        root.protocol('WM_DELETE_WINDOW',self.close)
        self.timer=root.after(25,self.poll)

    def report(self,exc):
        self.warning=str(exc); self.render()
    def connect(self):
        if self.port is not None:
            self.report('Already connected; disconnect first'); return
        try: port=Serial(self.path.get())
        except (OSError,termios.error) as exc:
            self.report(exc); return
        self.port=port
        self.session=Session('live'); self.decoder=Decoder(); self.warning=''
        self.render()
    def disconnect(self):
        if self.port is not None:
            self.port.close(); self.port=None
        self.render()
    def replay(self,path):
        session=load_replay(path)
        self.disconnect()
        self.session=session; self.decoder=Decoder(); self.warning=''
        self.render()
    def replay_dialog(self):
        path=filedialog.askopenfilename()
        if path:
            try: self.replay(path)
            except (OSError,ValueError) as exc: self.report(exc)
    def export(self,path):
        fd=os.open(path,os.O_WRONLY|os.O_CREAT|os.O_EXCL,0o600)
        with os.fdopen(fd,'w',newline='') as handle:
            writer=csv.writer(handle)
            writer.writerow(['author','jayis1','source',self.session.source,
                'connected',self.port is not None,
                'fresh',self.session.fresh(connected=self.port is not None),
                'parser_errors',self.decoder.errors,'initial_fragments',self.decoder.initial_fragments,
                'warning',self.warning,'session_error',self.session.error])
            writer.writerow(FIELDS)
            writer.writerows([r[k] for k in FIELDS] for r in self.session.rows)
    def export_dialog(self):
        path=filedialog.asksaveasfilename(defaultextension='.csv')
        if path:
            try: self.export(path)
            except OSError as exc: self.report(exc)
    def render(self):
        fresh=self.session.fresh(connected=self.port is not None)
        state='LIVE acquisition' if fresh else ('CONNECTED / unconfirmed or stale' if self.port else 'DISCONNECTED / offline')
        self.status.set(f'{state} | source={self.session.source} | errors={self.decoder.errors} | {self.warning or self.session.error}')
        self.table.delete(*self.table.get_children())
        if not self.session.rows:
            self.summary.set('No samples')
            for name,label in zip(NAMES,self.cells): label.config(text=f'{name}: unknown',bg='#cccccc')
            return
        r=self.session.rows[-1]
        for i,label in enumerate(self.cells):
            trusted=r['valid'] and not r['fault']
            closed=r['qualified']&(1<<i)
            text=('CLOSED' if closed else 'OPEN') if trusted else 'unqualified / halted'
            color=('#a8e6a1' if closed else '#ffd18a') if trusted else '#cccccc'
            label.config(text=f'{NAMES[i]}: {text}',bg=color)
        self.summary.set(f"Retained ms={r['now']} | acquired={r['samples']} | unknown={r['unknown']} | missed={r['missed']}\n"
            f"diagonal={r['diagonal']} | adjacent={r['adjacent']} | chain seeds={r['seeds']} | settle={r['settle']}/1000 | fault={r['fault']}\n"
            + 'Missing-foot dwell ms: '+', '.join(n+'='+str(r['dwell'+str(i)]) for i,n in enumerate(NAMES))
            + f"\nAll-four={r['dwell4']} | two-or-fewer={r['dwell5']} | latest retained snapshot, NOT current load.")
        for i in range(4):
            for j in range(4):
                kind='same (excluded)' if i==j else 'diagonal' if i^j==3 else 'adjacent'
                self.table.insert('','end',values=(NAMES[i],NAMES[j],kind,r[f'edges{i*4+j}']))
    def poll(self):
        if self.port is not None:
            try:
                for row in self.decoder.feed(self.port.read()): self.session.accept(row)
            except (OSError,ValueError,termios.error) as exc:
                self.warning=str(exc); self.disconnect()
        self.render()
        self.timer=self.root.after(25,self.poll)
    def close(self):
        self.root.after_cancel(self.timer)
        self.disconnect(); self.root.destroy()

if __name__=='__main__':
    root=tk.Tk(); app=Workbench(root)
    if len(sys.argv)==2:
        try: app.replay(sys.argv[1])
        except (OSError,ValueError) as exc: messagebox.showerror('Replay rejected',str(exc))
    root.mainloop()
