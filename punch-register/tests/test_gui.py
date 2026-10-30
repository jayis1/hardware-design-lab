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
ROOT=Path(__file__).resolve().parents[1]
class GuiTest(unittest.TestCase):
    def test_workflows(self):
        root=tk.Tk(); app=Workbench(root)
        master,slave=pty.openpty()
        data=subprocess.check_output([str(ROOT/'firmware/build/test-engine'),'--fixture'])
        try:
            app.path.set(os.ttyname(slave)); app.connect()
            self.assertTrue(app.session.connected)
            for line in data.splitlines(keepends=True):
                os.write(master,line)
                for _ in range(8): root.update(); time.sleep(.015)
            self.assertEqual(app.session.rows[-1]['good'],1)
            self.assertIn('1',app.summary.get())
            self.assertEqual(len(app.history.get_children()),2)
            for tab in app.tabs.tabs(): app.tabs.select(tab); root.update()
            app.connect(); self.assertIn('already',app.status.get())
            os.close(master); master=None
            for _ in range(8): root.update(); time.sleep(.015)
            self.assertFalse(app.session.connected)
            self.assertIn('disconnected',app.status.get())
            with tempfile.TemporaryDirectory() as directory:
                record=Path(directory)/'record'; record.write_bytes(data)
                app.replay(record); self.assertEqual(app.session.source,'replay')
                output=Path(directory)/'export.json'; app.save(output)
                self.assertTrue(output.exists())
                app.save(output); self.assertIn('exists',app.status.get())
                # Replacement failure preserves the old evidence.
                app.replay(Path(directory)/'missing')
                self.assertEqual(app.session.rows[-1]['good'],1)
        finally:
            app.close(); os.close(slave)
            if master is not None: os.close(master)
if __name__=='__main__': unittest.main()
