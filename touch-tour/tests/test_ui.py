# Author: jayis1. SPDX-License-Identifier: MIT
import os
from pathlib import Path
import pty
import sys
import tempfile
import time
import tkinter as tk
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'app'))
from workbench import Workbench
from model import Session

class UI(unittest.TestCase):
    def test_replay_live_export_disconnect(self):
        root=tk.Tk(); app=Workbench(root)
        master,slave=pty.openpty()
        try:
            fixture=Path(__file__).resolve().parents[1]/'firmware/build/demo.tt1'
            app.replay(str(fixture)); root.update()
            self.assertIn('rounds: 2',app.summary.get())
            for tab in app.tabs.tabs():
                app.tabs.select(tab); root.update()
            with tempfile.TemporaryDirectory() as d:
                app.session.export(Path(d)/'capture.json')
                app.connect(os.ttyname(slave)); root.update()
                self.assertFalse(app.session.rows)
                with self.assertRaises(ValueError): app.connect(os.ttyname(slave))
                frames=fixture.read_bytes().splitlines(keepends=True)
                os.write(master,b'suffix\n'+b''.join(frames[:24]))
                for _ in range(5):
                    app.poll(); root.update(); time.sleep(.01)
                self.assertEqual(len(app.session.rows),24)
                self.assertEqual(app.session.rows[-1]['mask'],5)
                self.assertTrue(app.session.fresh())
                os.close(master); master=None
                for _ in range(5): app.poll(); root.update()
                self.assertFalse(app.session.connected)
                self.assertFalse(app.session.fresh())
                self.assertEqual(app.session.provenance,'live')
                self.assertEqual(len(app.session.rows),24)
            app.replay(str(fixture)); root.update()
            self.assertEqual(app.session.provenance,'replay')
        finally:
            app.close()
            if master is not None: os.close(master)
            os.close(slave)
if __name__=='__main__': unittest.main()
