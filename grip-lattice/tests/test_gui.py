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
ROOT=Path(__file__).resolve().parents[1]
class GuiTests(unittest.TestCase):
    def setUp(self):
        self.root=tk.Tk(); self.ui=Workbench(self.root); self.root.update()
    def tearDown(self):
        self.ui.close()
    def test_replay_screens_and_connection_failure(self):
        self.ui.replay_path(ROOT/'firmware/build/demo.gl1'); self.root.update()
        self.assertEqual(len(self.ui.lattice.get_children()),8)
        self.assertIn('Completed 1',self.ui.summary.get())
        self.assertGreater(len(self.ui.history.get_children()),1)
        for tab in self.ui.tabs.tabs():
            self.ui.tabs.select(tab); self.root.update()
        self.ui.port.set('/dev/null'); self.ui.connect()
        self.assertIn('Connection failed', self.ui.status.get())
        self.assertEqual(self.ui.session.source,'replay')
    def test_live_real_encoder_and_hangup(self):
        master,slave=pty.openpty()
        self.ui.port.set(os.ttyname(slave)); self.ui.connect()
        self.assertTrue(self.ui.session.connected)
        self.ui.connect() # must not open same tty twice
        frames=(ROOT/'firmware/build/demo.gl1').read_bytes().splitlines(keepends=True)
        for line in frames:
            os.write(master,line)
            deadline=time.monotonic()+.1
            while time.monotonic()<deadline:
                self.root.update(); time.sleep(.005)
        self.assertIn('Completed 1', self.ui.summary.get())
        self.assertTrue(self.ui.session.fresh(time.monotonic()))
        os.close(master)
        for _ in range(20): self.root.update(); time.sleep(.01)
        self.assertFalse(self.ui.session.connected)
        self.assertEqual(self.ui.session.source,'live')
        os.close(slave)
if __name__=='__main__': unittest.main()
