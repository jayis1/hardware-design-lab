# Author: jayis1. SPDX-License-Identifier: MIT
"""Real Tk event loop and C encoder over a Linux pseudo-terminal."""
import os, pty, subprocess, sys, tempfile, time, tkinter as tk, unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'app'))
from workbench import Workbench

class GUI(unittest.TestCase):
    def test_encoder_serial_replay_export(self):
        with tempfile.TemporaryDirectory() as directory:
            binary=Path(directory)/'fixture'
            subprocess.run(['gcc','-std=c11','-Wall','-Wextra','-Werror','-I'+str(ROOT/'firmware'),
                            str(ROOT/'tests/fixture.c'),str(ROOT/'firmware/engine.c'),
                            str(ROOT/'firmware/wire.c'),'-o',str(binary)],check=True)
            data=subprocess.check_output([str(binary)])
            recording=Path(directory)/'recording'; recording.write_bytes(data)
            root=tk.Tk(); app=Workbench(root)
            master,slave=pty.openpty()
            try:
                app.path.set(os.ttyname(slave)); app.connect()
                port=app.port; app.connect(); self.assertIs(app.port,port)
                for line in data.splitlines(keepends=True):
                    os.write(master,line)
                    end=time.monotonic()+.1
                    while time.monotonic()<end: root.update(); time.sleep(.005)
                self.assertTrue(app.session.fresh())
                self.assertIn('CONTRADICTORY',app.summary.get())
                self.assertIn('Aborted 1',app.summary.get())
                self.assertEqual(len(app.gauges.get_children()),4)
                self.assertGreater(len(app.history.get_children()),3)
                for i in range(4): app.tabs.select(i); root.update()
                os.close(master); master=None
                end=time.monotonic()+.15
                while time.monotonic()<end: root.update(); time.sleep(.005)
                self.assertIsNone(app.port); self.assertFalse(app.session.fresh())
                output=Path(directory)/'live.csv'; app.export_to(output)
                self.assertIn('live',output.read_text())
                app.replay_path(recording)
                self.assertEqual(app.session.rows[-1]['accepted'],3)
                self.assertFalse(app.session.fresh())
                self.assertIn('REPLAY',app.status.get())
                app.path.set('/dev/null'); app.connect()
                self.assertIn('Connection failed',app.status.get())
                self.assertEqual(app.session.provenance,'replay')
            finally:
                if master is not None: os.close(master)
                os.close(slave); app.close()
if __name__=='__main__': unittest.main()
