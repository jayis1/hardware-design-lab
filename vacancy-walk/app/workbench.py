#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Local Linux workbench. No network, credentials or outbound device commands."""
import argparse
from pathlib import Path
import termios
import tkinter as tk
from tkinter import ttk, filedialog, messagebox
from model import Session, STATES, RESULTS, FIELDS
from transport import Serial, Decoder, load_replay

class Workbench:
    def __init__(self,root):
        self.root=root
        self.serial=None
        self.decoder=Decoder()
        self.session=Session('live')
        self.closed=False
        root.title('Vacancy Walk — jayis1')
        root.geometry('1000x640')
        bar=ttk.Frame(root); bar.pack(fill='x')
        self.port=tk.StringVar(value='/dev/ttyACM0')
        ttk.Entry(bar,textvariable=self.port,width=28).pack(side='left')
        for label,command in [('Connect',self.connect),('Disconnect',self.disconnect),
                               ('Replay',self.replay_dialog),('Export CSV',self.export_dialog)]:
            ttk.Button(bar,text=label,command=command).pack(side='left')
        self.status=tk.StringVar(value='Disconnected; no evidence')
        ttk.Label(root,textvariable=self.status).pack(fill='x')
        self.tabs=ttk.Notebook(root); self.tabs.pack(fill='both',expand=True)
        trial=ttk.Frame(self.tabs); history=ttk.Frame(self.tabs); setup=ttk.Frame(self.tabs)
        for name,view in [('Pockets',trial),('History',history),('Setup',setup)]:
            self.tabs.add(view,text=name)
        self.canvas=tk.Canvas(trial,width=450,height=300,bg='white'); self.canvas.pack()
        self.summary=tk.StringVar(value='No snapshots')
        ttk.Label(trial,textvariable=self.summary,justify='left').pack()
        cols=('now','state','result','vacancy','moves','canceled','abandoned','last_ms')
        self.tree=ttk.Treeview(history,columns=cols,show='headings',height=18)
        for col in cols:
            self.tree.heading(col,text=col); self.tree.column(col,width=110)
        scrollbar=ttk.Scrollbar(history,command=self.tree.yview)
        self.tree.configure(yscrollcommand=scrollbar.set)
        self.tree.pack(side='left',fill='both',expand=True); scrollbar.pack(side='right',fill='y')
        ttk.Label(setup,text=(
            'Author: jayis1\n\nThree inert pucks, four pockets labelled clockwise 0–3.\n'
            'Only adjacent one-puck transfers. Wait for READY after each move.\n'
            'J3=0, J4=1, J5=2, J6=3; each pin 1 signal / pin 2 GND.\n'
            'J1: 3V3, GND. J2: Nucleo A0, A1, A2, A3.\n'
            'Voltage-free contacts only. No industrial signals or powered sensors.\n\n'
            'MOVED describes vacancy direction, not puck identity.\n'
            'HALT requires physical reset and a new connection/session.\n'
            'Replay is synthetic or recorded evidence, never live.\n'
            'CRC detects corruption, not malicious senders. No authentication.\n'
            'History retains latest 2000 snapshots; exports include truncation and errors.\n'
            'No hardware was physically validated; see README and VALIDATION.'
        ),justify='left',padding=20).pack(anchor='nw')
        root.protocol('WM_DELETE_WINDOW',self.close)
        self.timer=root.after(30,self.poll)
    def connect(self):
        if self.serial is not None:
            messagebox.showerror('Connection','Disconnect before opening another port'); return
        try:
            port=Serial(self.port.get())
        except (OSError,termios.error) as exc:
            messagebox.showerror('Connection',str(exc)); return
        self.serial=port; self.decoder=Decoder(); self.session=Session('live')
        self.render()
    def disconnect(self):
        if self.serial:
            if self.decoder.buffer: self.decoder.errors+=1; self.decoder.buffer=b''
            self.serial.close(); self.serial=None
        self.session.errors=self.decoder.errors
        self.render()
    def replay(self,path):
        # Parse replacement first, preserving retained evidence if it fails.
        candidate=load_replay(path)
        self.disconnect()
        self.decoder=Decoder(); self.session=candidate
        self.render()
    def replay_dialog(self):
        path=filedialog.askopenfilename(title='Open VW1 recording (max 2 MiB)')
        if not path: return
        try: self.replay(path)
        except (OSError,ValueError) as exc: messagebox.showerror('Replay',str(exc))
    def export_dialog(self):
        path=filedialog.asksaveasfilename(title='New CSV filename',defaultextension='.csv')
        if not path: return
        try: self.session.export(path,self.serial is not None)
        except (OSError,ValueError) as exc: messagebox.showerror('Export',str(exc))
    def poll(self):
        changed=False
        if self.serial:
            try:
                data=self.serial.read()
                rows=self.decoder.feed(data)
                self.session.errors=self.decoder.errors
                for row in rows:
                    self.session.accept(row); changed=True
            except (OSError,termios.error,ValueError) as exc:
                self.session.warning=str(exc)
                self.disconnect(); changed=True
        if changed: self.render()
        else: self.render_status()
        if not self.closed: self.timer=self.root.after(30,self.poll)
    def render_status(self):
        mode='LIVE advancing' if self.session.fresh(connected=self.serial is not None) else (
            'REPLAY' if self.session.origin=='replay' else 'NOT LIVE / stale or disconnected')
        self.status.set(f'{mode} | parser errors {self.session.errors} | {self.session.warning}')
    def render(self):
        self.render_status()
        self.canvas.delete('all')
        self.tree.delete(*self.tree.get_children())
        rows=list(self.session.rows)
        if not rows:
            self.summary.set('No snapshots'); return
        row=rows[-1]
        for i,(x,y) in enumerate(((140,60),(310,60),(310,220),(140,220))):
            filled=bool(row['qualified']&(1<<i))
            color='#63b38c' if filled else '#eeeeee'
            self.canvas.create_oval(x-45,y-35,x+45,y+35,fill=color)
            self.canvas.create_text(x,y,text=f'{i}: '+('occupied' if filled else 'empty'))
        self.summary.set(f"{STATES[row['state']]} / {RESULTS[row['result']]}\n"
                         f"Vacancy {row['vacancy']} | source {row['source']} (4 = absent)\n"
                         f"Moves {row['moves']} | canceled {row['canceled']} | abandoned {row['abandoned']}\n"
                         f"Vacancy CW {row['clockwise']} | CCW {row['counterclockwise']}\n"
                         f"Last lift {row['last_ms']} ms | active age {row['age']} ms | fault {row['fault']}")
        # Render every retained snapshot, not just the last row of a receive batch.
        for r in rows:
            self.tree.insert('', 'end',values=(r['now'],STATES[r['state']],RESULTS[r['result']],
                r['vacancy'],r['moves'],r['canceled'],r['abandoned'],r['last_ms']))
    def close(self):
        self.closed=True
        self.root.after_cancel(self.timer)
        self.disconnect()
        self.root.destroy()

def main():
    parser=argparse.ArgumentParser(description='Vacancy Walk by jayis1')
    parser.add_argument('--replay',type=Path)
    parser.add_argument('--check',type=Path,help='validate recording without opening GUI')
    args=parser.parse_args()
    if args.check:
        session=load_replay(args.check)
        print(f"Validated {len(session.rows)} retained snapshots; last result {RESULTS[session.rows[-1]['result']]}")
        return
    root=tk.Tk(); app=Workbench(root)
    if args.replay: app.replay(args.replay)
    root.mainloop()
if __name__=='__main__': main()
