#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Local receive-only maze experiment notebook; Python stdlib + Tk on Linux."""
import tkinter as tk
from tkinter import ttk, filedialog
from protocol import Decoder, Session, Serial, STATES, reasons, route, load_replay

class Workbench:
    def __init__(self, root):
        self.root=root
        root.title('Maze Return — jayis1')
        root.geometry('1050x650')
        self.port=None
        self.session=Session('none')
        self.decoder=Decoder()
        self.path=tk.StringVar(value='/dev/ttyACM0')
        self.note=tk.StringVar()
        self.status=tk.StringVar(value='Disconnected. Checkpoint evidence, not a complete spatial path.')
        self.summary=tk.StringVar(value='No evidence')
        self.contacts=tk.StringVar(value='No input sample')
        self.tabs=ttk.Notebook(root); self.tabs.pack(fill='both',expand=True)
        connection=ttk.Frame(self.tabs,padding=16)
        live=ttk.Frame(self.tabs,padding=16)
        history=ttk.Frame(self.tabs,padding=16)
        self.tabs.add(connection,text='Connection / experiment')
        self.tabs.add(live,text='Loop-erased route')
        self.tabs.add(history,text='Snapshot history')
        ttk.Label(connection,text='Receive-only USB serial: 115200 8N1. No actuator or cloud service.').pack(anchor='w')
        ttk.Entry(connection,textvariable=self.path,width=65).pack(anchor='w',pady=8)
        buttons=ttk.Frame(connection); buttons.pack(anchor='w')
        for text,command in [('Connect',self.connect),('Disconnect',self.disconnect),
                             ('Open recording',self.replay),('Export evidence',self.save)]:
            ttk.Button(buttons,text=text,command=command).pack(side='left',padx=3)
        ttk.Label(connection,text='Maze layout / checkpoint positions / anonymous trial notes:').pack(anchor='w',pady=12)
        ttk.Entry(connection,textvariable=self.note,width=100).pack(anchor='w')
        ttk.Label(connection,text=(
            'Start with all gates clear for 100 ms. Move ONE captive magnetic puck.\n'
            'Pause at each gate for at least 20 ms; allow 150 ms clear between gates.\n'
            'Visits remain provisional until a 100 ms qualified clear interval.\n'
            'A > B > C > B walk retains A > B, recording one loop and one erased entry.\n'
            'Returning to the current tail counts as a repeat, not a loop.\n\n'
            'Ambiguity, sampling loss or a 30 s occupied visit halts correlation.\n'
            'Export, remove the puck, reset the board, then reconnect for a new walk.\n'
            'No measured intelligence, optimality, safety or full-path claim is made.\n'
            'Replay is never live. Reset inference cannot identify every silent reboot.'
        ),justify='left').pack(anchor='w',pady=16)
        ttk.Label(live,textvariable=self.summary,font=('TkDefaultFont',14),justify='left').pack(anchor='w',pady=12)
        ttk.Label(live,textvariable=self.contacts,justify='left').pack(anchor='w')
        columns=('ms','state','visits','loops','erased','route')
        self.history=ttk.Treeview(history,columns=columns,show='headings',height=18)
        for column in columns:
            self.history.heading(column,text=column)
            self.history.column(column,width=140)
        self.history.pack(fill='both',expand=True)
        ttk.Label(history,text='Last 2,000 snapshots; this is not a complete visit event log.').pack(anchor='w')
        ttk.Label(root,textvariable=self.status,wraplength=1000).pack(fill='x',padx=12,pady=10)
        self.timer=root.after(25,self.poll)
        root.protocol('WM_DELETE_WINDOW',self.close)

    def connect(self):
        if self.port:
            self.status.set('Serial already connected; disconnect first.'); return
        try: port=Serial(self.path.get())
        except Exception as exc:
            self.status.set(f'Connect failed: {exc}'); return
        self.port=port; self.decoder=Decoder(); self.session=Session('live')
        self.session.connected=True
        self.status.set('Connected; awaiting advancing frames.'); self.render()

    def disconnect(self):
        if self.port:
            self.port.close(); self.port=None
            try: self.decoder.finish()
            except ValueError: pass
            self.session.errors=self.decoder.errors
        self.session.connected=False
        self.status.set('Disconnected; evidence retained, not live.'); self.render()

    def replay(self, path=None):
        if self.port:
            self.status.set('Disconnect before replay.'); return
        path=path or filedialog.askopenfilename(title='Open MR1 recording')
        if not path: return
        try: replacement=load_replay(path)
        except Exception as exc:
            self.status.set(f'Replay failed: {exc}'); return
        self.session=replacement
        self.status.set('REPLAY — never live hardware.'); self.render()

    def save(self, path=None):
        path=path or filedialog.asksaveasfilename(defaultextension='.json',title='Export to a NEW file')
        if not path: return
        try:
            self.session.export(path,self.note.get()); self.status.set(f'Exported: {path}')
        except Exception as exc: self.status.set(f'Export failed: {exc}')

    def render(self):
        self.history.delete(*self.history.get_children())
        for r in self.session.rows:
            self.history.insert('', 'end',values=(r['now'],STATES[r['state']],r['visits'],
                r['loops'],r['erased'],route(r)))
        if not self.session.rows:
            self.summary.set('No evidence'); self.contacts.set('No input sample'); return
        r=self.session.rows[-1]
        pending='none' if r['pending']==4 else 'ABCD'[r['pending']]
        self.summary.set(f"{STATES[r['state']]}\nRetained checkpoint route: {route(r)}\n"
                         f"Visits {r['visits']}   Loops {r['loops']}   Repeats {r['repeats']}\n"
                         f"Erased entries {r['erased']}   Added entries {r['added']}\n"
                         f"Pending {pending}; clear interval {r['clear_ms']} ms\n"
                         f"Fault: {reasons(r['fault'])}")
        inputs='\n'.join(f"Gate {name}: {'CLOSED' if r['mask']&(1<<i) else 'open'}"
                         f" {'qualified' if r['qualified']&(1<<i) else 'UNKNOWN'}"
                         for i,name in enumerate('ABCD'))
        self.contacts.set(inputs+f"\nSampling gaps {r['gaps']}; transport errors {r['tx_errors']}")

    def poll(self):
        if self.port:
            try:
                rows=self.decoder.feed(self.port.read())
                self.session.errors=self.decoder.errors
                for row in rows: self.session.accept(row)
                if rows: self.render()
                if self.session.fresh(): self.status.set(f'LIVE stream; parser errors {self.session.errors}; check device fault separately')
                else: self.status.set('WAITING / STALE — no fresh advancing stream.')
            except Exception as exc:
                self.disconnect(); self.session.warning=str(exc)
                self.status.set(f'Disconnected: {exc}. Evidence retained.')
        self.timer=self.root.after(25,self.poll)

    def close(self):
        self.root.after_cancel(self.timer)
        self.disconnect(); self.root.destroy()

if __name__=='__main__':
    window=tk.Tk(); Workbench(window); window.mainloop()
