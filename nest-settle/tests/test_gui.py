# Author: jayis1. SPDX-License-Identifier: MIT
"""Actual Tk event loop and Linux PTY; no physical USB device claimed."""
import os
from pathlib import Path
import pty
import sys
import tempfile
import time
import tkinter as tk
from unittest.mock import patch
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'app'))
import nest
root = tk.Tk()
app = nest.App(root)
master, slave = pty.openpty()
app.port.set(os.ttyname(slave))
app.connect()
assert app.serial is not None
fixture = (Path(__file__).resolve().parents[1] / 'firmware/build/fixture.txt').read_bytes()
os.write(master, fixture)
def pump(seconds):
    deadline = time.monotonic() + seconds
    while time.monotonic() < deadline:
        root.update()
        time.sleep(.01)
pump(.15)
assert len(app.tree.get_children()) == 1
assert 'LIVE' in app.status.get()
for tab in range(3):
    app.tabs.select(tab); root.update()
app.compare()
assert '0 / 1' in app.comparison.get()
with tempfile.TemporaryDirectory() as directory:
    target = Path(directory) / 'capture.csv'
    with patch.object(nest.filedialog, 'asksaveasfilename', return_value=str(target)):
        app.save()
    assert 'jayis1,live' in target.read_text()
    os.close(master)
    pump(.15)
    assert app.serial is None and not app.model.connected
    assert 'OFFLINE' in app.status.get()
    replay = Path(directory) / 'fixture.txt'; replay.write_bytes(fixture)
    with patch.object(nest.filedialog, 'askopenfilename', return_value=str(replay)):
        app.replay()
    assert app.model.source == 'replay' and len(app.tree.get_children()) == 1
    assert 'OFFLINE' in app.status.get()
    master, second = pty.openpty()
    app.port.set(os.ttyname(second)); app.connect()
    assert len(app.tree.get_children()) == 0 and app.live.get() == 'No samples'
    os.write(master, fixture); pump(.15)
    pump(1.1)
    assert 'STALE' in app.status.get()
    os.write(master, fixture); pump(.1)
    assert app.model.blocked and 'STALE' in app.status.get()
    app.disconnect(); os.close(master); os.close(second)
os.close(slave)
app.close()
print('PASS Tk live/history/comparison, PTY connect/unplug, export, replay, reconnect, stale backlog')
