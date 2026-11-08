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
            app.replay(ROOT/'firmware/build/demo.pb1')
            self.assertIn('transfers 2 / returns 1',app.summary.get())
            self.assertEqual(len(app.trace.get_children()),len(app.session.rows))
            for tab in app.tabs.tabs(): app.tabs.select(tab); root.update()
            app.connect(os.ttyname(slave))
            self.assertIn('No evidence',app.summary.get())
            with self.assertRaises(ValueError): app.connect(os.ttyname(slave))
            # Deliver real encoder snapshots at a sparse cadence, within PTY capacity.
            lines=(ROOT/'firmware/build/demo.pb1').read_bytes().splitlines(keepends=True)
            selected=lines[::10]+[lines[-1]]
            os.write(master,b''.join(selected))
            for _ in range(8): root.update(); time.sleep(.02)
            self.assertIn('transfers 2 / returns 1',app.summary.get())
            self.assertTrue(app.session.fresh())
            self.assertEqual(len(app.trace.get_children()),len(selected))
            with tempfile.TemporaryDirectory() as temp:
                dest=Path(temp)/'out.csv'; app.export(dest)
                self.assertIn('jayis1,live,True',dest.read_text())
                with self.assertRaises(FileExistsError): app.export(dest)
            time.sleep(1.05); root.update()
            self.assertFalse(app.session.fresh())
            os.close(master); master=None
            for _ in range(8): root.update(); time.sleep(.02)
            self.assertFalse(app.session.connected)
            self.assertIsNone(app.serial)
            self.assertIn('transfers 2 / returns 1',app.summary.get())
            app.replay(ROOT/'firmware/build/demo.pb1')
            self.assertFalse(app.session.fresh())
        finally:
            app.close()
            if master is not None: os.close(master)
            os.close(slave)
if __name__=='__main__': unittest.main()
