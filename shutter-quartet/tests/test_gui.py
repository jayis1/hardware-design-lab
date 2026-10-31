# Author: jayis1. SPDX-License-Identifier: MIT
"""Real Tk event-loop tests with synthetic C encoder data over a Linux PTY."""
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
class TestGUI(unittest.TestCase):
    def setUp(self):
        self.root=tk.Tk(); self.app=Workbench(self.root)
    def tearDown(self):
        self.app.close()
    def spin(self,seconds=.15):
        end=time.monotonic()+seconds
        while time.monotonic()<end:
            self.root.update(); time.sleep(.01)
    def test_live_replay_export_hangup(self):
        master,slave=pty.openpty()
        try:
            self.app.path.set(os.ttyname(slave)); self.app.connect()
            first_port=self.app.port
            self.app.connect()
            self.assertIs(first_port,self.app.port)
            frames=fixture().splitlines(keepends=True)
            os.write(master,frames[0]); self.spin(.1)
            os.write(master,frames[1]); self.spin(.1)
            self.assertIn('LIVE',self.app.status.get())
            self.assertIn('width spread 0',self.app.advice.get())
            self.assertEqual(len(self.app.history.get_children()),2)
            for i in range(3): self.app.tabs.select(i); self.spin(.02)
            os.close(master); master=None; self.spin()
            self.assertIsNone(self.app.port)
            self.assertFalse(self.app.session.connected)
            self.assertEqual(self.app.session.source,'live')
            with tempfile.TemporaryDirectory() as temp:
                p=Path(temp)/'record'; p.write_bytes(fixture())
                self.app.replay_path(p); self.spin()
                self.assertEqual(len(self.app.history.get_children()),4)
                self.assertIn('OFFLINE',self.app.status.get())
                out=Path(temp)/'out.csv'; self.app.export_path(out)
                self.assertIn('jayis1',out.read_text())
                # Failed replacement preserves evidence, not a blank display.
                old=self.app.session
                self.app.path.set('/dev/null'); self.app.connect()
                self.assertIs(old,self.app.session)
        finally:
            if master is not None: os.close(master)
            os.close(slave)
    def test_reconnect_clears_old_result(self):
        with tempfile.TemporaryDirectory() as temp:
            p=Path(temp)/'record'; p.write_bytes(fixture()); self.app.replay_path(p)
            master,slave=pty.openpty()
            try:
                self.app.path.set(os.ttyname(slave)); self.app.connect(); self.spin(.03)
                self.assertIn('No completed',self.app.advice.get())
                self.assertEqual(len(self.app.history.get_children()),0)
            finally:
                self.app.disconnect(); os.close(master); os.close(slave)
if __name__=='__main__': unittest.main()
