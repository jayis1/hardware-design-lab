# Author: jayis1. SPDX-License-Identifier: MIT
"""Real Tk event loop + real encoder fixture over Linux pseudo-terminal."""
import os, sys, tempfile, time, tkinter as tk, termios
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'app'))
from main import App
from transport import Decoder
root=tk.Tk(); app=App(root)
fixture=Path(__file__).resolve().parents[1]/'firmware/build/demo.sm1'
app.replay(fixture)
root.update()
assert app.session.rows[-1]['completed']==1
assert len(app.table.get_children())==200
assert len(app.canvas.find_all())==132
app.tabs.select(app.table_page); root.update()
with tempfile.TemporaryDirectory() as tmp:
    path=Path(tmp)/'capture.csv'; app.export(path)
    assert 'jayis1' in path.read_text() and 'replay' in path.read_text()
    assert path.stat().st_mode&0o777==0o600
    try: app.export(path); raise AssertionError('overwrite allowed')
    except FileExistsError: pass
    fifo=Path(tmp)/'fifo'; os.mkfifo(fifo)
    try: app.replay(fifo); raise AssertionError('FIFO accepted')
    except ValueError: pass
    assert app.session.rows[-1]['completed']==1
master,slave=os.openpty(); name=os.ttyname(slave)
saved=termios.tcgetattr(slave)
app.connect(name)
assert not app.session.rows and app.summary.get()=='No retained capture'
try: app.connect(name); raise AssertionError('double connect')
except ValueError: pass
# Batch several actual encoded records through production poll, then drain
# remaining records; total simulated device span is below freshness allowance.
lines=fixture.read_bytes().splitlines(keepends=True)
for offset in range(0,len(lines),20):
    os.write(master,b''.join(lines[offset:offset+20]))
    until=time.monotonic()+0.06
    while time.monotonic()<until:
        root.update(); time.sleep(0.002)
assert app.session.rows[-1]['completed']==1
assert app.fresh()
os.close(master)
until=time.monotonic()+0.15
while time.monotonic()<until:
    root.update(); time.sleep(0.002)
assert app.serial is None and not app.fresh()
assert app.session.rows[-1]['completed']==1
os.close(slave)
# Failed connect preserves evidence; replay clears old transport diagnostics.
try: app.connect('/dev/null'); raise AssertionError('non-TTY accepted')
except (OSError,termios.error): pass
app.decoder.errors=3
app.replay(fixture)
assert app.decoder.errors==0 and app.last_progress is None
app.close()
print('PASS: Tk screens, real C replay, private export, FIFO refusal, PTY live/hangup, reconnect guards')
