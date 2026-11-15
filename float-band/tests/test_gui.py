# Author: jayis1. SPDX-License-Identifier: MIT
import os
from pathlib import Path
import pty
import sys
import time
import tkinter as tk
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'app'))
from workbench import Workbench

class GuiTest(unittest.TestCase):
    def test_replay_and_live(self):
        root=tk.Tk(); root.withdraw(); app=Workbench(root)
        path=Path(__file__).resolve().parents[1]/'firmware/build/demo.fb1'
        app.replay(path); root.update()
        self.assertIn('REPLAY',app.status.get())
        self.assertEqual(app.table.item('1')['values'],['low–mid',1,1])
        a,b=pty.openpty()
        try:
            app.connect(os.ttyname(b))
            self.assertEqual(len(app.session.rows),0)
            for line in path.read_bytes().splitlines(keepends=True):
                os.write(a,line)
                deadline=time.monotonic()+.1
                while time.monotonic()<deadline:
                    root.update(); time.sleep(.005)
            self.assertEqual(app.session.last['edges'],4)
            self.assertIn('LIVE',app.status.get())
            with self.assertRaises(ValueError): app.connect(os.ttyname(b))
            os.close(a); a=None
            deadline=time.monotonic()+.2
            while time.monotonic()<deadline: root.update(); time.sleep(.005)
            self.assertIsNone(app.port)
            self.assertIn('DISCONNECTED',app.status.get())
            self.assertEqual(app.session.last['edges'],4)
        finally:
            if a is not None: os.close(a)
            os.close(b); app.close()
if __name__=='__main__': unittest.main()
