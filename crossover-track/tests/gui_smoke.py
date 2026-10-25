# Author: jayis1. SPDX-License-Identifier: MIT
"""Real Tk event loop and Linux PTY; transport is synthetic, not hardware."""
import importlib.util
import os
from pathlib import Path
import pty
import stat
import tempfile
import time
import tkinter as tk

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('crossapp', ROOT/'app/app.py')
assert spec and spec.loader
app = importlib.util.module_from_spec(spec)
spec.loader.exec_module(app)
root = tk.Tk()
w = app.Window(root)
master, slave = pty.openpty()
w.path.set(os.ttyname(slave))
w.connect()
assert w.port is not None

def pump(seconds=0.15):
    until = time.monotonic() + seconds
    while time.monotonic() < until:
        root.update()
        time.sleep(0.005)

lines = (ROOT/'firmware/build/fixture.txt').read_bytes().splitlines(True)
os.write(master, lines[0])
pump()
w.begin()
assert w.arm_after is not None and not w.model.collecting
# A pre-boundary result cannot enter the pending study.
os.write(master, b''.join(lines[1:6]))
pump()
assert not w.model.study
# A fresh subsequent ARMED frame is required after the settling interval.
pump(1.05)
os.write(master, lines[6])
pump()
assert w.model.collecting and not w.model.study
assert 'LEFT cart A' in w.guidance.get()
# Remaining data has only three trials; GUI must not invent a fourth.
os.write(master, b''.join(lines[7:]))
pump()
assert len(w.model.study) == 3
assert len(w.table.get_children()) == 4
assert 'LIVE' in w.status.get()
for tab in w.tabs.tabs():
    w.tabs.select(tab)
    pump(0.03)
# No receipt for >1s stops the incomplete live block.
pump(1.1)
assert not w.model.collecting
assert 'Stale' in w.model.warning
os.close(master)
pump()
assert w.port is None
assert w.model.provenance == 'live'
with tempfile.TemporaryDirectory(prefix='crossover-gui-') as d:
    target = Path(d)/'evidence.csv'
    w.model.export(target,False,time.monotonic())
    assert stat.S_IMODE(target.stat().st_mode) == 0o600
    text = target.read_text()
    assert 'jayis1,live,False,True' in text
    try:
        w.model.export(target,False,time.monotonic())
        raise AssertionError('overwrite accepted')
    except FileExistsError:
        pass
    # Exercise actual replay handler and the complete four-run calculation.
    app.filedialog.askopenfilename = lambda: str(ROOT/'firmware/build/fixture.txt')
    w.replay()
    pump()
    assert len(w.model.study) == 4 and w.model.analysis() == (0.0,0.0)
    assert 'NOT LIVE' in w.status.get()
    assert 'cart A minus B' in w.guidance.get()
    fifo = Path(d)/'fifo'
    os.mkfifo(fifo)
    try:
        app.read_replay(fifo)
        raise AssertionError('FIFO accepted')
    except ValueError:
        pass
os.close(slave)
w.close()
print('PASS Tk/PTY: pending boundary, 3 screens, real C frames, stale stop, hangup, replay, CSV permissions and overwrite refusal')
