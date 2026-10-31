# Author: jayis1. SPDX-License-Identifier: MIT
"""Local Linux Tk companion; no network, telemetry service or device commands."""
import argparse
import termios
import tkinter as tk
from tkinter import ttk, filedialog
from model import Session
from transport import Decoder, Serial, load_replay

class Workbench:
    def __init__(self,root):
        self.root=root
        root.title('Coupon Transit — jayis1')
        root.geometry('1080x650')
        self.port=None
        self.decoder=Decoder()
        self.session=Session()
        self.path=tk.StringVar(value='/dev/ttyACM0')
        self.status=tk.StringVar(value='OFFLINE')
        self.message=tk.StringVar(value='Seat each labelled holder in exactly one matching dock')
        bar=ttk.Frame(root); bar.pack(fill='x')
        ttk.Entry(bar,textvariable=self.path,width=30).pack(side='left')
        for label,command in [('Connect',self.connect),('Disconnect',self.disconnect),
                              ('Replay',self.replay),('Export CSV',self.export)]:
            ttk.Button(bar,text=label,command=command).pack(side='left')
        ttk.Label(root,textvariable=self.status).pack(anchor='w')
        ttk.Label(root,textvariable=self.message,wraplength=1020).pack(anchor='w')
        self.tabs=ttk.Notebook(root); self.tabs.pack(fill='both',expand=True)
        live=ttk.Frame(self.tabs); history=ttk.Frame(self.tabs); setup=ttk.Frame(self.tabs)
        self.tabs.add(live,text='Holder transfers')
        self.tabs.add(history,text='Snapshot notebook')
        self.tabs.add(setup,text='Fixture and interpretation')
        cols=('phase','origin','delivered','returned','abandoned','last_ms','mean_ms','max_ms')
        self.holders=ttk.Treeview(live,columns=cols,height=2)
        self.holders.heading('#0',text='Holder'); self.holders.column('#0',width=80)
        for key in cols:
            self.holders.heading(key,text=key); self.holders.column(key,width=110)
        self.holders.pack(fill='x',pady=15)
        for p in ('a','b'): self.holders.insert('','end',iid=p,text=p.upper())
        self.summary=tk.StringVar(); ttk.Label(live,textvariable=self.summary,wraplength=1000).pack(anchor='w')
        ttk.Label(live,text='Last duration includes returns; mean and maximum include deliveries only.\n'
            'Dock observations do not verify specimen identity, drying, treatment or inspection.').pack(anchor='w',pady=20)
        self.history=ttk.Treeview(history,columns=('now','a_state','a_delivered','a_returned','b_state','b_delivered','b_returned','gaps'),show='headings')
        for key in self.history['columns']:
            self.history.heading(key,text=key); self.history.column(key,width=120)
        self.history.pack(fill='both',expand=True)
        text=tk.Text(setup,wrap='word'); text.pack(fill='both',expand=True)
        text.insert('1.0',
            'Author: jayis1\n\nJ3=A source, J4=A destination, J5=B source, J6=B destination. '
            'Each connector pin 1 is signal and pin 2 ground. Voltage-free contacts only. '
            'J1 receives Nucleo 3V3 and GND; J2 connects A0/A1/A2/A3. No powered sensor output.\n\n'
            'Use keyed, labelled coupon holders in two source and two destination pockets. '
            'Keep the electronics dry and use only inert classroom samples in this prototype. '
            'Switches report occupied docks, not RFID identity or completion of a process.\n\n'
            'Ten consecutive samples qualify each switch. Fifty milliseconds of raw/qualified agreement '
            'at exactly one dock arms that holder. Departure into neither dock starts a transit. '
            'Arrival at the opposite dock before 5000 ms records delivery; arrival back at the origin records return. '
            'At exactly 5000 ms timeout wins. Simultaneous presence or direct handover is ambiguous. '
            'Every outcome requires a new stable dock baseline. Holders operate independently.\n\n'
            'Gap, timeout or ambiguity abandons pending timing, not a physical specimen. '
            'Lost or shorted wires can resemble legitimate movement. Never use this as chain-of-custody evidence, '
            'a safety interlock, a clinical tracker or an acceptance test for treated samples.\n\n'
            'LIVE means advancing telemetry, not verified sensor operation. Replays are always offline. '
            'Export before reconnecting: connection replacement starts a fresh local notebook. '
            'Only 500 snapshots are retained. Faster moves may complete between snapshots, so counters '
            'are not a complete per-transfer event log. CRC detects corruption, not forgery. '
            'Some resets are indistinguishable without a persistent boot identity.\n')
        text.configure(state='disabled')
        root.protocol('WM_DELETE_WINDOW',self.close)
        self.job=root.after(20,self.poll)
        self.render()
    def connect(self):
        if self.port is not None:
            self.message.set('Already connected; disconnect first'); return
        try: candidate=Serial(self.path.get())
        except (OSError,termios.error) as exc:
            self.message.set('Connection failed: '+str(exc)); return
        self.port=candidate; self.decoder=Decoder()
        self.session=Session('live'); self.session.connected=True
        self.message.set('Connected; waiting for advancing CT1 records'); self.render()
    def disconnect(self):
        if self.port:
            self.port.close(); self.port=None
            try: self.decoder.finish()
            except ValueError: pass
            self.session.errors=self.decoder.errors
        self.session.connected=False; self.render()
    def replay_path(self,path):
        if self.port is not None: raise ValueError('Disconnect before replay')
        replacement=load_replay(path)
        self.session=replacement
        self.message.set('Offline replay; no physical experiment implied'); self.render()
    def replay(self):
        path=filedialog.askopenfilename()
        if path:
            try: self.replay_path(path)
            except (OSError,ValueError) as exc: self.message.set(str(exc))
    def export_path(self,path):
        self.session.export(path); self.message.set('Exported private CSV; existing files are never overwritten')
    def export(self):
        path=filedialog.asksaveasfilename(defaultextension='.csv')
        if path:
            try: self.export_path(path)
            except (OSError,ValueError) as exc: self.message.set(str(exc))
    def poll(self):
        if self.port is not None and not self.session.blocked:
            try:
                rows=self.decoder.feed(self.port.read())
                self.session.errors=self.decoder.errors
                for row in rows: self.session.accept(row)
            except (OSError,termios.error,ValueError) as exc:
                self.message.set(str(exc)); self.disconnect()
        self.render(); self.job=self.root.after(20,self.poll)
    def render(self):
        self.status.set(self.session.freshness()+' — source: '+self.session.source)
        r=self.session.rows[-1] if self.session.rows else None
        for p in ('a','b'):
            values=['—']*8
            if r:
                h=lambda k:r[f'{p}_{k}']
                values=[['WAIT','DOCKED','TRANSIT'][h('state')],['none','source','destination'][h('origin')],
                    h('delivered'),h('returned'),h('abandoned'),h('last_ms') if h('last_kind') else '—',
                    round(h('sum_ms')/h('delivered'),1) if h('delivered') else '—',h('max_ms') if h('delivered') else '—']
            self.holders.item(p,values=values)
        self.summary.set('No snapshot' if r is None else
            f"Contact mask {r['mask']:04b}; known {r['known']:04b}; gaps {r['gaps']}; flags {r['fault']}; parser errors {self.session.errors}\n"
            f"A ambiguities {r['a_ambiguities']}, timeouts {r['a_timeouts']}; B ambiguities {r['b_ambiguities']}, timeouts {r['b_timeouts']}")
        self.history.delete(*self.history.get_children())
        for row in self.session.rows:
            self.history.insert('','end',values=[row[k] for k in self.history['columns']])
    def close(self):
        self.root.after_cancel(self.job); self.disconnect(); self.root.destroy()

if __name__=='__main__':
    parser=argparse.ArgumentParser(description='Coupon Transit — author jayis1')
    parser.add_argument('--check',help='validate a recording without a display')
    args=parser.parse_args()
    if args.check:
        result=load_replay(args.check)
        print(f'Author: jayis1; {len(result.rows)} retained CT1 snapshots; offline replay')
    else:
        root=tk.Tk(); Workbench(root); root.mainloop()
