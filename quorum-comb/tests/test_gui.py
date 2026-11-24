# Author: jayis1. SPDX-License-Identifier: MIT
"""Real Tk event loop and serial PTY; synthetic C-encoder data only."""
import os
from pathlib import Path
import sys
import tempfile
import time
import tkinter as tk
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'app'))
from main import App
root=tk.Tk(); app=App(root)
fixture=Path(__file__).resolve().parents[1]/'firmware/build/demo.qc1'
app.replay(str(fixture)); root.update()
assert len(app.log.get_children())==160
assert len(app.table.get_children())==5
assert '1600' not in app.status.get()  # Status describes provenance, not live time.
with tempfile.TemporaryDirectory() as directory:
    path=Path(directory)/'evidence.csv'
    app.export(str(path)); text=path.read_text()
    assert 'jayis1' in text and 'replay' in text
    assert path.stat().st_mode & 0o777 == 0o600
    app.export(str(path)); assert path.read_text()==text
    m,s=os.openpty(); app.path.set(os.ttyname(s)); app.connect(); root.update()
    assert app.port is not None and not app.log.get_children()
    original=app.port; app.connect(); assert app.port is original
    # Feed records at their actual device-time cadence to test freshness.
    for frame in fixture.read_bytes().splitlines(keepends=True)[:12]:
        os.write(m,frame)
        end=time.monotonic()+.011
        while time.monotonic()<end:
            root.update(); time.sleep(.002)
    end=time.monotonic()+.06
    while time.monotonic()<end: root.update(); time.sleep(.002)
    assert len(app.log.get_children())==12
    assert app.session.fresh()
    app.disconnect(); assert not app.session.fresh()
    assert app.session.source=='live' and len(app.session.rows)==12
    app.export(str(Path(directory)/'disconnected.csv'))
    assert 'live,connected,False,fresh,False' in (Path(directory)/'disconnected.csv').read_text()
    app.connect(); os.close(m)
    end=time.monotonic()+.08
    while time.monotonic()<end: root.update(); time.sleep(.002)
    assert app.port is None and 'disconnected' in app.status.get()
    os.close(s)
    app.replay(str(fixture)); assert app.session.source=='replay'
app.close()
print('PASS real Tk replay, two screens, CSV, overwrite refusal, paced PTY, reconnect, hangup')
