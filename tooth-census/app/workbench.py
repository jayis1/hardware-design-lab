#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Local Linux Tk notebook. No network, command transmission or pip packages."""
import argparse
import termios
import tkinter as tk
from tkinter import filedialog, messagebox, ttk
from model import Session, STATES, REASONS
from transport import Decoder, Serial, load_replay

class Workbench:
    def __init__(self,root):
        self.root=root
        self.serial=None
        self.decoder=Decoder()
        self.session=Session('replay')
        root.title('Tooth Census — jayis1')
        root.geometry('1050x650')
        self.port=tk.StringVar(value='/dev/ttyACM0')
        self.status=tk.StringVar(value='Disconnected — no evidence')
        self.summary=tk.StringVar(value='No accepted telemetry')
        self.capture=tk.StringVar(value='')
        bar=ttk.Frame(root); bar.pack(fill='x',padx=8,pady=8)
        ttk.Entry(bar,textvariable=self.port,width=24).pack(side='left')
        for label,fn in [('Connect',lambda:self.connect(self.port.get())),
                         ('Disconnect',self.disconnect),('Replay',self.choose_replay),
                         ('Export CSV',self.choose_export)]:
            ttk.Button(bar,text=label,command=lambda f=fn:self.safe(f)).pack(side='left',padx=4)
        ttk.Label(root,textvariable=self.status).pack(fill='x',padx=10)
        book=ttk.Notebook(root); book.pack(fill='both',expand=True,padx=8,pady=8)
        live=ttk.Frame(book); book.add(live,text='Gear comparison')
        ttk.Label(live,textvariable=self.summary,font=('TkDefaultFont',15)).pack(pady=20)
        ttk.Label(live,textvariable=self.capture).pack(pady=15)
        ttk.Label(live,text='Accepted capture ≠ acceptable gear. Counts are follower pulses, not inspected teeth.\n'
                  'First accepted capture is the volatile reference. Reboot for a different gear.\n'
                  'Hand operation only; index window must lie in a tooth valley.').pack(pady=15)
        history=ttk.Frame(book); book.add(history,text='Snapshot notebook')
        self.columns=('now','state','good','bad','reference','last','minimum','maximum','mismatch','reason')
        self.table=ttk.Treeview(history,columns=self.columns,show='headings',height=20)
        for key in self.columns:
            self.table.heading(key,text=key)
            self.table.column(key,width=90,stretch=True)
        scroll=ttk.Scrollbar(history,orient='vertical',command=self.table.yview)
        self.table.configure(yscrollcommand=scroll.set)
        scroll.pack(side='right',fill='y'); self.table.pack(fill='both',expand=True)
        about=ttk.Frame(book); book.add(about,text='Setup and limits')
        ttk.Label(about,justify='left',wraplength=900,text=(
            'Author and creator: jayis1\n\nTC1: 96 quadrature transitions per revolution, five-sample '
            'qualification, rejecting deadline 120 seconds. Either initial direction is allowed; '
            'reversal rejects the capture. No controls on this screen alter firmware.\n\n'
            'J3 encoder A; J4 encoder B; J5 once-per-turn index; J6 tooth follower. '
            'Only disconnected voltage-free contacts, maximum 0.5 m leads, common USB ground. '
            'No motors, mains, energized machine contacts or safety use.\n\n'
            'Replay is synthetic or recorded evidence, never a live instrument. CSV preserves provenance, '
            'parser errors and warnings. The notebook retains at most 512 snapshots, not every tooth. '
            'A receive pause or contradictory history requires a new connection. '
            'A restarted MCU can sometimes look like forward traffic; this protocol has no persistent boot ID.\n\n'
            'Prototype not physically validated. See README.md for wiring, mechanical mounting and acceptance tests.'
        )).pack(anchor='nw',padx=20,pady=20)
        self.timer=root.after(50,self.tick)
    def safe(self,fn):
        try: fn()
        except (OSError,ValueError,termios.error) as exc:
            messagebox.showerror('Tooth Census',str(exc),parent=self.root)
    def connect(self,path):
        if self.serial is not None: raise ValueError('Disconnect current port first')
        port=Serial(path)  # Failure preserves existing evidence.
        self.serial=port
        self.decoder=Decoder()
        self.session=Session('live'); self.session.connected=True
        self.render()
    def disconnect(self):
        if self.serial is not None:
            self.serial.close(); self.serial=None
            if self.decoder.buffer:
                self.decoder.errors+=1
                self.decoder.buffer=b''
            self.session.errors=self.decoder.errors
        self.session.connected=False
        self.render()
    def replay(self,path):
        replacement=load_replay(path)  # Validate fully before replacing evidence.
        self.disconnect(); self.session=replacement
        self.render()
    def export(self,path):
        self.session.export(path)
    def choose_replay(self):
        path=filedialog.askopenfilename(parent=self.root,title='Open TC1 recording')
        if path: self.replay(path)
    def choose_export(self):
        path=filedialog.asksaveasfilename(parent=self.root,title='New evidence CSV',defaultextension='.csv')
        if path: self.export(path)
    def poll(self):
        if self.serial is None: return
        try:
            rows=self.decoder.feed(self.serial.read())
            self.session.errors=self.decoder.errors
            for row in rows: self.session.accept(row)
        except (OSError,ValueError,termios.error) as exc:
            self.session.warning=str(exc)
            self.disconnect()
        self.render()
    def render(self):
        s=self.session
        self.status.set(f'{s.provenance} | connected={s.connected} | advancing={s.fresh()} | '
                        f'parser errors={s.errors} | {s.warning}')
        self.table.delete(*self.table.get_children())
        for row in s.rows:
            self.table.insert('', 'end',values=[STATES[row[k]] if k=='state' else row[k] for k in self.columns])
        if not s.rows:
            self.summary.set('No accepted telemetry'); self.capture.set(''); return
        r=s.rows[-1]
        self.summary.set(f"Accepted {r['good']}   Rejected {r['bad']}   Count mismatches {r['mismatch']}\n"
                         f"Reference {r['reference']}   Last {r['last']}   Range {r['minimum']}–{r['maximum']}")
        self.capture.set(f"{STATES[r['state']]} | pending edges {r['steps']}/96 | pulses {r['teeth']} | age {r['age']} ms\n"
                         f"Last accepted duration {r['last_ms']} ms | last rejection: {REASONS[r['reason']]}")
    def tick(self):
        self.poll()
        if self.serial is None: self.render()
        self.timer=self.root.after(50,self.tick)
    def close(self):
        self.root.after_cancel(self.timer)
        self.disconnect()

if __name__=='__main__':
    parser=argparse.ArgumentParser(description='Tooth Census by jayis1')
    parser.add_argument('--replay'); args=parser.parse_args()
    root=tk.Tk(); app=Workbench(root)
    if args.replay: app.safe(lambda:app.replay(args.replay))
    def quit_app():
        app.close(); root.destroy()
    root.protocol('WM_DELETE_WINDOW',quit_app)
    root.mainloop()
