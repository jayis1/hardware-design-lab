#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Local, receive-only Takeup Atlas application. Linux + standard-library Tk."""
import binascii
from collections import deque
import csv
import glob
import math
import os
import re
import select
import stat
import termios
import time

FIELDS = ('sequence','time','input','output','invalid_input','invalid_output','gaps',
          'reversals','events','rejected','first','confirm','duration','direction',
          'state','travel','flags','transport_errors')
RESULT = ('first','confirm','duration','direction')
COUNTERS = ('invalid_input','invalid_output','gaps','reversals','events','rejected','transport_errors')
STATES = ('Priming: move both shafts in one direction','Following: reverse the input',
          'Taking up: continue slowly','FAULT: inspect wiring, then reset')

def parse(line):
    if len(line) > 256:
        raise ValueError('Oversized frame')
    try:
        body, check = line.strip().rsplit(b'*',1)
        if not re.fullmatch(b'[0-9A-F]{4}',check):
            raise ValueError('Malformed checksum')
        if binascii.crc_hqx(body,65535) != int(check,16):
            raise ValueError('CRC mismatch')
        words=body.decode('ascii').split(',')
        if len(words)!=len(FIELDS)+1 or words[0]!='TKA1':
            raise ValueError('Incompatible protocol')
        if any(not re.fullmatch(r'0|-?[1-9][0-9]*',w) for w in words[1:]):
            raise ValueError('Invalid integer')
        row=dict(zip(FIELDS,map(int,words[1:])))
    except UnicodeError as error:
        raise ValueError('Non-ASCII data') from error
    for key,value in row.items():
        low,high=(-2147483648,2147483647) if key in ('input','output') else (0,4294967295)
        if key=='direction': low,high=-1,1
        if not low<=value<=high:
            raise ValueError('Out of range: '+key)
    if row['state']>3 or row['flags']>15:
        raise ValueError('Unknown state or flags')
    quality=bool(row['gaps']) | (bool(row['invalid_input'])<<1) | (bool(row['invalid_output'])<<2)
    if row['flags']&7 != quality or bool(row['flags']) != (row['state']==3):
        raise ValueError('Inconsistent quality state')
    pending=row['state']==2
    if bool(row['travel']) != pending:
        raise ValueError('Inconsistent active travel')
    if not row['flags']&8 and row['reversals'] != row['events']+row['rejected']+pending:
        raise ValueError('Trial conservation failure')
    if row['events']:
        if not (1<=row['first']<=row['confirm']<=row['duration']<10000) or not row['direction']:
            raise ValueError('Impossible completed trial')
    elif any(row[k] for k in RESULT):
        raise ValueError('Result without event')
    return row

class Stream:
    def __init__(self):
        self.pending=b''
        self.errors=0

    def feed(self,chunk):
        self.pending+=chunk
        rows=[]
        while b'\n' in self.pending:
            line,self.pending=self.pending.split(b'\n',1)
            offset=line.rfind(b'TKA1,')
            try:
                if offset<0: raise ValueError('No marker')
                rows.append(parse(line[offset:]))
            except ValueError:
                self.errors+=1
        if len(self.pending)>256:
            self.pending=self.pending[-256:]
            self.errors+=1
        return rows

class Model:
    def __init__(self):
        self.rows=deque(maxlen=6000)
        self.last_received=None
        self.missing=0

    def accept(self,row,received):
        if self.rows:
            prev=self.rows[-1]
            ds=(row['sequence']-prev['sequence'])&0xffffffff
            dt=(row['time']-prev['time'])&0xffffffff
            if not 0<ds<0x80000000 or not 0<dt<0x80000000:
                raise ValueError('Reset, duplicate or reorder: reconnect explicitly')
            if any(row[k]<prev[k] for k in COUNTERS) or row['flags']&prev['flags']!=prev['flags']:
                raise ValueError('Sticky evidence disappeared: reconnect explicitly')
            if row['events']==prev['events'] and any(row[k]!=prev[k] for k in RESULT):
                raise ValueError('Completed result changed without a new event')
            if row['events']-prev['events']>dt:
                raise ValueError('Impossible event rate')
            self.missing+=ds-1
        self.rows.append(dict(row,received=received))
        self.last_received=received

    def stale(self,now):
        return self.last_received is None or now-self.last_received>1

    def export(self,path,source,connected,edges,warning):
        fd=os.open(path,os.O_WRONLY|os.O_CREAT|os.O_EXCL,0o600)
        with os.fdopen(fd,'w',newline='') as file:
            keys=('author','source','connected','age_seconds','edges_per_turn','warning','first_degrees','confirm_degrees')+FIELDS
            writer=csv.DictWriter(file,fieldnames=list(keys))
            writer.writeheader()
            now=time.monotonic()
            for row in self.rows:
                record={k:row[k] for k in FIELDS}
                record.update(author='jayis1',source=source,connected=connected,
                              age_seconds=max(0,now-row['received']),edges_per_turn=edges,warning=warning,
                              first_degrees=row['first']*360/edges,confirm_degrees=row['confirm']*360/edges)
                writer.writerow(record)

def read_replay(path):
    fd=os.open(path,os.O_RDONLY|os.O_NONBLOCK)
    with os.fdopen(fd,'rb') as file:
        if not stat.S_ISREG(os.fstat(file.fileno()).st_mode):
            raise ValueError('Replay must be a regular file')
        data=file.read(2_000_001)
        if len(data)>2_000_000: raise ValueError('Replay exceeds 2 MB')
        return data

class App:
    def __init__(self,root):
        import tkinter as tk
        from tkinter import ttk
        self.root=root
        self.fd=None
        self.attrs=None
        self.model=Model()
        self.stream=Stream()
        self.source='NO DATA'
        self.error=''
        self.edges=400
        root.title('Takeup Atlas — jayis1')
        root.geometry('1000x650')
        self.status=tk.StringVar(value='Disconnected')
        ttk.Label(root,textvariable=self.status,wraplength=980).pack(fill='x')
        self.tabs=ttk.Notebook(root);self.tabs.pack(fill='both',expand=True)
        setup,live,archive=[ttk.Frame(self.tabs) for _ in range(3)]
        for frame,label in zip((setup,live,archive),('Connect / Replay','Reversal trials','Calibration / Export')):
            self.tabs.add(frame,text=label)
        self.port=tk.StringVar()
        self.ports=ttk.Combobox(setup,textvariable=self.port,width=65)
        self.ports.pack()
        for label,callback in [('Refresh',self.refresh),('Connect: clears history',self.connect),('Disconnect',self.disconnect),('Replay: NOT LIVE',self.replay_dialog)]:
            ttk.Button(setup,text=label,command=callback).pack()
        ttk.Label(setup,text='115200 8N1. Receive only. Export before reconnecting.\nPower encoders first; wait 100 ms; then reset the observer.').pack()
        self.summary=tk.StringVar(value='No samples')
        ttk.Label(live,textvariable=self.summary,justify='left').pack(fill='x')
        self.table=ttk.Treeview(live,columns=('id','direction','first','confirm','duration'),show='headings')
        for key,label in zip(self.table['columns'],('Event','Direction','First input edges','Confirmed input edges','Duration ms')):
            self.table.heading(key,text=label)
            self.table.column(key,width=170)
        self.table.pack(fill='both',expand=True)
        ttk.Label(live,text='Latest 100 observed results. Event-ID gaps mean results were missed, not zero take-up.').pack()
        self.scale=tk.StringVar(value='400')
        ttk.Label(archive,text='Input encoder quadrature edges per shaft turn (4 × pulses/revolution).').pack()
        ttk.Entry(archive,textvariable=self.scale).pack()
        ttk.Button(archive,text='Apply angular scale',command=self.calibrate).pack()
        ttk.Button(archive,text='Export new private CSV',command=self.export_dialog).pack()
        ttk.Label(archive,text='First-following travel is a quantized observation, NOT certified backlash.\nConfirmation requires three output edges; compare like fixtures and loads.\nExports include raw telemetry, provenance, current warnings and angular scale.').pack()
        self.refresh()
        root.protocol('WM_DELETE_WINDOW',self.close)
        self.timer=root.after(50,self.poll)

    def refresh(self):
        self.ports['values']=sorted(glob.glob('/dev/ttyACM*')+glob.glob('/dev/ttyUSB*'))

    def disconnect(self):
        if self.fd is not None:
            try:
                if self.attrs is not None:
                    termios.tcsetattr(self.fd,termios.TCSANOW,self.attrs)
            except (OSError,termios.error):
                pass
            os.close(self.fd)
            self.fd=None
        self.draw()

    def connect(self):
        self.disconnect()
        fd=None;original=None
        try:
            fd=os.open(self.port.get(),os.O_RDONLY|os.O_NOCTTY|os.O_NONBLOCK)
            if not stat.S_ISCHR(os.fstat(fd).st_mode): raise ValueError('Not a serial character device')
            original=termios.tcgetattr(fd)
            attrs=termios.tcgetattr(fd)
            attrs[0]=attrs[1]=attrs[3]=0
            attrs[2]=termios.CS8|termios.CREAD|termios.CLOCAL
            attrs[4]=attrs[5]=termios.B115200
            attrs[6][termios.VMIN]=1;attrs[6][termios.VTIME]=0
            termios.tcsetattr(fd,termios.TCSANOW,attrs)
            termios.tcflush(fd,termios.TCIFLUSH)
            self.fd,self.attrs=fd,original
            self.model,self.stream=Model(),Stream()
            self.source='LIVE USB';self.error=''
        except (OSError,ValueError,termios.error) as error:
            if fd is not None:
                if original is not None:
                    try: termios.tcsetattr(fd,termios.TCSANOW,original)
                    except (OSError,termios.error): pass
                os.close(fd)
            self.error=str(error)
        self.draw()

    def load_replay(self,path):
        stream,model=Stream(),Model()
        for row in stream.feed(read_replay(path)):
            model.accept(row,time.monotonic())
        if not model.rows: raise ValueError('No valid frames')
        if stream.pending: stream.errors+=1
        self.disconnect()
        self.stream,self.model=stream,model
        self.source='REPLAY — NOT LIVE';self.error=''
        self.draw()

    def replay_dialog(self):
        from tkinter import filedialog
        path=filedialog.askopenfilename()
        if path:
            try: self.load_replay(path)
            except (OSError,ValueError) as error: self.error=str(error)
        self.draw()

    def calibrate(self):
        try:
            value=float(self.scale.get())
            if not math.isfinite(value) or not 4<=value<=1000000:
                raise ValueError('Scale must be finite: 4..1000000 edges/turn')
            self.edges=value;self.error=''
        except ValueError as error: self.error=str(error)
        self.draw()

    def warning(self):
        return f'bad_frames={self.stream.errors};missing_frames={self.model.missing};{self.error}'

    def export_dialog(self):
        from tkinter import filedialog
        path=filedialog.asksaveasfilename(defaultextension='.csv')
        if path:
            try: self.model.export(path,self.source,self.fd is not None,self.edges,self.warning())
            except OSError as error: self.error=str(error)
        self.draw()

    def poll(self):
        if self.fd is not None:
            try:
                poller=select.poll();poller.register(self.fd,select.POLLIN|select.POLLHUP|select.POLLERR)
                events=poller.poll(0)
                mask=events[0][1] if events else 0
                if mask & select.POLLIN:
                    data=os.read(self.fd,4096)
                    if not data: raise OSError('Serial disconnected')
                    for row in self.stream.feed(data): self.model.accept(row,time.monotonic())
                if mask & (select.POLLHUP|select.POLLERR|select.POLLNVAL):
                    raise OSError('Serial disconnected')
            except BlockingIOError: pass
            except (OSError,ValueError) as error:
                self.error=str(error);self.disconnect()
        self.draw()
        self.timer=self.root.after(50,self.poll)

    def draw(self):
        state='STALE' if self.model.stale(time.monotonic()) else 'receiving'
        if self.fd is None: state='not connected'
        self.status.set(f'{self.source} | {state} | {self.warning()}')
        self.table.delete(*self.table.get_children())
        if not self.model.rows:
            self.summary.set('No samples');return
        row=self.model.rows[-1]
        self.summary.set(f"{STATES[row['state']]}\nCompleted {row['events']}; rejected {row['rejected']}; flags {row['flags']}\n"
                         f"Last first-following {row['first']} edges ({row['first']*360/self.edges:.3f} degrees); "
                         f"confirmed {row['confirm']} edges; pending travel {row['travel']}\n"
                         'Historical results remain visible after faults; do not interpret them as current validity.')
        results={r['events']:r for r in self.model.rows if r['events']}
        for event,r in list(results.items())[-100:]:
            self.table.insert('', 'end',values=(event,r['direction'],r['first'],r['confirm'],r['duration']))

    def close(self):
        self.root.after_cancel(self.timer)
        self.disconnect();self.root.destroy()

if __name__=='__main__':
    import tkinter as tk
    window=tk.Tk();App(window);window.mainloop()
