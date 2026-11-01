#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Braid Trace: local Linux/Tk declaration diagram and evidence export."""
import termios
import tkinter as tk
from tkinter import ttk, filedialog
from model import Session, STATES, TOKENS, word, analyse
from transport import Decoder, Serial, load_replay

class Workbench:
    def __init__(self,root):
        self.root=root; root.title('Braid Trace — jayis1'); root.geometry('1040x740')
        self.port=None; self.decoder=Decoder(); self.session=Session('replay')
        self.path=tk.StringVar(value='/dev/ttyACM0')
        self.label=tk.StringVar(value='three captive strands A B C; declared crossings')
        self.status=tk.StringVar(value='Disconnected; no evidence')
        self.summary=tk.StringVar(value='No samples')
        self.tabs=ttk.Notebook(root); self.tabs.pack(fill='both',expand=True)
        pages=[ttk.Frame(self.tabs,padding=12) for _ in range(4)]
        for page,title in zip(pages,('Connection / replay','Crossing diagram','History','Setup / export')):
            self.tabs.add(page,text=title)
        connection,live,history,settings=pages
        ttk.Label(connection,text='Linux serial path; 115200 8N1; receive only').pack(anchor='w')
        ttk.Entry(connection,textvariable=self.path,width=70).pack(anchor='w')
        for text,command in [('Connect',self.connect),('Disconnect',self.disconnect),('Open replay',self.replay)]:
            ttk.Button(connection,text=text,command=command).pack(anchor='w',pady=5)
        ttk.Label(connection,text='Before starting: reset MCU with strands A B C laid out separately.\n'
                  'Check left detent=1, right detent=2, OVER=4 and UNDER=8 in the live contact mask.\n'
                  'An open wire can imitate neutral. Inspect every input before braiding.').pack(anchor='w',pady=15)
        ttk.Label(root,textvariable=self.status,wraplength=1000).pack(fill='x')
        ttk.Label(live,textvariable=self.summary,wraplength=980).pack(anchor='w')
        self.diagram=tk.Canvas(live,width=920,height=520,background='white')
        self.diagram.pack(fill='both',expand=True)
        self.history=ttk.Treeview(history,columns=('time','accepted','rejected','aborted','word'),show='headings')
        for c in ('time','accepted','rejected','aborted','word'):
            self.history.heading(c,text=c); self.history.column(c,width=100 if c!='word' else 560)
        self.history.pack(fill='both',expand=True)
        ttk.Label(settings,text='BT1 policy — Author: jayis1\n\n'
                  'Viewed from the clamped origin toward the free ends: select the LEFT or RIGHT adjacent pair.\n'
                  'OVER declares the left member over the right; UNDER declares it under.\n'
                  'Keep the detent fixed through a complete press/release; pause at least 20 ms per state.\n'
                  'Use the complete 32-crossing word as evidence. Adjacent inverse reduction is NOT a braid solver.\n'
                  'Identity endpoint order is NOT proof of an unbraided object.\n'
                  'The guide cannot verify thread identity, tension, a completed crossing or physical undo.\n'
                  'Any acquisition gap, timeout or capacity overflow permanently halts this session.\n'
                  'Export, physically restore the origin, reset MCU, then reconnect to start again.\n'
                  'Reconnect alone neither clears nor repairs the device word.\n'
                  'Snapshots retained: 2000. No cloud, commands, authentication or persistent device memory.',
                  wraplength=980).pack(anchor='w')
        ttk.Entry(settings,textvariable=self.label,width=100).pack(anchor='w',pady=15)
        ttk.Button(settings,text='Export private CSV; no overwrite',command=self.export).pack(anchor='w')
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
                state='HALTED device' if self.session.rows and self.session.rows[-1]['state']==3 else 'LIVE transport'
                self.status.set(state if self.session.fresh() else 'Waiting / STALE')
            except (OSError,ValueError) as exc:
                self.disconnect(); self.status.set(str(exc))
        self.timer=self.root.after(30,self.poll)

    def render(self):
        self.diagram.delete('all'); self.history.delete(*self.history.get_children())
        if not self.session.rows: self.summary.set('No samples'); return
        r=self.session.rows[-1]; codes=word(r); reduced,p,pos,neg=analyse(codes)
        order=' '.join('ABC'[(p//(3**i))%3] for i in range(3))
        short=' '.join(('L' if abs(x)==1 else 'R')+('+' if x>0 else '-') for x in reduced) or '(empty)'
        self.summary.set(f"{STATES[r['state']]} | {r['accepted']} / 32 declared crossings | endpoint order {order}\n"
                         f"Immediate inverse reduction: {short} | exponent sum {pos-neg}\n"
                         f"NOT proof of physical topology or complete braid equivalence. Rejected {r['rejected']}, "
                         f"aborted {r['aborted']}, contacts {r['levels']}, faults {r['faults']}, gaps {r['gaps']}")
        strands=[0,1,2]; colors=['#a52a2a','#146694','#427a25']; step=14
        for i in range(3): self.diagram.create_text(140+i*200,15,text='ABC'[i],fill=colors[i])
        for n,code in enumerate(codes):
            at=(code-1)%2; y=30+n*step
            # Draw under first, then a white clearance and the over strand.
            over=at if code<=2 else at+1; under=at+1 if code<=2 else at
            for slot in range(3):
                if slot not in (at,at+1):
                    self.diagram.create_line(140+slot*200,y,140+slot*200,y+step,fill=colors[strands[slot]],width=3)
            x1=140+under*200; x2=140+over*200
            self.diagram.create_line(x1,y,x2,y+step,fill=colors[strands[under]],width=3)
            self.diagram.create_line(x2,y,x1,y+step,fill='white',width=8)
            self.diagram.create_line(x2,y,x1,y+step,fill=colors[strands[over]],width=3,tags='crossing')
            self.diagram.create_text(630,y+7,text=f'{n+1}: {TOKENS[code]}',anchor='w')
            strands[at],strands[at+1]=strands[at+1],strands[at]
        previous=None
        for row in self.session.rows:
            key=tuple(row[k] for k in ('accepted','rejected','aborted'))
            if key!=previous:
                self.history.insert('','end',values=(row['now'],*key,' '.join(TOKENS[c] for c in word(row))))
            previous=key

    def replay_path(self,path):
        if self.port is not None: raise ValueError('disconnect before replacing evidence')
        session=load_replay(path)
        self.session=session; self.decoder=Decoder(); self.render()
        self.status.set('REPLAY — never live; source may be synthetic')

    def replay(self):
        path=filedialog.askopenfilename(title='Open BT1 recording')
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
