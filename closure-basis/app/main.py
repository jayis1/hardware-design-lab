#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Closure Basis: local coverage workbench, replay and evidence export."""
import argparse
import csv
import os
import termios
import time
import tkinter as tk
from tkinter import ttk, filedialog, messagebox
from model import FIELDS, Session, mask_name, witness, closure, support, minimal_for
from transport import Serial, Decoder, load_replay

class App:
    def __init__(self, root):
        self.root = root
        self.root.title('Closure Basis — jayis1 — unbuilt prototype')
        self.serial = None
        self.decoder = Decoder()
        self.session = Session('empty')
        self.last_progress = None
        self.last_samples = None
        self.anchor_host = None
        self.device_elapsed = 0
        self.last_device = None
        self.warning = ''
        self.timer = None
        self.path = tk.StringVar(value='/dev/ttyACM0')
        self.status = tk.StringVar(value='disconnected; no evidence')
        self.summary = tk.StringVar(value='No acquired settings')
        self.suggestion = tk.StringVar(value='No suggestion')
        toolbar = ttk.Frame(root, padding=8); toolbar.pack(fill='x')
        ttk.Entry(toolbar, textvariable=self.path, width=32).pack(side='left')
        for name,action in [('Connect',lambda:self.connect(self.path.get())),
                            ('Disconnect',self.disconnect),('Replay',self.choose_replay),
                            ('Export CSV',self.choose_export)]:
            ttk.Button(toolbar,text=name,command=lambda a=action:self.invoke(a)).pack(side='left')
        ttk.Label(root,textvariable=self.status,wraplength=850).pack(fill='x',padx=8)
        book=ttk.Notebook(root); book.pack(fill='both',expand=True,padx=8,pady=8)
        coverage=ttk.Frame(book); book.add(coverage,text='Implications / counterexamples')
        ttk.Label(coverage,textvariable=self.summary).pack(anchor='w')
        ttk.Label(coverage,textvariable=self.suggestion).pack(anchor='w')
        self.table=ttk.Treeview(coverage,columns=('input','seen','state','support'),show='headings',height=16)
        for key,title in [('input','Antecedent'),('seen','Extra closures / evidence'),('state','Refuted bit:word'),('support','Distinct support')]:
            self.table.heading(key,text=title); self.table.column(key,width=220)
        self.table.pack(fill='both',expand=True)
        detail=ttk.Frame(book); book.add(detail,text='Acquisition / limits')
        self.details=tk.Text(detail,width=95,height=15,state='disabled'); self.details.pack(fill='both',expand=True)
        ttk.Label(root,text='Voltage-free contacts only. Stable observations do not prove causality or correctness.').pack()
        self.timer=root.after(25,self.poll)

    def invoke(self, action):
        try: action()
        except (OSError,ValueError,termios.error) as exc:
            messagebox.showerror('Closure Basis',str(exc),parent=self.root)

    def connect(self,path):
        if self.serial is not None: raise ValueError('Disconnect the owned port first')
        candidate=Serial(path)  # Failed opens must not erase retained evidence.
        self.serial=candidate
        self.decoder=Decoder(); self.session=Session('live serial')
        self.last_progress=time.monotonic(); self.last_samples=None
        self.anchor_host=None; self.last_device=None; self.device_elapsed=0
        self.warning=''; self.render()

    def disconnect(self):
        if self.serial:
            self.serial.close(); self.serial=None
        self.render()

    def replay(self,path):
        if self.serial is not None: raise ValueError('Disconnect before replay')
        candidate=load_replay(path)
        self.session=candidate; self.decoder=Decoder()
        self.last_progress=None; self.last_samples=None; self.anchor_host=None
        self.last_device=None; self.device_elapsed=0; self.warning=''
        self.render()

    def choose_replay(self):
        path=filedialog.askopenfilename(parent=self.root)
        if path: self.replay(path)

    def choose_export(self):
        path=filedialog.asksaveasfilename(parent=self.root,defaultextension='.csv')
        if path: self.export(path)

    def export(self,path):
        # Exclusive creation, private permissions, metadata even without rows.
        fd=os.open(path,os.O_WRONLY|os.O_CREAT|os.O_EXCL,0o600)
        with os.fdopen(fd,'w',newline='') as handle:
            writer=csv.writer(handle)
            writer.writerow(['author','jayis1'])
            writer.writerow(['provenance',self.session.provenance])
            writer.writerow(['connection',self.status.get()])
            writer.writerow(['warning',self.warning or self.session.error])
            writer.writerow(['parser_errors',self.decoder.errors])
            writer.writerow(['initial_fragments',self.decoder.initial_fragments])
            writer.writerow(['total_accepted',self.session.total])
            writer.writerow(['retained_rows',len(self.session.rows)])
            writer.writerow(['progress_age_seconds', '' if self.last_progress is None else
                             f'{time.monotonic()-self.last_progress:.3f}'])
            writer.writerow(FIELDS)
            for row in self.session.rows: writer.writerow([row[f] for f in FIELDS])

    def poll(self):
        try:
            if self.serial is not None:
                host=time.monotonic()
                if self.last_progress is not None and host-self.last_progress > 2:
                    raise ValueError('stale acquisition; reconnect required')
                rows=self.decoder.feed(self.serial.read())
                for row in rows:
                    advanced=self.session.accept(row)
                    if not advanced: continue
                    if self.anchor_host is None:
                        self.anchor_host=host; self.last_device=row['now']
                    else:
                        self.device_elapsed+=(row['now']-self.last_device)&0xffffffff
                        self.last_device=row['now']
                        # Fixed budget does not silently tolerate growing backlog forever.
                        if abs(self.device_elapsed/1000-(host-self.anchor_host)) > 1:
                            raise ValueError('device/host progress mismatch; reconnect required')
                    if self.last_samples is None or row['samples'] > self.last_samples:
                        self.last_progress=host; self.last_samples=row['samples']
                    if row['fault']: raise ValueError('terminal firmware fault '+str(row['fault']))
        except (OSError,ValueError,termios.error) as exc:
            self.warning=str(exc)
            if self.serial: self.serial.close(); self.serial=None
        finally:
            self.render()  # Earlier accepted rows survive a later failing row in this read.
            self.timer=self.root.after(25,self.poll)

    def render(self):
        mode='live' if self.serial is not None else 'disconnected'
        self.status.set(f'{mode}; {self.session.provenance}; parser errors {self.decoder.errors}; {self.warning}')
        for child in self.table.get_children(): self.table.delete(child)
        if not self.session.rows:
            self.summary.set('No acquired settings'); self.suggestion.set('No suggestion')
            text='No evidence. Connect a serial device or load a CB1 recording.'
        else:
            row=self.session.rows[-1]
            rules=[mask_name(a)+' -> '+name for bit,name in enumerate('ABCD')
                   for a in range(16) if minimal_for(row['settings'],bit)&(1<<a)]
            self.summary.set(f"{row['rules']} provisional minimal rules; {row['visits']} visits")
            m=row['suggestion']
            self.suggestion.set('All configurations observed; no nontrivial positive rules remain' if m==16 else
                'Unobserved A B C D: '+' '.join(str((m>>i)&1) for i in range(4))+'; investigate only if freely reachable, never force'+
                (' (historical suggestion)' if self.serial is None else ''))
            for mask in range(16):
                common=closure(row['settings'],mask)
                seen='UNOBSERVED' if common==16 else mask_name(common&~mask)+'; provisional'
                bad=[name+':'+str(w) for bit,name in enumerate('ABCD')
                     if (w:=witness(row['settings'],mask,bit)) is not None]
                self.table.insert('', 'end', values=(mask_name(mask),seen,', '.join(bad) or 'none',support(row['settings'],mask)))
            text='\n'.join(f'{key}: {row[key]}' for key in FIELDS)
            text+='\n\n100 consecutive unchanged 1 ms intervals qualify a setting.\n'
            text+='Raw disturbances and missing samples restart qualification.\n'
            text+='Raw word = A + 2*B + 4*C + 8*D. All channels are observed positions.\n'
            text+='Minimal antecedents per consequent (not a canonical basis):\n'+'\n'.join(rules)+'\n'
            text+='Empty support is UNOBSERVED, never proof of an implication or impossible motion.\n'
            text+='Only the latest 2000 accepted rows are retained for export. No persistent boot identity.'
        self.details.configure(state='normal'); self.details.delete('1.0','end')
        self.details.insert('1.0',text); self.details.configure(state='disabled')

    def close(self):
        if self.timer is not None: self.root.after_cancel(self.timer); self.timer=None
        if self.serial: self.serial.close(); self.serial=None

def main():
    parser=argparse.ArgumentParser(description='Closure Basis by jayis1')
    parser.add_argument('--replay')
    args=parser.parse_args()
    root=tk.Tk(); app=App(root)
    root.protocol('WM_DELETE_WINDOW',lambda:(app.close(),root.destroy()))
    if args.replay: app.invoke(lambda:app.replay(args.replay))
    root.mainloop()
if __name__=='__main__': main()
