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
ROOT=Path(__file__).resolve().parents[1]
class GuiTest(unittest.TestCase):
    def test_live_replay_export_and_hangup(self):
        root=tk.Tk(); app=Workbench(root)
        master,slave=pty.openpty()
        try:
            app.replay(ROOT/'firmware/build/demo.ar1')
            self.assertIn('Removals 6',app.summary.get())
            self.assertEqual(len(app.trace.get_children()),len(app.session.rows))
            for tab in app.tabs.tabs(): app.tabs.select(tab); root.update()
            app.connect(os.ttyname(slave))
            self.assertIn('No evidence',app.summary.get())
            with self.assertRaises(ValueError): app.connect(os.ttyname(slave))
            os.write(master,(ROOT/'firmware/build/demo.ar1').read_bytes())
            for _ in range(8): root.update(); time.sleep(.02)
            self.assertIn('Ambiguous 2',app.summary.get())
            self.assertTrue(app.session.fresh())
            with tempfile.TemporaryDirectory() as temp:
                dest=Path(temp)/'out.csv'; app.export(dest)
                self.assertIn('jayis1,live,True',dest.read_text())
                with self.assertRaises(FileExistsError): app.export(dest)
            os.close(master); master=None
            for _ in range(8): root.update(); time.sleep(.02)
            self.assertFalse(app.session.connected)
            self.assertIsNone(app.serial)
            self.assertIn('Removals 6',app.summary.get())
            app.replay(ROOT/'firmware/build/demo.ar1')
            self.assertFalse(app.session.fresh())
        finally:
            app.close()
            if master is not None: os.close(master)
            os.close(slave)
if __name__=='__main__': unittest.main()
