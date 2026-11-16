# Author: jayis1. SPDX-License-Identifier: MIT
"""Real Tk event-loop tests with synthetic C-encoded serial data."""
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
from transport import Decoder
ROOT=Path(__file__).resolve().parents[1]
class GuiTests(unittest.TestCase):
    def setUp(self):
        self.root=tk.Tk(); self.ui=Workbench(self.root)
    def tearDown(self):
        self.ui.close()
    def pump(self):
        for _ in range(8):
            self.root.update(); time.sleep(.02)
    def test_replay_screens_export(self):
        self.ui.replay(ROOT/'firmware/build/demo.cd1'); self.pump()
        self.assertIn('Completed: 3', self.ui.summary.get())
        self.assertIn('HALTED', self.ui.state.get())
        self.assertEqual(len(self.ui.tabs.tabs()),3)
        for tab in self.ui.tabs.tabs():
            self.ui.tabs.select(tab)
            self.root.update()
            self.assertEqual(self.ui.tabs.select(),tab)
        self.assertGreater(len(self.ui.history.get_children()),3)
        with tempfile.TemporaryDirectory() as t:
            path=Path(t)/'capture.csv'; self.ui.export(path)
            self.assertIn('jayis1',path.read_text())
        # A failed replacement must retain existing evidence.
        with self.assertRaises(OSError): self.ui.replay('/no/such/case-delta-file')
        self.assertEqual(self.ui.session.rows[-1]['completed'],3)
    def test_serial_disconnect_double_connect(self):
        master,slave=pty.openpty()
        try:
            self.ui.connect(os.ttyname(slave))
            with self.assertRaises(ValueError): self.ui.connect(os.ttyname(slave))
            data=(ROOT/'firmware/build/demo.cd1').read_bytes()
            os.write(master,data); self.pump()
            self.assertEqual(self.ui.session.rows[-1]['completed'],3)
            os.close(master); master=None; self.pump()
            self.assertFalse(self.ui.session.connected)
            self.assertIsNone(self.ui.serial)
            self.assertEqual(self.ui.session.provenance,'live')
            # Replacement connection clears visible old data before its first frame.
            m2,s2=pty.openpty()
            try:
                self.ui.connect(os.ttyname(s2)); self.pump()
                self.assertEqual(len(self.ui.history.get_children()),0)
                self.assertIn('No snapshots',self.ui.summary.get())
            finally:
                self.ui.disconnect(); os.close(m2); os.close(s2)
        finally:
            if master is not None: os.close(master)
            os.close(slave)
if __name__=='__main__': unittest.main()
