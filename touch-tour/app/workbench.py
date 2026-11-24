#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Linux desktop for Touch Tour. Run with the system Python/Tk interpreter."""
import argparse
import termios
import tkinter as tk
from tkinter import ttk, filedialog, messagebox
from model import Session, VISITS, order_ids
from transport import Decoder, Serial, load_replay

class Workbench:
    def __init__(self,root):
        self.root=root
        root.title('Touch Tour — jayis1')
        root.geometry('980x620')
        self.serial=None
        self.decoder=Decoder()
        self.session=Session('none')
        self.timer=None
        top=ttk.Frame(root); top.pack(fill='x',padx=8,pady=8)
        self.path=tk.StringVar(value='/dev/ttyACM0')
        ttk.Entry(top,textvariable=self.path,width=30).pack(side='left')
        for label,fn in [('Connect',lambda:self.connect(self.path.get())),
                         ('Disconnect',self.disconnect),('Replay TT1',self.choose_replay),
                         ('Export JSON',self.choose_export)]:
            ttk.Button(top,text=label,command=lambda f=fn:self.action(f)).pack(side='left',padx=4)
        self.status=tk.StringVar(value='Disconnected; no evidence')
        ttk.Label(root,textvariable=self.status).pack(fill='x',padx=8)
        self.tabs=ttk.Notebook(root); self.tabs.pack(expand=True,fill='both',padx=8,pady=8)
        overview=ttk.Frame(self.tabs); history=ttk.Frame(self.tabs); setup=ttk.Frame(self.tabs)
        self.tabs.add(overview,text='Coverage'); self.tabs.add(history,text='Snapshots')
        self.tabs.add(setup,text='Wiring and limits')
        self.summary=tk.StringVar(value='No observations')
        ttk.Label(overview,textvariable=self.summary,justify='left',font=('TkDefaultFont',13)).pack(anchor='w',padx=12,pady=12)
        self.pads=[]
        for i,name in enumerate('ABCD'):
            variable=tk.StringVar(value=f'Pad {name}: no observations')
            self.pads.append(variable)
            ttk.Label(overview,textvariable=variable).pack(anchor='w',padx=12,pady=6)
        self.table=ttk.Treeview(history,columns=('time','samples','mask','rounds','repeat','aborted'),show='headings')
        for key in self.table['columns']:
            self.table.heading(key,text=key); self.table.column(key,width=130)
        self.table.pack(expand=True,fill='both')
        text=tk.Text(setup,wrap='word'); text.pack(expand=True,fill='both')
        text.insert('1.0',
            'Author: jayis1\n\nTouch Tour revision A: unbuilt dry-practice prototype.\n\n'
            'J1: pin 1 = Nucleo CN6.4 3V3, pin 2 = CN6.6 GND.\n'
            'J2 pins 1..4 = CN8.1..4 = PA0, PA1, PA4, PB0.\n'
            'J3..J6: pads A..D. Each pin 1 signal, pin 2 GND.\n'
            'Only isolated voltage-free contacts; never powered machine wiring.\n\n'
            'Release every pad for at least six observations before each press. '
            'Hold one pad for at least six observations. Any first-visit order is allowed. '
            'Repeats do not advance distinct coverage. Four distinct visits close a round. '
            'Raw mixed-pad envelopes abort incomplete rounds; sixty seconds also aborts. '
            'Sampling gaps abandon partial coverage. The next round needs an all-clear baseline.\n\n'
            'This device observes contact visits, not cleaning, effort, force, identity or compliance. '
            'Do not use liquids, disinfectants, dangerous tools or medical/production tasks.\n\n'
            'Snapshots are bounded to the newest 1000. Replay is never live. '
            'Exports are owner-only JSON, refuse overwrite, and retain source and warnings. '
            'Reboots can be undetectable without a persistent boot identity. '
            'Reconnect after any restart, stale acquisition or changed fixture. '
            'A reconnect starts a new host notebook, not a device reset.')
        text.configure(state='disabled')
        root.protocol('WM_DELETE_WINDOW',self.close)
        self.schedule()
    def action(self,fn):
        try: fn()
        except (ValueError,OSError,termios.error) as exc:
            messagebox.showerror('Touch Tour',str(exc),parent=self.root)
    def connect(self,path):
        if self.serial is not None:
            raise ValueError('Disconnect before connecting another port')
        serial=Serial(path)  # Failed opening does not erase retained evidence.
        self.serial=serial
        self.decoder=Decoder()
        self.session=Session('live'); self.session.connected=True
        self.render()
    def disconnect(self):
        if self.serial:
            self.serial.close(); self.serial=None
            if self.decoder.buffer:
                self.decoder.errors+=1; self.decoder.buffer=b''
            self.session.errors=self.decoder.errors
            self.session.fragments=self.decoder.initial_fragments
        self.session.connected=False
        self.render()
    def replay(self,path):
        if self.serial:
            raise ValueError('Disconnect before replacing evidence with replay')
        replacement=load_replay(path)
        self.session=replacement
        self.render()
    def choose_replay(self):
        path=filedialog.askopenfilename(parent=self.root,title='Open TT1 recording (not export JSON)')
        if path: self.replay(path)
    def choose_export(self):
        path=filedialog.asksaveasfilename(parent=self.root,defaultextension='.json',title='New evidence file; overwrite refused')
        if path:
            self.session.fresh()
            self.session.export(path)
    def poll(self):
        if self.serial:
            try:
                self.session.fresh()
                if self.session.blocked:
                    raise ValueError(self.session.blocked)
                data=self.serial.read()
                rows=self.decoder.feed(data)
                self.session.errors=self.decoder.errors
                self.session.fragments=self.decoder.initial_fragments
                for row in rows: self.session.accept(row)
            except (ValueError,OSError,termios.error) as exc:
                self.session.errors=self.decoder.errors
                self.session.blocked=str(exc)
                self.disconnect()
        self.render()
    def schedule(self):
        self.poll()
        self.timer=self.root.after(50,self.schedule)
    def render(self):
        live=self.session.connected and self.session.fresh()
        self.status.set(f'{self.session.provenance} | '+('advancing acquisition' if live else 'NOT LIVE')+
                        f' | parser errors {self.session.errors} | {self.session.blocked}')
        self.table.delete(*self.table.get_children())
        for r in self.session.rows:
            self.table.insert('', 'end',values=(r['now'],r['samples'],f"{r['mask']:04b}",
                                                r['rounds'],r['repeats'],r['aborted']))
        if not self.session.rows:
            self.summary.set('No observations')
            for i,v in enumerate(self.pads): v.set(f'Pad {"ABCD"[i]}: no observations')
            return
        r=self.session.rows[-1]
        order='-'.join('ABCD'[i] for i in order_ids(r['last_order'],4)) if r['rounds'] else 'none'
        self.summary.set(f"Complete coverage rounds: {r['rounds']}\n"
                         f"Repeat visits: {r['repeats']}   Abandoned rounds: {r['aborted']}\n"
                         f"Current distinct mask: {r['mask']:04b}   Age: {r['age']} nominal ms\n"
                         f"Latest complete order: {order}   Duration: {r['last_age']} nominal ms\n"
                         f"Mixed envelopes: {r['disruptions']}   Timeouts: {r['timeouts']}\n"
                         f"Missed ticks: {r['missed']}   Fault mask: {r['fault']}\n"
                         'Contact evidence only — no cleaning or work-quality claim.')
        for i,v in enumerate(self.pads):
            v.set(f'Pad {"ABCD"[i]}: {r[VISITS[i]]} visits | '+
                  ('visited this round' if r['mask'] & (1<<i) else 'not yet visited this round'))
    def close(self):
        if self.timer is not None:
            self.root.after_cancel(self.timer); self.timer=None
        self.disconnect(); self.root.destroy()

def main():
    parser=argparse.ArgumentParser(description='Touch Tour desktop by jayis1')
    parser.add_argument('--replay')
    args=parser.parse_args()
    root=tk.Tk(); app=Workbench(root)
    if args.replay: app.action(lambda:app.replay(args.replay))
    root.mainloop()
if __name__=='__main__': main()
