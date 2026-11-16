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
class GuiTests(unittest.TestCase):
    def setUp(self):
        self.root=tk.Tk(); self.root.withdraw(); self.app=Workbench(self.root)
    def tearDown(self):
        self.app.close()
    def pump(self,seconds=.15):
        end=time.monotonic()+seconds
        while time.monotonic()<end:
            self.root.update(); time.sleep(.005)
    def test_replay_matrix_and_export(self):
        self.app.replay(ROOT/'firmware/build/demo.ay1'); self.pump()
        self.assertEqual(len(self.app.table.get_children()),10)
        self.assertTrue(all(int(v.get())>0 for v in self.app.cells))
        self.assertIn('HALT',self.app.current.get())
        with tempfile.TemporaryDirectory() as temp:
            path=Path(temp)/'out.csv'; self.app.session.export(path,False)
            self.assertIn('jayis1,replay',path.read_text())
    def test_live_serial_reconnect_hangup(self):
        master,slave=pty.openpty()
        try:
            self.app.replay(ROOT/'firmware/build/demo.ay1')
            self.app.port.set(os.ttyname(slave)); self.app.connect(); self.pump()
            self.assertEqual(self.app.current.get(),'No samples')
            self.assertTrue(all(v.get()=='0' for v in self.app.cells))
            original=self.app.serial; self.app.connect()
            self.assertIs(self.app.serial,original)
            with self.assertRaises(ValueError): self.app.replay(ROOT/'firmware/build/demo.ay1')
            frames=(ROOT/'firmware/build/demo.ay1').read_bytes().splitlines(keepends=True)
            for frame in frames[:3]:
                os.write(master,frame); self.pump(.1)
            self.assertEqual(len(self.app.table.get_children()),3)
            self.assertIn('Advancing',self.app.status.get())
            os.close(master); master=None; self.pump()
            self.assertIsNone(self.app.serial)
            self.assertIn('Stopped',self.app.status.get())
            self.assertEqual(len(self.app.session.rows),3)
        finally:
            if master is not None: os.close(master)
            os.close(slave)
    def test_failed_open_keeps_replay(self):
        self.app.replay(ROOT/'firmware/build/demo.ay1')
        previous=self.app.session
        self.app.port.set('/dev/null'); self.app.connect()
        self.assertIs(self.app.session,previous)
        self.assertIsNone(self.app.serial)
if __name__=='__main__': unittest.main()
