# Author: jayis1. SPDX-License-Identifier: MIT
"""Local Linux/Tk Pattern Sled workbench. Receive-only serial or explicit replay."""
import argparse
import termios
import time
import tkinter as tk
from tkinter import ttk, filedialog
from model import Session, STATES, FAULTS
from transport import Serial, Decoder, load_replay

class Workbench:
    def __init__(self,root):
        self.root=root; self.port=None; self.decoder=Decoder(); self.session=Session('replay')
        root.title('Pattern Sled — jayis1'); root.geometry('1000x700')
        self.status=tk.StringVar(value='Disconnected. No hardware data.')
        ttk.Label(root,textvariable=self.status).pack(fill='x',padx=8,pady=8)
        self.tabs=ttk.Notebook(root); self.tabs.pack(fill='both',expand=True)
        connect=ttk.Frame(self.tabs); self.pattern=ttk.Frame(self.tabs)
        history=ttk.Frame(self.tabs); diagnostics=ttk.Frame(self.tabs)
        for widget,title in [(connect,'Connect / protocol'),(self.pattern,'Pattern'),(history,'History'),(diagnostics,'Diagnostics / setup')]:
            self.tabs.add(widget,text=title)
        self.path=tk.StringVar(value='/dev/ttyACM0')
        ttk.Label(connect,text='Linux serial path, 115200 8N1. Connect does not reset or arm the reader.').pack(pady=10)
        ttk.Entry(connect,textvariable=self.path,width=60).pack(pady=10)
        ttk.Button(connect,text='Connect',command=lambda:self.connect(self.path.get())).pack(pady=5)
        ttk.Button(connect,text='Disconnect (retain evidence)',command=self.disconnect).pack(pady=5)
        ttk.Button(connect,text='Open recording (explicit replay)',command=self.choose_replay).pack(pady=5)
        ttk.Button(connect,text='Export retained evidence to new CSV',command=self.choose_export).pack(pady=5)
        ttk.Label(connect,text='No network, device commands, remote configuration, or background uploads.\n'
                  'PS1 fixed policy: 16 words, 3 data tracks, setup/rearm 20 ms, high 10–999 ms.\n'
                  'Reset the MCU with the strip at its marked start to learn a NEW reference.').pack(pady=20)
        self.summary=tk.StringVar(value='No accepted frames')
        ttk.Label(self.pattern,textvariable=self.summary).pack(pady=10)
        self.grid=ttk.Treeview(self.pattern,columns=('slot','reference','latest','xor','errors','visits'),show='headings',height=16)
        for name in self.grid['columns']:
            self.grid.heading(name,text=name); self.grid.column(name,width=125,anchor='center')
        self.grid.pack(fill='both',expand=True,padx=10)
        self.history=ttk.Treeview(history,columns=('ms','words','bad','state','fault'),show='headings')
        for name in self.history['columns']:
            self.history.heading(name,text=name); self.history.column(name,width=140)
        self.history.pack(fill='both',expand=True)
        self.details=tk.StringVar()
        ttk.Label(diagnostics,textvariable=self.details,wraplength=920,justify='left').pack(padx=20,pady=20)
        ttk.Label(diagnostics,text='Setup checklist\n\n'
            '1. Detached voltage-free switches only; connect ground before signal.\n'
            '2. Keep clock low. Put the first reference card before the reading line.\n'
            '3. Reset MCU; close/reopen connection to establish a new host session.\n'
            '4. Advance three-track words with data stable before clock and through release.\n'
            '5. First 16 accepted pulses teach the pattern; subsequent groups compare.\n'
            '6. HALTED means ordinal alignment is lost. Export, realign, then reset.\n\n'
            'Latest is the last comparison at EACH slot, not necessarily one coherent repeat.\n'
            'A skipped pulse or whole missed repeat may be undetectable. This is not an interlock.',
            justify='left').pack(padx=20,pady=20)
        root.protocol('WM_DELETE_WINDOW',self.close)
        self.timer=root.after(25,self.poll)
        self.render()
    def error(self,exc):
        self.session.warning=str(exc); self.status.set(str(exc)); self.render()
    def connect(self,path):
        if self.port is not None:
            self.error(ValueError('Disconnect the owned serial port first')); return
        try: port=Serial(path)
        except (OSError,termios.error) as exc:
            self.error(exc); return
        self.port=port; self.decoder=Decoder(); self.session=Session('live'); self.render()
    def disconnect(self):
        if self.port is not None:
            self.port.close(); self.port=None
            if self.decoder.buffer:
                self.decoder.errors+=1; self.decoder.buffer=b''
            self.session.errors=self.decoder.errors
        self.session.connected=False; self.render()
    def replay(self,path):
        if self.port is not None:
            self.error(ValueError('Disconnect before opening replay')); return
        try: replacement=load_replay(path)
        except (OSError,ValueError) as exc:
            self.error(exc); return
        self.session=replacement; self.decoder=Decoder(); self.render()
    def choose_replay(self):
        path=filedialog.askopenfilename(title='PS1 recording — replay, not live')
        if path: self.replay(path)
    def choose_export(self):
        path=filedialog.asksaveasfilename(title='NEW file only; existing files refused',defaultextension='.csv')
        if path:
            try: self.session.export(path)
            except (OSError,ValueError) as exc: self.error(exc)
    def poll(self):
        if self.port is not None:
            try:
                data=self.port.read()
                rows=self.decoder.feed(data)
                self.session.errors=self.decoder.errors
                for row in rows: self.session.accept(row)
            except (OSError,ValueError,termios.error) as exc:
                self.session.warning=str(exc); self.disconnect()
        self.render()
        self.timer=self.root.after(25,self.poll)
    def render(self):
        s=self.session
        mode='LIVE / progressing' if s.fresh() else ('LIVE / waiting or stale' if s.connected else 'DISCONNECTED')
        if s.source=='replay': mode='REPLAY / NOT LIVE'
        if s.blocked: mode+=' / BLOCKED'
        self.status.set(mode+' — '+s.warning)
        self.details.set(f'Author: jayis1\nSource: {s.source}; connected: {s.connected}; parser errors: {s.errors}; '
                         f'blocked: {s.blocked}\n{ s.warning }\n'
                         'Fresh transport does not imply valid acquisition. Inspect terminal fault and partial learning.\n'
                         'History is bounded to the latest 2048 snapshots. Export includes provenance and diagnostics.')
        # Rebuild on changed row; age/status still refresh on every timer tick.
        row=s.rows[-1] if s.rows else None
        key=(id(s),tuple(row.values()) if row else None,len(s.rows))
        if key==getattr(self,'rendered',None): return
        self.rendered=key
        self.grid.delete(*self.grid.get_children()); self.history.delete(*self.history.get_children())
        if row is None:
            self.summary.set('No accepted frames'); return
        from model import visits
        self.summary.set(f"Words {row['words']} | compared {max(0,row['words']-16)} | mismatches {row['bad']} | "
                         f"{STATES[row['state']]} | fault: {FAULTS[row['fault']]}")
        for i in range(16):
            learned=row['words']>i; count=visits(row['words'],i)
            ref=row[f'ref{i}']; last=row[f'latest{i}']
            self.grid.insert('','end',values=(i,format(ref,'03b') if learned else 'unlearned',
                format(last,'03b') if count else 'unseen',format(ref^last,'03b') if count else '—',row[f'errors{i}'],count))
        for r in s.rows:
            self.history.insert('','end',values=(r['now'],r['words'],r['bad'],STATES[r['state']],FAULTS[r['fault']]))
    def close(self):
        self.root.after_cancel(self.timer); self.disconnect(); self.root.destroy()

def main():
    parser=argparse.ArgumentParser(description='Pattern Sled companion — author jayis1')
    parser.add_argument('--replay'); args=parser.parse_args()
    root=tk.Tk(); app=Workbench(root)
    if args.replay: app.replay(args.replay)
    root.mainloop()
if __name__=='__main__': main()
