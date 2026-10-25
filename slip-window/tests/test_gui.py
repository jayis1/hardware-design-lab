# Author: jayis1. SPDX-License-Identifier: MIT
"""Real Tk event loop plus Linux PTY; not physical USB verification."""
import importlib.util
import os
from pathlib import Path
import pty
import time
import tkinter as tk
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('slip_app',ROOT/'app/app.py')
assert spec and spec.loader
app=importlib.util.module_from_spec(spec)
spec.loader.exec_module(app)
root=tk.Tk()
ui=app.App(root)
master,slave=pty.openpty()
ui.path.set(os.ttyname(slave))
ui.connect()
assert ui.connection=='connected'
os.write(master,(ROOT/'firmware/build/fixture.txt').read_bytes())
def pump(seconds):
    deadline=time.monotonic()+seconds
    while time.monotonic()<deadline:
        root.update()
        time.sleep(0.01)
pump(0.4)
assert ui.model.latest['completed']==1
assert len(ui.tree.get_children())==len(ui.model.rows)
assert 'Complete 1' in ui.summary.get()
assert len(ui.tabs.tabs())==3
for tab in ui.tabs.tabs():
    ui.tabs.select(tab)
    root.update()
pump(1.6)
assert 'STALE' in ui.status.get()
ui.disconnect()
assert ui.model.source.startswith('live serial')
ui.connect()
assert 'No valid' in ui.summary.get()
assert not ui.tree.get_children()
os.close(master)
pump(0.2)
assert ui.connection.startswith('disconnected')
ui.load_replay(ROOT/'firmware/build/fixture.txt')
assert ui.model.latest['completed']==1
assert 'not live' in ui.status.get()
ui.close()
os.close(slave)
print('PASS: Tk screens, PTY capture, batch rows, stale, reconnect, unplug, replay')
