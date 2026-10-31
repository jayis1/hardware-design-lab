# Author: jayis1. SPDX-License-Identifier: MIT
import os
from pathlib import Path
import sys
import tempfile
import time
import tkinter as tk
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'app'))
from workbench import Workbench
from test_app import fixture
class GuiTests(unittest.TestCase):
    def setUp(self):
        self.root=tk.Tk();self.app=Workbench(self.root)
    def tearDown(self):
        self.app.close()
    def pump(self,seconds=.15):
        end=time.monotonic()+seconds
        while time.monotonic()<end:self.root.update();time.sleep(.005)
    def test_pty_screens_export_disconnect(self):
        master,slave=os.openpty()
        try:
            self.app.path.set(os.ttyname(slave));self.app.connect()
            with self.assertRaises(ValueError):self.app.connect()
            os.write(master,fixture());self.pump()
            self.assertEqual(len(self.app.table.get_children()),4)
            self.assertIn('4',self.app.summary.get())
            self.assertEqual(len(self.app.tabs.tabs()),3)
            with tempfile.TemporaryDirectory() as td:
                path=Path(td)/'e.csv';self.app.session.export(path)
                self.assertIn('live',path.read_text())
            os.close(master);master=None;self.pump()
            self.assertIsNone(self.app.port)
            self.assertFalse(self.app.session.connected)
            self.assertIn('disconnected',self.app.status.get())
        finally:
            if master is not None:os.close(master)
            os.close(slave)
    def test_replay_clears_view(self):
        with tempfile.TemporaryDirectory() as td:
            path=Path(td)/'capture';path.write_bytes(fixture())
            self.app.replay(path);self.pump()
            self.assertEqual(len(self.app.table.get_children()),4)
            self.assertIn('REPLAY',self.app.status.get())
            self.app.new_session('live');self.app.render()
            self.assertEqual(len(self.app.table.get_children()),0)
            self.assertEqual(self.app.summary.get(),'No telemetry')
if __name__=='__main__':unittest.main()
