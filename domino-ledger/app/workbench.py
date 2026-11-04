#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Local-only Linux/Tk companion for Domino Ledger."""
import termios
import tkinter as tk
from tkinter import ttk, filedialog
from model import Session, PHASES, OUTCOMES, OFFSETS, ABSENT
from transport import Serial, Decoder, load_replay

class Workbench:
    def __init__(self, root):
        self.root=root
        root.title('Domino Ledger — jayis1')
        root.geometry('1000x650')
        self.port=None
        self.session=Session('none')
        self.decoder=Decoder()
        self.message=tk.StringVar(value='Unbuilt prototype. Connect only isolated voltage-free contacts.')
        self.summary=tk.StringVar()
        self.result=tk.StringVar()
        self.contacts=tk.StringVar()
        self.freshness=tk.StringVar()
        self.path=tk.StringVar(value='/dev/ttyACM0')
        self.tabs=ttk.Notebook(root)
        self.tabs.pack(fill='both',expand=True)
        connection=ttk.Frame(self.tabs,padding=15)
        trial=ttk.Frame(self.tabs,padding=15)
        history=ttk.Frame(self.tabs,padding=15)
        self.tabs.add(connection,text='Connection')
        self.tabs.add(trial,text='Cascade')
        self.tabs.add(history,text='History / export')
        ttk.Label(connection,text='Local Linux serial port (115200 baud, 8N1)').pack(anchor='w')
        ttk.Entry(connection,textvariable=self.path,width=60).pack(anchor='w',pady=10)
        ttk.Button(connection,text='Connect',command=lambda:self.connect(self.path.get())).pack(anchor='w')
        ttk.Button(connection,text='Disconnect',command=self.disconnect).pack(anchor='w',pady=8)
        ttk.Button(connection,text='Open recording',command=self.choose_replay).pack(anchor='w')
        ttk.Label(connection,textvariable=self.freshness,wraplength=900).pack(anchor='w',pady=20)
        ttk.Label(connection,text='No network, actuation, automatic port opening or firmware upload.\n'
                  'Replay is synthetic or previously recorded evidence, never live acquisition.\n'
                  'Resetting the MCU clears volatile history; export before resetting.').pack(anchor='w')
        for variable in (self.summary,self.contacts,self.result):
            ttk.Label(trial,textvariable=variable,wraplength=900).pack(anchor='w',pady=15)
        ttk.Label(trial,text='Upright switches: A B C D. Equal offsets are unresolved ties.\n'
                  'Re-erected means a qualified return after a first fall, not a cause diagnosis.\n'
                  'Missing offsets mean no fall observed before finalization.\n'
                  '500 ms upright arms; 250 ms all-down closes; 10 s ends a partial trial.').pack(anchor='w',pady=20)
        columns=('time','trial','phase','outcome','seen','returned','offsets')
        self.history=ttk.Treeview(history,columns=columns,show='headings',height=18)
        for name in columns:
            self.history.heading(name,text=name)
            self.history.column(name,width=100 if name!='offsets' else 250)
        self.history.pack(fill='both',expand=True)
        ttk.Label(history,text='Latest 100 snapshots shown; exports retain up to 2000. Not a lossless trial database.').pack(anchor='w')
        ttk.Button(history,text='Export private CSV (new file)',command=self.choose_export).pack(anchor='w',pady=8)
        ttk.Label(root,textvariable=self.message,wraplength=950).pack(fill='x',padx=15,pady=10)
        root.protocol('WM_DELETE_WINDOW',self.close)
        self.render()
        self.timer=root.after(25,self.poll)
    def connect(self,path):
        if self.port:
            self.message.set('Disconnect before opening another serial port.')
            return
        try:
            port=Serial(path)
        except (OSError,termios.error) as exc:
            self.message.set('Connection failed: '+str(exc))
            return
        self.port=port
        self.session=Session('live')
        self.decoder=Decoder()
        self.message.set('Connected; waiting for a complete DL1 snapshot.')
        self.render()
    def disconnect(self):
        if self.port:
            self.port.close()
            self.port=None
        self.message.set('Disconnected; retained evidence is not live.')
        self.render()
    def replay(self,path):
        if self.port:
            self.message.set('Disconnect before replacing the session with replay.')
            return
        try:
            session=load_replay(path)
        except (OSError,ValueError) as exc:
            self.message.set('Replay rejected: '+str(exc))
            return
        self.session=session
        self.decoder=Decoder()
        self.message.set('Replay loaded; no physical measurement is implied.')
        self.render()
    def choose_replay(self):
        path=filedialog.askopenfilename(title='Open DL1 recording')
        if path: self.replay(path)
    def export(self,path):
        try:
            self.session.export(path,self.port is not None,self.decoder.errors)
            self.message.set('Exported '+str(path))
        except OSError as exc:
            self.message.set('Export failed: '+str(exc))
    def choose_export(self):
        path=filedialog.asksaveasfilename(title='New CSV filename',defaultextension='.csv')
        if path: self.export(path)
    def poll(self):
        if self.port:
            try:
                for row in self.decoder.feed(self.port.read()):
                    self.session.accept(row)
            except (OSError,ValueError,termios.error) as exc:
                self.port.close()
                self.port=None
                self.message.set('Capture stopped: '+str(exc))
        self.render()
        self.timer=self.root.after(25,self.poll)
    @staticmethod
    def offsets(row):
        return ' / '.join('—' if row[key]==ABSENT else str(row[key]) for key in OFFSETS)
    def render(self):
        r=self.session.latest
        fresh=self.session.fresh(self.port is not None)
        self.freshness.set(f'Provenance: {self.session.provenance}; connected: {self.port is not None}; '
                           f'recent acquisition: {fresh}; parser errors: {self.decoder.errors}. '
                           +self.session.warning)
        if not r:
            self.summary.set('No accepted snapshot.')
            self.contacts.set('Contacts unknown.')
            self.result.set('No trial evidence.')
        else:
            self.summary.set(f'{PHASES[r["phase"]]} | starts {r["starts"]} | completed {r["completed"]} | '
                             f'full {r["full"]} | partial {r["partial"]} | '
                             f're-erected {r["disturbed"]} | aborted {r["aborted"]}')
            self.contacts.set(f'Upright mask {r["mask"]:04b}; qualified {r["valid"]:04b}; '
                              f'raw {r["raw"]:04b}. Fault {r["fault"]}. '
                              f'Arm progress {r["baseline"]}/500 ms; quiet {r["quiet"]}/250 ms.')
            self.result.set(f'Trial {r["starts"]}: {OUTCOMES[r["outcome"]]}; age {r["age"]} ms.\n'
                            f'First falls A / B / C / D: {self.offsets(r)} ms.\n'
                            f'Seen mask {r["seen"]:04b}; re-erected mask {r["rebound"]:04b}.')
        # Repaint only when retained content changes, not on every timer tick.
        signature=(len(self.session.rows),id(self.session),tuple(r.values()) if r else ())
        if signature != getattr(self,'painted',None):
            self.painted=signature
            self.history.delete(*self.history.get_children())
            for row in list(self.session.rows)[-100:]:
                self.history.insert('', 'end', values=(row['now'],row['starts'],PHASES[row['phase']],
                                    OUTCOMES[row['outcome']],row['seen'],row['rebound'],self.offsets(row)))
    def close(self):
        if self.port:
            self.port.close()
            self.port=None
        self.root.after_cancel(self.timer)
        self.root.destroy()

if __name__=='__main__':
    root=tk.Tk()
    Workbench(root)
    root.mainloop()
