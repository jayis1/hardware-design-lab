# Author: jayis1. SPDX-License-Identifier: MIT
"""Local Linux/Tk workbench; no network or device control channel."""
import glob
import termios
import tkinter as tk
from tkinter import ttk, filedialog, messagebox
from model import Session, describe, shared
from transport import Serial, Decoder, load_replay

class Workbench:
    def __init__(self,root):
        self.root=root
        root.title('Aisle Pair — jayis1')
        self.serial=None
        self.decoder=Decoder()
        self.session=Session('live')
        self.status=tk.StringVar(value='Disconnected — no physical evidence')
        ttk.Label(root,textvariable=self.status).pack(fill='x')
        tabs=ttk.Notebook(root)
        tabs.pack(fill='both',expand=True)
        setup=ttk.Frame(tabs); matrix=ttk.Frame(tabs); history=ttk.Frame(tabs)
        tabs.add(setup,text='Connection / setup')
        tabs.add(matrix,text='Aisle occupancy matrix')
        tabs.add(history,text='Snapshots / export')
        self.port=tk.StringVar()
        self.ports=ttk.Combobox(setup,textvariable=self.port,width=60)
        self.ports.pack(); self.scan()
        ttk.Button(setup,text='Rescan serial ports',command=self.scan).pack()
        ttk.Button(setup,text='Connect',command=self.connect).pack()
        ttk.Button(setup,text='Disconnect',command=self.disconnect).pack()
        ttk.Button(setup,text='Replay AY1 recording',command=self.replay_dialog).pack()
        ttk.Label(setup,text='Read-only observation, never an anti-tip interlock.\n'
                  'A = bits 0/1; B = bits 2/3. Codes: 00 / 01 / 11.\n'
                  'Use small captive tabletop drawers, not occupied aisles.\n'
                  'Milliseconds describe clean sampled intervals, not measured clearance.').pack(pady=20)
        self.current=tk.StringVar(value='No samples')
        self.summary=tk.StringVar(value='No observations')
        ttk.Label(matrix,textvariable=self.current).pack(pady=10)
        grid=ttk.Frame(matrix); grid.pack()
        self.cells=[]
        ttk.Label(grid,text='A rows / B columns').grid(row=0,column=0,padx=8,pady=8)
        for b in range(3):
            ttk.Label(grid,text=f'B stage {b}').grid(row=0,column=b+1,padx=8)
        for a in range(3):
            ttk.Label(grid,text=f'A stage {a}').grid(row=a+1,column=0,padx=8)
            for b in range(3):
                value=tk.StringVar(value='0'); self.cells.append(value)
                ttk.Label(grid,textvariable=value,width=12,anchor='center',relief='groove').grid(
                    row=a+1,column=b+1,padx=5,pady=5)
        ttk.Label(matrix,text='All cells: observed milliseconds; shared = rows 1/2 × columns 1/2').pack()
        ttk.Label(matrix,textvariable=self.summary,wraplength=800).pack(pady=20)
        columns=('time','position','shared','unknown','maximum')
        self.table=ttk.Treeview(history,columns=columns,show='headings')
        for key in columns:
            self.table.heading(key,text=key)
            self.table.column(key,width=150)
        self.table.pack(fill='both',expand=True)
        ttk.Button(history,text='Export private CSV (new file)',command=self.export_dialog).pack()
        root.protocol('WM_DELETE_WINDOW',self.close)
        self.job=root.after(50,self.poll)
    def scan(self):
        self.ports['values']=sorted(glob.glob('/dev/ttyACM*')+glob.glob('/dev/ttyUSB*'))
    def connect(self):
        if self.serial:
            self.status.set('Already connected; disconnect first'); return
        try:
            port=Serial(self.port.get())
        except (OSError,termios.error) as exc:
            self.status.set(str(exc)); return
        self.serial=port
        self.decoder=Decoder()
        self.session=Session('live')
        self.render()
        self.status.set('Connected; waiting for advancing acquisition')
    def disconnect(self):
        if self.serial:
            self.serial.close(); self.serial=None
        if self.decoder.buffer:
            self.decoder.errors+=1; self.decoder.buffer=b''
        self.session.errors=self.decoder.errors
        self.status.set('Disconnected — retained evidence is not live')
    def replay(self,path):
        if self.serial: raise ValueError('Disconnect before replay')
        replacement=load_replay(path)
        self.session=replacement
        self.decoder=Decoder()
        self.render()
        self.status.set('Replay — fixtures are synthetic, not physical measurements')
    def replay_dialog(self):
        path=filedialog.askopenfilename()
        if path:
            try: self.replay(path)
            except (ValueError,OSError) as exc: messagebox.showerror('Replay rejected',str(exc))
    def export_dialog(self):
        path=filedialog.asksaveasfilename(defaultextension='.csv')
        if path:
            try: self.session.export(path,self.serial is not None)
            except (ValueError,OSError) as exc: messagebox.showerror('Export failed',str(exc))
    def render(self):
        self.table.delete(*self.table.get_children())
        if not self.session.rows:
            self.current.set('No samples'); self.summary.set('No observations')
            for value in self.cells: value.set('0')
            return
        r=self.session.rows[-1]
        phase={0:'WAIT',1:'OBSERVE',3:'HALT'}[r['phase']]
        self.current.set(f"{phase}: {describe(r)}; raw={r['raw']:04b}; fault={r['fault']}")
        for i,value in enumerate(self.cells): value.set(str(r[f'bin{i}']))
        self.summary.set(f"Shared={shared(r)} ms; unknown={r['unknown']} / {r['intervals']} intervals\n"
                         f"Observed shared segments={r['episodes']}; current={r['run']} ms; longest={r['maximum']} ms\n"
                         f"Forbidden-code samples={r['invalid']}; gaps={r['gaps']}; TX errors={r['tx_errors']}")
        for row in self.session.rows:
            self.table.insert('', 'end',values=(row['now'],describe(row),shared(row),row['unknown'],row['maximum']))
    def poll(self):
        try:
            if self.serial:
                rows=self.decoder.feed(self.serial.read())
                self.session.errors=self.decoder.errors
                for row in rows: self.session.accept(row)
                if rows: self.render()
                self.status.set('Advancing acquisition — contact evidence only' if self.session.fresh(True)
                                else 'Waiting / stale / halted — not current acquisition')
        except (ValueError,OSError,termios.error) as exc:
            self.disconnect(); self.status.set('Stopped: '+str(exc))
        self.job=self.root.after(50,self.poll)
    def close(self):
        self.root.after_cancel(self.job)
        self.disconnect()
        self.root.destroy()

if __name__=='__main__':
    root=tk.Tk(); root.geometry('950x650'); Workbench(root); root.mainloop()
