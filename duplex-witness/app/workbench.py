#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Duplex Witness: local connection, face coverage, history and export."""
import termios
import tkinter as tk
from tkinter import ttk, filedialog
from model import Session, STATES
from transport import Decoder, Serial, load_replay

class Workbench:
    def __init__(self, root):
        self.root=root
        root.title('Duplex Witness — jayis1')
        root.geometry('920x600')
        self.port=None
        self.decoder=Decoder()
        self.session=Session('replay')
        self.path=tk.StringVar(value='/dev/ttyACM0')
        self.label=tk.StringVar(value='unnamed flip holder')
        self.status=tk.StringVar(value='Disconnected; no evidence')
        self.summary=tk.StringVar(value='No samples')
        self.tabs=ttk.Notebook(root); self.tabs.pack(fill='both',expand=True)
        pages=[ttk.Frame(self.tabs,padding=16) for _ in range(4)]
        for page,title in zip(pages,('Connection / replay','Face coverage','History','Policy / export')):
            self.tabs.add(page,text=title)
        connection,live,history,settings=pages
        ttk.Label(connection,text='Linux serial path: 115200 baud, receive only').pack(anchor='w')
        ttk.Entry(connection,textvariable=self.path,width=70).pack(anchor='w')
        for text,command in [('Connect',self.connect),('Disconnect',self.disconnect),('Open strict replay',self.replay)]:
            ttk.Button(connection,text=text,command=command).pack(anchor='w',pady=5)
        ttk.Label(root,textvariable=self.status,wraplength=880).pack(fill='x')
        ttk.Label(live,textvariable=self.summary,wraplength=850,font=('TkDefaultFont',13)).pack(anchor='w')
        ttk.Label(live,text='A covered face means a qualified press indication at its detent.\n'
                  'It does not prove ink transfer, sheet identity, pressure or artwork alignment.\n'
                  'A stale display retains history, not current physical state.').pack(anchor='w',pady=20)
        self.history=ttk.Treeview(history,columns=('time','result','coverage','duration'),show='headings',height=16)
        for key,title in [('time','Device ms'),('result','Latest outcome'),('coverage','Faces / strokes'),('duration','Retained ms')]:
            self.history.heading(key,text=title); self.history.column(key,width=190)
        self.history.pack(fill='both',expand=True)
        ttk.Label(settings,text='DW1 policy: exactly one stroke on each face, either order, before unlatching.\n'
                  'Face must remain stable through press release. Deadline: 60 seconds.\n'
                  'No remote commands. Reset the physical MCU for a new acquisition session.\n'
                  'Firmware policy revision 1; author and creator jayis1.',wraplength=850).pack(anchor='w')
        ttk.Label(settings,text='Local fixture label (printable, maximum 120 characters)').pack(anchor='w',pady=12)
        ttk.Entry(settings,textvariable=self.label,width=70).pack(anchor='w')
        ttk.Button(settings,text='Export private CSV; never overwrite',command=self.export).pack(anchor='w',pady=12)
        ttk.Label(settings,text='Last 2000 snapshots only; sparse telemetry can skip individual outcomes.\n'
                  'Exports include provenance, warnings and corruption count. No uploads.').pack(anchor='w')
        root.protocol('WM_DELETE_WINDOW',self.close)
        self.timer=root.after(30,self.poll)

    def connect(self):
        if self.port is not None:
            self.status.set('Already connected; disconnect before changing ports'); return
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
        self.session.connected=False
        self.session.errors=self.decoder.errors
        self.status.set('Disconnected; retained evidence is not live'); self.render()

    def poll(self):
        if self.port is not None:
            try:
                rows=self.decoder.feed(self.port.read())
                self.session.errors=self.decoder.errors
                for row in rows: self.session.accept(row)
                if rows: self.render()
                self.status.set('LIVE' if self.session.fresh() else 'Waiting / STALE; no current progress confirmed')
            except (OSError,ValueError) as exc:
                self.disconnect(); self.status.set(str(exc))
        self.timer=self.root.after(30,self.poll)

    def render(self):
        if not self.session.rows: self.summary.set('No samples')
        else:
            r=self.session.rows[-1]
            faces=('neither','A only','B only','A and B')[r['faces']]
            self.summary.set(f"{STATES[r['state']]} | Covered: {faces} | Strokes: {r['strokes']}\n"
                f"Matching {r['good']}   Mismatching {r['bad']}   Abandoned {r['aborted']}\n"
                f"Trial {r['starts']} | Age {r['age']} ms | Tainted {r['tainted']}\n"
                f"Contacts mask {r['levels']} | Gaps {r['gaps']} | Faults {r['faults']}")
        self.history.delete(*self.history.get_children())
        previous=None
        for r in self.session.rows:
            key=(r['good'],r['bad'],r['aborted'])
            if key!=previous and r['last_result']:
                self.history.insert('', 'end',values=(r['now'],('none','matching','mismatching','abandoned')[r['last_result']],
                    f"{r['last_faces']} / {r['last_strokes']}",r['last_age']))
            previous=key

    def replay_path(self,path):
        if self.port is not None: raise ValueError('disconnect before replacing evidence')
        session=load_replay(path)
        self.session=session; self.decoder=Decoder()
        self.render(); self.status.set('REPLAY — never live; source may be synthetic')

    def replay(self):
        path=filedialog.askopenfilename(title='Open DW1 recording')
        if not path: return
        try: self.replay_path(path)
        except (OSError,ValueError) as exc: self.status.set('Replay rejected: '+str(exc))

    def export_to(self,path):
        self.session.export(path,self.label.get()); self.status.set('Exported '+str(path))

    def export(self):
        path=filedialog.asksaveasfilename(title='New CSV',defaultextension='.csv')
        if not path: return
        try: self.export_to(path)
        except (OSError,ValueError) as exc: self.status.set('Export failed: '+str(exc))

    def close(self):
        self.root.after_cancel(self.timer); self.disconnect(); self.root.destroy()

if __name__=='__main__':
    root=tk.Tk(); Workbench(root); root.mainloop()
