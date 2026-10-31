# Author: jayis1. SPDX-License-Identifier: MIT
"""Exercise actual Tk screens with a synthetic C-generated serial stream."""
import os
import pty
import sys
import tempfile
import termios
import time
import tkinter as tk
import unittest
from pathlib import Path
from test_app import fixture
from workbench import Workbench
class GUI(unittest.TestCase):
    def setUp(self):
        self.root=tk.Tk(); self.ui=Workbench(self.root)
        self.master,self.slave=pty.openpty()
        self.original=termios.tcgetattr(self.slave)
    def tearDown(self):
        self.ui.close()
        if self.master is not None: os.close(self.master)
        os.close(self.slave)
    def pump(self,seconds=.08):
        end=time.monotonic()+seconds
        while time.monotonic()<end:
            self.root.update(); time.sleep(.005)
    def test_live_screens_export_replay(self):
        self.ui.path.set(os.ttyname(self.slave)); self.ui.connect()
        port=self.ui.port; self.ui.connect(); self.assertIs(self.ui.port,port)
        lines=fixture().splitlines(keepends=True)
        for line in lines:
            os.write(self.master,line); self.pump(.1)
        self.assertEqual(len(self.ui.history.get_children()),5)
        self.assertIn('LIVE',self.ui.status.get())
        self.assertEqual(self.ui.holders.item('a')['values'][2],1)
        for tab in self.ui.tabs.tabs(): self.ui.tabs.select(tab); self.pump(.025)
        with tempfile.TemporaryDirectory() as d:
            out=Path(d)/'out.csv'; self.ui.export_path(out)
            self.assertIn('jayis1',out.read_text())
            os.close(self.master); self.master=None; self.pump()
            self.assertIsNone(self.ui.port)
            self.assertIn('DISCONNECTED',self.ui.status.get())
            capture=Path(d)/'recording'; capture.write_bytes(fixture())
            self.ui.replay_path(capture)
            self.assertEqual(self.ui.session.source,'replay')
            self.assertIn('OFFLINE',self.ui.status.get())
            self.assertEqual(len(self.ui.history.get_children()),5)
    def test_setup_error_and_restore(self):
        self.ui.path.set('/dev/null'); self.ui.connect()
        self.assertIsNone(self.ui.port)
        self.assertIn('failed',self.ui.message.get())
        self.ui.path.set(os.ttyname(self.slave)); self.ui.connect()
        self.ui.disconnect()
        self.assertEqual(termios.tcgetattr(self.slave),self.original)
        self.ui.connect(); self.assertEqual(len(self.ui.history.get_children()),0)
if __name__=='__main__': unittest.main()
