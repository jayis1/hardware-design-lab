# Author: jayis1. SPDX-License-Identifier: MIT
"""Exercise actual Tk event loop and POSIX serial with synthetic C fixtures."""
import json
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
from test_app import fixture
class GuiTests(unittest.TestCase):
    def test_workflow(self):
        root=tk.Tk(); app=Workbench(root)
        def pump(seconds):
            until=time.monotonic()+seconds
            while time.monotonic()<until:
                root.update(); time.sleep(.005)
        with tempfile.TemporaryDirectory() as tmp:
            p=Path(tmp)/'recording'; p.write_bytes(fixture())
            app.replay(str(p)); root.update()
            self.assertEqual(len(app.history.get_children()),9)
            self.assertIn('X traversals: 2',app.summary.get())
            self.assertEqual(len(app.matrix.get_children()),4)
            app.save(str(Path(tmp)/'export.json'))
            self.assertEqual(json.loads((Path(tmp)/'export.json').read_text())['source'],'replay')
            master,slave=pty.openpty()
            app.path.set(os.ttyname(slave)); app.connect(); root.update()
            self.assertEqual(app.summary.get(),'No evidence')
            original=app.port; app.connect(); self.assertIs(app.port,original)
            lines=fixture().splitlines(keepends=True)
            for line in lines[:3]:
                os.write(master,line); pump(.14)
            self.assertEqual(len(app.session.rows),3)
            self.assertTrue(app.session.fresh())
            pump(1.05); self.assertFalse(app.session.fresh())
            os.close(master); pump(.1); self.assertIsNone(app.port)
            self.assertEqual(app.session.source,'live')
            self.assertEqual(len(app.session.rows),3)
            app.save(str(Path(tmp)/'disconnected.json'))
            d=json.loads((Path(tmp)/'disconnected.json').read_text())
            self.assertFalse(d['connected']); self.assertFalse(d['fresh'])
            os.close(slave)
        app.close()
if __name__=='__main__': unittest.main()
