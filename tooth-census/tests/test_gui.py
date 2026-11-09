# Author: jayis1. SPDX-License-Identifier: MIT
import os
from pathlib import Path
import pty
import sys
import tempfile
import time
import tkinter as tk
import unittest
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'app'))
from workbench import Workbench

class Screens(unittest.TestCase):
    def setUp(self):
        self.root=tk.Tk(); self.ui=Workbench(self.root)
    def tearDown(self):
        self.ui.close(); self.root.destroy()
    def test_replay_export_and_reconnect(self):
        self.ui.replay(ROOT/'firmware/build/demo.tc1')
        self.root.update()
        self.assertIn('Accepted 3',self.ui.summary.get())
        self.assertEqual(len(self.ui.table.get_children()),len(self.ui.session.rows))
        self.assertIn('replay',self.ui.status.get())
        with tempfile.TemporaryDirectory() as d:
            self.ui.export(Path(d)/'result.csv')
            self.assertIn('jayis1',(Path(d)/'result.csv').read_text())
        master,slave=pty.openpty()
        try:
            self.ui.connect(os.ttyname(slave)); self.root.update()
            self.assertEqual(self.ui.summary.get(),'No accepted telemetry')
            first=self.ui.serial
            with self.assertRaises(ValueError): self.ui.connect(os.ttyname(slave))
            self.assertIs(self.ui.serial,first)
            data=(ROOT/'firmware/build/demo.tc1').read_bytes().splitlines(True)
            for line in data[:3]:
                os.write(master,line); time.sleep(.1); self.ui.poll(); self.root.update()
            self.assertEqual(len(self.ui.session.rows),3)
            os.close(master); master=-1
            self.ui.poll(); self.root.update()
            self.assertFalse(self.ui.session.connected)
            self.assertIsNone(self.ui.serial)
        finally:
            if master>=0: os.close(master)
            os.close(slave)
    def test_bad_connection_retains_replay(self):
        self.ui.replay(ROOT/'firmware/build/demo.tc1')
        session=self.ui.session
        with self.assertRaises(Exception): self.ui.connect('/dev/null')
        self.assertIs(self.ui.session,session)

if __name__=='__main__': unittest.main()
