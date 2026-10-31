#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Linux/Tk Rail Skew workbench. No demo data is presented as live."""
import glob, termios, tkinter as tk
from tkinter import ttk, filedialog
from model import Session, STATES, BINS
from transport import Decoder, Serial, load_replay

class Workbench:
    def __init__(self,root):
        self.root=root
        root.title('Rail Skew — jayis1')
        root.geometry('1000x650')
        self.serial=None
        self.decoder=Decoder()
        self.session=Session()
        self.path=tk.StringVar()
        self.status=tk.StringVar(value='Disconnected; no evidence')
        self.values=tk.StringVar(value='No observations')
        self.tabs=ttk.Notebook(root); self.tabs.pack(fill='both',expand=True)
        connect=ttk.Frame(self.tabs); live=ttk.Frame(self.tabs); evidence=ttk.Frame(self.tabs)
        for widget,name in [(connect,'Connect / Setup'),(live,'Rail progression'),(evidence,'History / Export')]:
            self.tabs.add(widget,text=name)
        ttk.Label(connect,text='115200 baud, RS1; disconnected voltage-free contacts only. Author: jayis1').pack(pady=15)
        self.ports=ttk.Combobox(connect,textvariable=self.path,width=65); self.ports.pack()
        for label,command in [('Refresh ports',self.refresh),('Connect',self.connect),
                              ('Disconnect',self.disconnect),('Open replay',self.choose_replay)]:
            ttk.Button(connect,text=label,command=command).pack(pady=6)
        ttk.Label(connect,text='1. Check each rail code: 00 closed, 01 middle, 11 open; never 10.\n'
                  '2. Start at both closed or both open; verify READY.\n'
                  '3. Move by hand, export before reset or reconnect.\n'
                  'Fixed thresholds are mechanical. No software commands or safety outputs.').pack(pady=15)
        ttk.Label(live,textvariable=self.values,justify='left',font=('TkFixedFont',12)).pack(anchor='w',padx=15,pady=15)
        ttk.Label(live,text='Bins: matched / one-stage apart / two-stages apart, in milliseconds.\n'
                  'Direction means origin of traverse, not instantaneous motion.\n'
                  'Position thresholds are proxies: do not infer force, mm of skew, or safe operation.').pack(anchor='w',padx=15)
        self.history=ttk.Treeview(evidence,columns=('time','state','starts','opening','closing','aborted'),show='headings',height=16)
        for name in self.history['columns']:
            self.history.heading(name,text=name); self.history.column(name,width=135)
        self.history.pack(fill='both',expand=True)
        ttk.Button(evidence,text='Export retained snapshots (new CSV)',command=self.choose_export).pack(pady=8)
        ttk.Label(root,textvariable=self.status,wraplength=950).pack(fill='x')
        self.refresh()
        root.protocol('WM_DELETE_WINDOW',self.close)
        self.timer=root.after(20,self.poll)
    def refresh(self):
        ports=sorted(glob.glob('/dev/ttyACM*')+glob.glob('/dev/ttyUSB*'))
        self.ports['values']=ports
        if ports and not self.path.get(): self.path.set(ports[0])
    def connect(self):
        if self.serial is not None:
            self.status.set('Already connected; disconnect before replacing session')
            return
        try:
            port=Serial(self.path.get())
        except (OSError,ValueError,termios.error) as exc:
            self.status.set('Open failed: '+str(exc)); return
        self.serial=port
        self.decoder=Decoder()
        self.session=Session('live'); self.session.connected=True
        self.render()
    def disconnect(self):
        if self.serial is not None:
            self.serial.close(); self.serial=None
            if self.decoder.buffer:
                self.decoder.errors+=1; self.decoder.buffer=b''
            self.session.errors=self.decoder.errors
        self.session.connected=False
        self.render()
    def replay_from(self,path):
        # Replacement is fully validated before releasing existing evidence.
        replacement=load_replay(path)
        self.disconnect()
        self.session=replacement
        self.decoder=Decoder()
        self.render()
    def choose_replay(self):
        path=filedialog.askopenfilename()
        if not path: return
        try: self.replay_from(path)
        except (OSError,ValueError) as exc: self.status.set('Replay rejected: '+str(exc))
    def export_to(self,path):
        self.session.export(path)
    def choose_export(self):
        path=filedialog.asksaveasfilename(defaultextension='.csv')
        if not path: return
        try:
            self.export_to(path); self.status.set('Exported '+path)
        except (OSError,ValueError) as exc: self.status.set('Export failed: '+str(exc))
    def render(self):
        s=self.session
        label='LIVE' if s.fresh() else 'NOT LIVE'
        self.status.set(f'{s.source} / {label} / connected={s.connected} / parser errors={s.errors} / {s.warning}')
        self.history.delete(*self.history.get_children())
        for row in list(s.rows)[-200:]:
            self.history.insert('', 'end', values=(row['time'],STATES[row['state']],row['starts'],
                    row['opening'],row['closing'],row['aborted']))
        if not s.rows:
            self.values.set('No observations'); return
        r=s.rows[-1]
        self.values.set(f"{STATES[r['state']]} — raw={r['raw']:04b} qualified={r['levels']:04b} known={r['known']:04b}\n"
                       f"starts={r['starts']} opening={r['opening']} closing={r['closing']} abandoned={r['aborted']}\n"
                       f"active={r['active_ms']} ms peak={r['peak']} stages\n"
                       f"latest completed={r['last_ms']} ms direction={r['last_dir']} peak={r['last_peak']}\n"
                       f"Opening bins (ms): {[r[k] for k in BINS[:3]]}\n"
                       f"Closing bins (ms): {[r[k] for k in BINS[3:]]}\n"
                       f"flags={r['flags']} gaps={r['gaps']} UART failures={r['tx']}")
    def poll(self):
        if self.serial is not None:
            try:
                data=self.serial.read()
                rows=self.decoder.feed(data)
                self.session.errors=self.decoder.errors
                for row in rows: self.session.accept(row)
            except (OSError,ValueError) as exc:
                self.session.warning=str(exc)
                self.disconnect()
        self.render()
        self.timer=self.root.after(20,self.poll)
    def close(self):
        self.root.after_cancel(self.timer)
        self.disconnect()
        self.root.destroy()

if __name__=='__main__':
    root=tk.Tk(); Workbench(root); root.mainloop()
