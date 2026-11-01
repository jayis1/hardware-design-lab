# Author: jayis1. SPDX-License-Identifier: MIT
"""Real Tk event loop, real C fixtures, and simulated POSIX serial transport."""
import os
from pathlib import Path
import pty
import subprocess
import sys
import tempfile
import time
import tkinter as tk
import unittest
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'app'))
from workbench import Workbench

class GuiTests(unittest.TestCase):
    def test_workflow(self):
        with tempfile.TemporaryDirectory() as tmp:
            binary=Path(tmp)/'fixture'
            subprocess.run(['gcc','-std=c11','-Wall','-Wextra','-Werror','-I'+str(ROOT/'firmware'),
                            str(ROOT/'tests/fixture.c'),str(ROOT/'firmware/engine.c'),
                            str(ROOT/'firmware/wire.c'),'-o',str(binary)],check=True)
            data=subprocess.check_output([str(binary)])
            capture=Path(tmp)/'capture.dw1'; capture.write_bytes(data)
            root=tk.Tk(); app=Workbench(root)
            master,slave=pty.openpty()
            def pump(seconds=.06):
                end=time.monotonic()+seconds
                while time.monotonic()<end: root.update(); time.sleep(.005)
            try:
                self.assertEqual(len(app.tabs.tabs()),4)
                for tab in app.tabs.tabs(): app.tabs.select(tab); pump(.01)
                app.replay_path(capture); pump()
                final=app.session.rows[-1]
                self.assertEqual((final['good'],final['bad'],final['aborted']),(1,1,1))
                self.assertEqual(len(app.history.get_children()),3)
                self.assertIn('Matching 1',app.summary.get())
                app.export_to(Path(tmp)/'replay.csv')
                with self.assertRaises(FileExistsError): app.export_to(Path(tmp)/'replay.csv')
                app.path.set('/dev/null'); app.connect(); self.assertIsNone(app.port)
                self.assertIn('Matching 1',app.summary.get())
                app.path.set(os.ttyname(slave)); app.connect(); pump()
                self.assertEqual(app.summary.get(),'No samples')
                original=app.port; app.connect(); self.assertIs(app.port,original)
                with self.assertRaises(ValueError): app.replay_path(capture)
                # Preserve device pacing; simulation does not imply real USB hardware.
                previous=0
                for line in data.splitlines(keepends=True):
                    stamp=int(line.split(b',')[1])
                    pump((stamp-previous)/1000); os.write(master,line); previous=stamp
                pump(.1)
                self.assertEqual(app.session.rows[-1]['aborted'],1)
                self.assertTrue(app.session.fresh())
                self.assertEqual(len(app.history.get_children()),3)
                os.close(master); master=None; pump()
                self.assertIsNone(app.port); self.assertFalse(app.session.fresh())
                app.export_to(Path(tmp)/'disconnected.csv')
                self.assertIn('live', (Path(tmp)/'disconnected.csv').read_text())
                self.assertIn('False', (Path(tmp)/'disconnected.csv').read_text())
            finally:
                app.close()
                if master is not None: os.close(master)
                os.close(slave)

if __name__=='__main__': unittest.main()
