# Author: jayis1. SPDX-License-Identifier: MIT
import os
from pathlib import Path
import pty
import subprocess
import sys
import tempfile
import time
import tkinter as tk
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'app'))
from workbench import Workbench
from test_app import frame, idle

class GuiTests(unittest.TestCase):
    def test_real_event_loop(self):
        root=tk.Tk(); app=Workbench(root)
        master,slave=pty.openpty()
        try:
            app.path.set(os.ttyname(slave)); app.connect()
            for i in range(4):
                os.write(master,frame(idle(10+i*100)))
                until=time.monotonic()+.11
                while time.monotonic()<until:
                    root.update(); time.sleep(.005)
            self.assertEqual(len(app.tabs.tabs()),3)
            self.assertTrue(app.session.fresh())
            self.assertIn('READY',app.summary.get())
            app.connect()  # second ownership is refused
            self.assertIsNotNone(app.port)
            with tempfile.TemporaryDirectory() as temp:
                path=Path(temp)/'evidence.csv'
                app.export_to(path); self.assertIn('jayis1',path.read_text())
            os.close(master); master=None
            until=time.monotonic()+.2
            while time.monotonic()<until: root.update(); time.sleep(.005)
            self.assertIsNone(app.port)
            self.assertFalse(app.session.connected)
            self.assertGreater(len(app.session.rows),0)
            app.path.set('/dev/null'); app.connect()
            self.assertIsNone(app.port)
            self.assertGreater(len(app.session.rows),0)
            base=Path(__file__).resolve().parents[1]
            with tempfile.TemporaryDirectory() as temp:
                exe=Path(temp)/'fixture'
                subprocess.run(['gcc','-std=c11','-I'+str(base/'firmware'),
                    str(base/'tests/fixture.c'),str(base/'firmware/engine.c'),
                    str(base/'firmware/wire.c'),'-o',str(exe)],check=True)
                source=Path(temp)/'trace'
                source.write_bytes(subprocess.check_output([exe]))
                app.replay_path(source); root.update()
                self.assertIn('Matching 1',app.summary.get())
                self.assertIn('Mismatching 1',app.summary.get())
                self.assertEqual(len(app.history.get_children()),2)
                self.assertFalse(app.session.fresh())
        finally:
            app.close()
            if master is not None: os.close(master)
            os.close(slave)

if __name__=='__main__': unittest.main()
