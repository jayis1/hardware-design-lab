# Author: jayis1. SPDX-License-Identifier: MIT
"""Linux/Tk desktop companion. Run python3 app/workbench.py."""
import termios
import tkinter as tk
from tkinter import ttk, filedialog, messagebox
from model import Session, PHASES, REACH
from transport import Decoder, Serial, load_replay

class Workbench:
    def __init__(self,root):
        self.root=root
        self.port=None
        self.decoder=Decoder()
        self.session=Session('empty')
        self.root.title('Latch Reach — jayis1 — unbuilt prototype')
        self.root.geometry('1020x580')
        top=ttk.Frame(root);top.pack(fill='x',padx=10,pady=10)
        self.path=tk.StringVar(value='/dev/ttyACM0')
        ttk.Entry(top,textvariable=self.path,width=28).pack(side='left')
        for title,command in [('Connect',self.connect),('Disconnect',self.disconnect),
                              ('Replay LR1',self.replay),('Export CSV',self.export)]:
            ttk.Button(top,text=title,command=command).pack(side='left',padx=4)
        self.status=tk.StringVar(value='No evidence loaded')
        ttk.Label(root,textvariable=self.status,wraplength=980).pack(fill='x',padx=10)
        self.summary=tk.StringVar(value='No capture')
        ttk.Label(root,textvariable=self.summary).pack(fill='x',padx=10,pady=10)
        notebook=ttk.Notebook(root);notebook.pack(fill='both',expand=True,padx=10,pady=10)
        matrix=ttk.Frame(notebook);notebook.add(matrix,text='Return outcomes')
        self.table=ttk.Treeview(matrix,columns=('reach','open','closed','disturbed'),show='headings',height=6)
        for name,title in [('reach','Observed checkpoints'),('open','Latch open throughout'),
                           ('closed','Latch closed throughout'),('disturbed','Latch disturbed')]:
            self.table.heading(name,text=title)
            self.table.column(name,width=220)
        self.table.pack(fill='x',pady=12)
        for i,title in enumerate(REACH): self.table.insert('', 'end',iid=str(i),values=(title,0,0,0))
        ttk.Label(matrix,text='Counts describe contact indications, not holding force or certified engagement.\n'
                  'FAR only does not prove NEAR was crossed; missed checkpoints stay missing.\n'
                  'A rejected trip is excluded from every matrix cell.',justify='left').pack(anchor='w')
        setup=ttk.Frame(notebook);notebook.add(setup,text='Setup and limits')
        text=tk.Text(setup,wrap='word',height=16)
        text.insert('end','Author: jayis1\n\nUse a detached, unloaded cabinet latch and an enclosed hand-operated carriage. '
                    'J3 HOME, J4 NEAR, J5 FAR are disjoint position windows; J6 LATCH is an independent bolt indicator. '
                    'All are voltage-free contacts to ground. J1 is 3.3 V and ground from NUCLEO-F401RE; '
                    'J2 goes to A0, A1, A2, A3. Never attach powered outputs.\n\n'
                    'Hold HOME for the 100 ms baseline. Open slowly past the desired checkpoint(s), then return HOME '
                    'and hold for the 100 ms result window. A new baseline is required before the next trip. '
                    'Both raw and qualified HOME must stay asserted during the return window. '
                    'Any raw LATCH disturbance makes the outcome disturbed. Position overlap, HOME loss during '
                    'return, or the 30 second trial deadline rejects the trip. Sampling gaps halt the run.\n\n'
                    'Qualification is five acquired samples per channel. Use contact windows of at least 10 ms. '
                    'Reset the MCU to clear volatile totals. Reset suspicion blocks this app session; reconnect explicitly. '
                    'Replay is never physical evidence. CSV retains up to 2000 snapshots plus session metadata; '
                    'export is exclusive and will not overwrite a file. No network services or control commands exist.')
        text.configure(state='disabled');text.pack(fill='both',expand=True)
        self.root.protocol('WM_DELETE_WINDOW',self.close)
        self.timer=self.root.after(50,self.poll)
    def connect(self):
        if self.port is not None:
            messagebox.showerror('Already connected','Disconnect before replacing a serial connection.')
            return
        try:
            port=Serial(self.path.get())
        except (OSError,termios.error) as exc:
            messagebox.showerror('Serial error',str(exc));return
        self.port=port
        self.decoder=Decoder()
        self.session=Session('serial')
        self.render()
    def disconnect(self):
        if self.port:
            self.port.close();self.port=None
        if self.decoder.buffer:
            self.decoder.errors+=1;self.decoder.buffer=b''
        self.render()
    def replay_path(self,path):
        if self.port: raise ValueError('Disconnect before replay')
        replacement=load_replay(path)
        self.session=replacement
        self.decoder=Decoder()
        self.render()
    def replay(self):
        path=filedialog.askopenfilename(title='Open LR1 recording (max 2 MiB)')
        if path:
            try: self.replay_path(path)
            except (OSError,ValueError) as exc: messagebox.showerror('Replay rejected',str(exc))
    def export(self):
        path=filedialog.asksaveasfilename(title='New CSV file',defaultextension='.csv')
        if path:
            try: self.session.export(path,self.port is not None,self.decoder.errors)
            except (OSError,ValueError) as exc: messagebox.showerror('Export error',str(exc))
    def render(self):
        self.status.set(self.session.status(self.port is not None)+f' | parser errors: {self.decoder.errors}')
        row=self.session.latest
        if row:
            self.summary.set(f"{PHASES[row['phase']]} | Started {row['starts']} | Completed {row['completed']} | "
                             f"Rejected {row['rejected']} | age {row['age']} ms | raw {row['raw']:04b}")
        else:
            self.summary.set('No capture')
        for i,title in enumerate(REACH):
            values=[row['b%d'%(i*3+j)] if row else 0 for j in range(3)]
            self.table.item(str(i),values=(title,*values))
    def poll(self):
        if self.port:
            try:
                for row in self.decoder.feed(self.port.read()): self.session.accept(row)
            except (OSError,ValueError,termios.error) as exc:
                self.session.error=str(exc)
                self.disconnect()
        self.render()
        self.timer=self.root.after(50,self.poll)
    def close(self):
        self.root.after_cancel(self.timer)
        self.disconnect()
        self.root.destroy()

if __name__=='__main__':
    root=tk.Tk()
    Workbench(root)
    root.mainloop()
