# Author: jayis1. SPDX-License-Identifier: MIT
"""Linux Tk companion: no commands, cloud service or background telemetry."""
import argparse
import os
import termios
import time
import tkinter as tk
from tkinter import ttk, filedialog, messagebox
from model import Session, FIELDS
from transport import Decoder, Serial, load_replay

class Workbench:
    def __init__(self, root):
        self.root = root
        root.title('Ratio Window — jayis1 — unvalidated physical prototype')
        self.serial = None
        self.session = Session('live')
        self.decoder = Decoder()
        self.path = tk.StringVar(value='/dev/ttyACM0')
        self.status = tk.StringVar(value='DISCONNECTED')
        self.summary = tk.StringVar(value='No evidence')
        controls = ttk.Frame(root); controls.pack(fill='x', padx=10, pady=10)
        ttk.Entry(controls, textvariable=self.path, width=32).pack(side='left')
        for text, fn in [('Connect',self.connect_dialog),('Disconnect',self.disconnect),
                         ('Replay',self.replay_dialog),('Export',self.export_dialog)]:
            ttk.Button(controls,text=text,command=fn).pack(side='left')
        ttk.Label(root,textvariable=self.status).pack(fill='x',padx=10)
        self.tabs = ttk.Notebook(root); self.tabs.pack(fill='both',expand=True)
        comparison = ttk.Frame(self.tabs); notebook = ttk.Frame(self.tabs)
        setup = ttk.Frame(self.tabs)
        self.tabs.add(comparison,text='Turn windows')
        self.tabs.add(notebook,text='Snapshot notebook')
        self.tabs.add(setup,text='Wiring and limits')
        ttk.Label(comparison,textvariable=self.summary,justify='left',font=('TkDefaultFont',13)).pack(padx=20,pady=20,anchor='w')
        self.chart = tk.Canvas(comparison,height=180,background='white')
        self.chart.pack(fill='both',expand=True,padx=10,pady=10)
        columns = ('samples','state','good','bad','steps','output','last','last_ms','reason')
        self.table = ttk.Treeview(notebook,columns=columns,show='headings',height=15)
        for name in columns:
            self.table.heading(name,text=name); self.table.column(name,width=85)
        scrollbar = ttk.Scrollbar(notebook,orient='vertical',command=self.table.yview)
        self.table.configure(yscrollcommand=scrollbar.set)
        scrollbar.pack(side='right',fill='y'); self.table.pack(fill='both',expand=True)
        text = tk.Text(setup,wrap='word',height=20)
        text.pack(fill='both',expand=True)
        text.insert('1.0',
            'Author: jayis1\n\n'
            'Two Bourns PEC11R-1015F-N0024 contact encoders, 24 pulses / 96 edges per turn.\n'
            'J3 INPUT_A / PA0, J4 INPUT_B / PA1, J5 OUTPUT_A / PA4, J6 OUTPUT_B / PB0.\n'
            'Each sensor pin 2 is GND. Encoder commons go to GND. J1 pin 1 is Nucleo 3V3 only.\n\n'
            'With both shafts still for 100 acquired milliseconds, a window opens. '
            'Turn the input slowly in one direction. Each 96 input transitions closes a window. '
            'Output transitions qualifying on that cut belong to the ending window. '
            'A reversal on either shaft or a two-bit jump rejects the current window. '
            'Stop both shafts to rearm. A 120-second window deadline rejects at equality.\n\n'
            'The latest ratio is an electrical output-edge count / 96, not a tooth-count proof. '
            'No index identifies a physical shaft angle. Sensor drag, slip, aliasing and bounce '
            'require bench testing. No powered mechanism, safety use, energized switches or '
            'unknown sensor electronics. Use only a guarded, low-energy hand-cranked fixture.\n\n'
            'Replay is not live. The notebook retains at most 512 snapshots, not every window. '
            'Export before reconnect: replacement sessions clear retained evidence. '
            'The graph shows latest completed count by received snapshot, not unique turns. '
            'Reset the MCU and reconnect when changing gearing. Firmware never accepts commands.\n')
        text.configure(state='disabled')
        root.protocol('WM_DELETE_WINDOW',self.close)
        self.job = root.after(25,self.poll)

    def connect(self, path):
        if self.serial is not None:
            raise ValueError('Disconnect before opening another port')
        port = Serial(path)  # Preserve old evidence if construction fails.
        self.serial = port
        self.session = Session('live'); self.session.connected = True
        self.decoder = Decoder(); self.refresh()

    def connect_dialog(self):
        try: self.connect(self.path.get())
        except (OSError,termios.error,ValueError) as exc:
            messagebox.showerror('Connection failed',str(exc))

    def disconnect(self):
        if self.serial is not None:
            if self.decoder.buffer:
                self.decoder.errors += 1
                self.decoder.buffer = b''
            self.serial.close(); self.serial = None
            self.session.errors = self.decoder.errors
        self.session.connected = False
        self.refresh()

    def replay(self, path):
        if self.serial is not None:
            raise ValueError('Disconnect before replay')
        replacement = load_replay(path)
        self.session = replacement
        self.decoder = Decoder()
        self.refresh()

    def replay_dialog(self):
        path = filedialog.askopenfilename(title='Select RW1 recording (not live)')
        if path:
            try: self.replay(path)
            except (OSError,ValueError) as exc: messagebox.showerror('Replay failed',str(exc))

    def export_dialog(self):
        path = filedialog.asksaveasfilename(defaultextension='.csv',confirmoverwrite=False)
        if path:
            try: self.session.export(path,time.monotonic())
            except (OSError,ValueError) as exc: messagebox.showerror('Export failed',str(exc))

    def poll(self):
        if self.serial is not None and not self.session.blocked:
            try:
                rows = self.decoder.feed(self.serial.read())
                self.session.errors = self.decoder.errors
                for row in rows:
                    self.session.accept(row,time.monotonic())
            except (OSError,ValueError) as exc:
                self.session.warning = str(exc)
                self.disconnect()
        self.refresh()
        self.job = self.root.after(25,self.poll)

    def refresh(self):
        s = self.session
        self.status.set(s.status(time.monotonic())+' | '+s.provenance+
                        ' | bad frames: '+str(s.errors)+' | '+s.warning)
        self.table.delete(*self.table.get_children())
        for row in s.rows:
            self.table.insert('', 'end',values=[row[k] for k in self.table['columns']])
        self.chart.delete('all')
        if not s.rows:
            self.summary.set('No evidence')
            return
        r = s.rows[-1]
        result = ('No completed window' if not r['good'] else
                  f"Latest output/input magnitude: {r['last']}/96 = {r['last']/96:.6f}\n"
                  f"Input direction {r['last_input']} / output direction {r['last_output']}\n"
                  f"Completed duration: {r['last_ms']} acquired ms; output range {r['minimum']}…{r['maximum']}")
        self.summary.set(f"State: {['WAIT','ACTIVE','HALT'][r['state']]} | fault {r['fault']}\n"
                         f"Accepted {r['good']} / rejected {r['bad']} | latest rejection {r['reason']}\n"
                         f"Same {r['same']} / opposite {r['opposite']} / still {r['still']}\n"
                         f"Pending: input {r['steps']}/96; output {r['output']}; age {r['age']} ms\n\n"+result+
                         '\n\nElectrical observations only; no measured gear accuracy claim.')
        rows = list(s.rows)[-100:]
        maximum = max(1,max(x['last'] for x in rows))
        width = max(100,self.chart.winfo_width()-20)
        for i,row in enumerate(rows):
            x = 10+i*width/max(1,len(rows))
            y = 160-140*row['last']/maximum
            self.chart.create_line(x,160,x,y,fill='#286080',width=3)

    def close(self):
        self.root.after_cancel(self.job)
        self.disconnect()
        self.root.destroy()

def main():
    parser = argparse.ArgumentParser(description='Ratio Window by jayis1')
    parser.add_argument('--replay')
    args = parser.parse_args()
    root = tk.Tk(); app = Workbench(root)
    if args.replay:
        try: app.replay(args.replay)
        except (OSError,ValueError) as exc: messagebox.showerror('Replay failed',str(exc))
    root.mainloop()

if __name__ == '__main__': main()
