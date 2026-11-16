# Author: jayis1. SPDX-License-Identifier: MIT
"""Linux Tk companion. USB serial, local replay and private CSV only."""
import tkinter as tk
from tkinter import ttk, filedialog, messagebox
import glob, time, termios
from model import Session, phrase
from transport import Serial, Decoder, load_replay

class Workbench:
    def __init__(self,root):
        self.root=root; root.title('Bin Phrase — jayis1')
        self.serial=None; self.decoder=Decoder(); self.session=Session('live')
        self.status=tk.StringVar(value='Disconnected — no physical evidence')
        ttk.Label(root,textvariable=self.status).pack(fill='x')
        tabs=ttk.Notebook(root); tabs.pack(fill='both',expand=True)
        connection=ttk.Frame(tabs); live=ttk.Frame(tabs); history=ttk.Frame(tabs)
        tabs.add(connection,text='Connection'); tabs.add(live,text='Current / retained word')
        tabs.add(history,text='Snapshots / export')
        self.port=tk.StringVar()
        self.ports=ttk.Combobox(connection,textvariable=self.port,width=60)
        self.ports.pack(); self.scan()
        ttk.Button(connection,text='Rescan serial ports',command=self.scan).pack()
        ttk.Button(connection,text='Connect',command=self.connect).pack()
        ttk.Button(connection,text='Disconnect',command=self.disconnect).pack()
        ttk.Button(connection,text='Replay BQ1 recording',command=self.replay_dialog).pack()
        ttk.Label(connection,text='Read-only telemetry; no recipe, quantity or safety certification.\n'
                  'Bits 0..2 = lid OPEN. Bit 3 = tray PRESENT.\n'
                  'Remove tray and close lids before starting.').pack(pady=20)
        self.current=tk.StringVar(value='No samples')
        self.retained=tk.StringVar(value='No completed session')
        ttk.Label(live,textvariable=self.current,wraplength=750).pack(pady=20)
        ttk.Label(live,textvariable=self.retained,wraplength=750).pack(pady=20)
        self.table=ttk.Treeview(history,columns=('time','phase','completed','word'),show='headings')
        for key in ('time','phase','completed','word'): self.table.heading(key,text=key)
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
        self.serial=port; self.decoder=Decoder(); self.session=Session('live')
        self.render(); self.status.set('Connected; waiting for advancing acquisition')
    def disconnect(self):
        if self.serial: self.serial.close(); self.serial=None
        if self.decoder.buffer: self.decoder.errors+=1; self.decoder.buffer=b''
        self.session.errors=self.decoder.errors
        self.status.set('Disconnected — retained evidence is not live')
    def replay(self,path):
        if self.serial: raise ValueError('Disconnect before replacing session with replay')
        replacement=load_replay(path)
        self.session=replacement; self.decoder=Decoder(); self.render()
        self.status.set('Replay — synthetic fixtures are not hardware measurements')
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
            self.current.set('No samples'); self.retained.set('No completed session'); return
        row=self.session.rows[-1]
        self.current.set(f"phase={row['phase']} raw={row['raw']:04b} qualified={row['qualified']:04b}\n"
                         f"Current/last attempted word: {phrase(row)}\n"
                         f"starts={row['starts']} completed={row['completed']} aborted={row['aborted']} reason={row['reason']}")
        self.retained.set('Last completed word: '+phrase(row,True)+f"; age={row['last_age']} ms"
                          if row['completed'] else 'No completed session')
        for row in self.session.rows:
            self.table.insert('', 'end',values=(row['now'],row['phase'],row['completed'],phrase(row)))
    def poll(self):
        try:
            if self.serial:
                data=self.serial.read()
                rows=self.decoder.feed(data)
                self.session.errors=self.decoder.errors
                for row in rows: self.session.accept(row)
                if rows: self.render()
                if self.session.fresh(True): self.status.set('Advancing acquisition — observed contacts only')
                else: self.status.set('Waiting / stale / halted — not current acquisition')
        except (ValueError,OSError,termios.error) as exc:
            self.disconnect(); self.status.set('Stopped: '+str(exc))
        self.job=self.root.after(50,self.poll)
    def close(self):
        self.root.after_cancel(self.job); self.disconnect(); self.root.destroy()

if __name__=='__main__':
    root=tk.Tk(); root.geometry('900x600'); Workbench(root); root.mainloop()
