# Author: jayis1. SPDX-License-Identifier: MIT
"""Real Tk event loop with Linux PTY transport, not physical hardware."""
import os
from pathlib import Path
import sys
import tempfile
import time
import tkinter as tk
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'app'))
from cam import App
ROOT = Path(__file__).resolve().parents[1]
fixture = ROOT / 'firmware/build/fixture.txt'
frame = fixture.read_bytes().splitlines()[-1]+b'\n'
root = tk.Tk(); app = App(root)
def pump(seconds=0.08):
    deadline = time.monotonic()+seconds
    while time.monotonic() < deadline:
        root.update(); time.sleep(0.005)
try:
    app.load(fixture); pump()
    assert app.session.source == 'replay'
    assert len(app.table.get_children()) == 1
    assert len(app.canvas.find_all()) >= 40
    for page in range(3):
        app.tabs.select(page); pump()
        assert app.tabs.index(app.tabs.select()) == page
    master, slave = os.openpty()
    try:
        app.path.set(os.ttyname(slave)); app.connect(); pump()
        assert not app.table.get_children()
        assert not app.canvas.find_all()
        os.write(master, frame); pump()
        assert app.session.source == 'live'
        assert len(app.table.get_children()) == 1
        # Duplicate timestamps do not extend freshness.
        pump(1.05); os.write(master, frame); pump()
        assert app.session.blocked and app.port is None
        app.connect(); pump(); os.write(master, frame); pump()
        assert not app.session.blocked and app.session.connected
        os.close(master); master = None; pump()
        assert not app.session.connected and app.port is None
        with tempfile.TemporaryDirectory() as d:
            output = Path(d)/'capture.csv'
            app.session.export(output, time.monotonic(), app.stream.errors)
            assert 'live' in output.read_text()
            assert 'connected,False' in output.read_text()
        app.load(fixture); pump()
        assert app.session.source == 'replay'
    finally:
        if master is not None: os.close(master)
        os.close(slave)
finally:
    app.close()
print('PASS real Tk screens, replay, angular chart, live PTY, freshness freeze, reconnect, hangup, CSV')
