# Author: jayis1. SPDX-License-Identifier: MIT
"""Real Tk event loop with C-encoder replay and a Linux pseudo-terminal."""
import os
from pathlib import Path
import pty
import sys
import tempfile
import time
import tkinter as tk
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'app'))
from workbench import Workbench
root=tk.Tk(); app=Workbench(root)
fixture=Path(__file__).resolve().parents[1]/'firmware/build/demo.vw1'
app.replay(fixture); root.update()
assert 'Moves 2' in app.summary.get()
assert len(app.tree.get_children())==len(app.session.rows)
for tab in range(3): app.tabs.select(tab); root.update()
with tempfile.TemporaryDirectory() as d:
    out=Path(d)/'gui.csv'; app.session.export(out,False)
    assert 'jayis1' in out.read_text()
master,slave=pty.openpty()
try:
    app.port.set(os.ttyname(slave)); app.connect(); root.update()
    assert app.summary.get()=='No snapshots'
    assert not app.tree.get_children()
    # Keep writes small so PTY capacity cannot block the test thread.
    lines=fixture.read_bytes().splitlines(keepends=True)
    for i in range(0,len(lines),10):
        os.write(master,b''.join(lines[i:i+10]))
        deadline=time.monotonic()+0.04
        while time.monotonic()<deadline: root.update(); time.sleep(0.002)
    assert app.session.rows[-1]['abandoned']==1
    assert len(app.tree.get_children())==len(app.session.rows)
    os.close(master); master=None
    deadline=time.monotonic()+0.1
    while time.monotonic()<deadline: root.update(); time.sleep(0.002)
    assert app.serial is None
    assert 'NOT LIVE' in app.status.get()
finally:
    if master is not None: os.close(master)
    os.close(slave); app.close()
print('PASS Tk screens, C replay, batched PTY capture, reset labels, private export and hangup')
