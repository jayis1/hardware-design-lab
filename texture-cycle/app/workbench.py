#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Texture Cycle: Linux/Tk connection, preference graph, history and exports."""
import termios
import tkinter as tk
from tkinter import ttk, filedialog
from model import Session, STATES, NAMES, describe
from transport import Decoder, Serial, load_replay

class Workbench:
    def __init__(self,root):
        self.root=root; root.title('Texture Cycle — jayis1'); root.geometry('1000x650')
        self.port=None; self.decoder=Decoder(); self.session=Session('replay')
        self.path=tk.StringVar(value='/dev/ttyACM0')
        self.label=tk.StringVar(value='four swatches, one participant, one criterion')
        self.status=tk.StringVar(value='Disconnected; no evidence')
        self.summary=tk.StringVar(value='No samples')
        self.tabs=ttk.Notebook(root); self.tabs.pack(fill='both',expand=True)
        pages=[ttk.Frame(self.tabs,padding=16) for _ in range(4)]
        for page,title in zip(pages,('Connection / replay','Preference graph','History','Policy / export')):
            self.tabs.add(page,text=title)
        connection,live,history,settings=pages
        ttk.Label(connection,text='Linux serial path; 115200 8N1; receive only').pack(anchor='w')
        ttk.Entry(connection,textvariable=self.path,width=70).pack(anchor='w')
        for text,command in [('Connect',self.connect),('Disconnect',self.disconnect),('Open replay',self.replay)]:
            ttk.Button(connection,text=text,command=command).pack(anchor='w',pady=5)
        ttk.Label(root,textvariable=self.status,wraplength=960).pack(fill='x')
        ttk.Label(live,textvariable=self.summary,wraplength=950,font=('TkDefaultFont',12)).pack(anchor='w')
        ttk.Label(live,text='Arrow means declared preferred to, not measured softness.\n'
                  'Both directions and longer cycles remain visible. Repeated votes do not erase edges.').pack(anchor='w',pady=10)
        self.graph=tk.Canvas(live,width=750,height=300,background='white'); self.graph.pack(fill='both',expand=True)
        self.history=ttk.Treeview(history,columns=('time','refs','votes','rejected','aborted','graph'),show='headings')
        for c in ('time','refs','votes','rejected','aborted','graph'):
            self.history.heading(c,text=c); self.history.column(c,width=110)
        self.history.pack(fill='both',expand=True)
        ttk.Label(settings,text='TC1: Gray selector 00,01,11,10 identifies captive swatches A,B,C,D.\n'
                  'Select reference; press and release REFERENCE. Select preferred swatch; press PREFER.\n'
                  'Each PREFER consumes its reference. Self-comparisons and uncertain strokes are rejected.\n'
                  'For reverse preference, nominate the other swatch as reference first. Ties are not recorded.\n'
                  'One participant and criterion per MCU reset. Reconnect does NOT clear the graph.\n'
                  'Export, deliberately reset the MCU, then reconnect for a new study.\n'
                  'No ranking of people, diagnostic claim or network service. Author: jayis1.',wraplength=950).pack(anchor='w')
        ttk.Entry(settings,textvariable=self.label,width=90).pack(anchor='w',pady=12)
        ttk.Button(settings,text='Export private CSV; no overwrite',command=self.export).pack(anchor='w')
        ttk.Label(settings,text='Last 2000 snapshots retained; sparse traffic may omit individual strokes.').pack(anchor='w',pady=12)
        root.protocol('WM_DELETE_WINDOW',self.close); self.timer=root.after(30,self.poll)

    def connect(self):
        if self.port is not None:
            self.status.set('Already connected; disconnect first'); return
        try: port=Serial(self.path.get())
        except (OSError,termios.error,ValueError) as exc:
            self.status.set('Connection failed: '+str(exc)); return
        self.port=port; self.decoder=Decoder(); self.session=Session('live')
        self.render(); self.status.set('Waiting for two advancing snapshots')

    def disconnect(self):
        if self.port is not None:
            self.port.close(); self.port=None
            if self.decoder.buffer:
                self.decoder.errors+=1; self.decoder.buffer=b''
        self.session.connected=False; self.session.errors=self.decoder.errors
        self.status.set('Disconnected; retained evidence is historical'); self.render()

    def poll(self):
        if self.port is not None:
            try:
                rows=self.decoder.feed(self.port.read()); self.session.errors=self.decoder.errors
                for row in rows: self.session.accept(row)
                if rows: self.render()
                self.status.set('LIVE' if self.session.fresh() else 'Waiting / STALE')
            except (OSError,ValueError) as exc:
                self.disconnect(); self.status.set(str(exc))
        self.timer=self.root.after(30,self.poll)

    def render(self):
        self.graph.delete('all'); self.history.delete(*self.history.get_children())
        if not self.session.rows: self.summary.set('No samples'); return
        r=self.session.rows[-1]
        ref=NAMES[r['anchor']] if r['anchored'] else 'none — nominate reference'
        self.summary.set(f"{STATES[r['state']]} | {describe(r)}\nReference: {ref}\n"
                         f"Preferences {r['preferences']} | Rejected {r['rejected']} | Aborted {r['aborted']}\n"
                         f"Contacts {r['levels']} | Gaps {r['gaps']} | Faults {r['faults']}")
        points=[(170,65),(560,65),(560,240),(170,240)]
        for a,(x,y) in enumerate(points):
            for b,(xx,yy) in enumerate(points):
                if r['graph']&(1<<(4*a+b)):
                    # Offset opposing directed edges so neither conceals the other.
                    dx,dy=xx-x,yy-y; length=(dx*dx+dy*dy)**.5
                    ox,oy=-dy/length*7,dx/length*7
                    self.graph.create_line(x+dx/length*27+ox,y+dy/length*27+oy,
                        xx-dx/length*27+ox,yy-dy/length*27+oy,arrow='last',width=2,
                        fill='#ad2222' if r['cycle'] else '#124b78',tags='edge')
        for i,(x,y) in enumerate(points):
            self.graph.create_oval(x-24,y-24,x+24,y+24,fill='#f6d477' if r['anchored'] and r['anchor']==i else '#ddd')
            self.graph.create_text(x,y,text=NAMES[i],font=('TkDefaultFont',16))
        previous=None
        for row in self.session.rows:
            key=tuple(row[k] for k in ('references','preferences','rejected','aborted','graph'))
            if key!=previous: self.history.insert('','end',values=(row['now'],*key))
            previous=key

    def replay_path(self,path):
        if self.port is not None: raise ValueError('disconnect before replacing evidence')
        session=load_replay(path)
        self.session=session; self.decoder=Decoder(); self.render()
        self.status.set('REPLAY — never live; source may be synthetic')

    def replay(self):
        path=filedialog.askopenfilename(title='Open TC1 recording')
        if path:
            try: self.replay_path(path)
            except (OSError,ValueError) as exc: self.status.set('Replay rejected: '+str(exc))

    def export_to(self,path):
        self.session.export(path,self.label.get()); self.status.set('Exported '+str(path))

    def export(self):
        path=filedialog.asksaveasfilename(title='New CSV',defaultextension='.csv')
        if path:
            try: self.export_to(path)
            except (OSError,ValueError) as exc: self.status.set('Export failed: '+str(exc))

    def close(self):
        self.root.after_cancel(self.timer); self.disconnect(); self.root.destroy()

if __name__=='__main__':
    root=tk.Tk(); Workbench(root); root.mainloop()
