# Author: jayis1. SPDX-License-Identifier: MIT
"""Exercise real Tk event loop and Linux PTY, not physical USB."""
import importlib.util
import os
import pathlib
import pty
import tempfile
import time
import tkinter as tk
ROOT = pathlib.Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('pick_app', ROOT/'app/app.py')
assert spec and spec.loader
app = importlib.util.module_from_spec(spec)
spec.loader.exec_module(app)
root = tk.Tk()
ui = app.Workbench(root)
master, slave = pty.openpty()
ui.path.set(os.ttyname(slave))
ui.connect()
assert ui.fd is not None
os.write(master, (ROOT/'firmware/build/fixture.txt').read_bytes())
def spin(seconds):
    end=time.monotonic()+seconds
    while time.monotonic()<end:
        root.update()
        time.sleep(.01)
spin(.25)
assert ui.model.latest['accepted']==2
assert 'LIVE |' in ui.status.get()
assert 'repeated shed' in ui.summary.get()
assert len(ui.tree.get_children()) == len(ui.model.rows)
for index in range(3):
    ui.book.select(index)
    root.update()
spin(1.6)
assert ui.status.get().startswith('NOT LIVE')
with tempfile.TemporaryDirectory() as directory:
    target=pathlib.Path(directory)/'session.csv'
    app.filedialog.asksaveasfilename=lambda **kwargs: str(target)
    ui.export()
    assert 'live serial' in target.read_text()
    assert target.stat().st_mode & 0o777 == 0o600
    ui.disconnect()
    assert ui.model.source.startswith('live serial')
    app.filedialog.askopenfilename=lambda **kwargs: str(ROOT/'firmware/build/fixture.txt')
    ui.replay()
    assert ui.model.latest['accepted']==2
    assert ui.status.get().startswith('NOT LIVE')
    assert ui.model.source.startswith('replay ')
    assert len(ui.tree.get_children()) == len(ui.model.rows)
ui.close()
os.close(master); os.close(slave)
print('PASS Tk three views, PTY receive, stale timeout, CSV, disconnect, replay')
