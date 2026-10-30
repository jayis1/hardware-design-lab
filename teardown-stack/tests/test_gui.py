# Author: jayis1. SPDX-License-Identifier: MIT
import csv
import os
from pathlib import Path
import pty
import subprocess
import sys
import tempfile
import termios
import time
import tkinter as tk
import unittest
from unittest.mock import patch
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'app'))
from workbench import Workbench, Serial, export
ROOT=Path(__file__).resolve().parents[1]
class GuiTests(unittest.TestCase):
    def setUp(self):
        self.root=tk.Tk(); self.app=Workbench(self.root)
        self.master,self.slave=pty.openpty()
        self.path=os.ttyname(self.slave)
        self.original=termios.tcgetattr(self.slave)
        self.lines=subprocess.check_output([str(ROOT/'firmware/build/test-engine'),'--fixture']).splitlines(keepends=True)
    def tearDown(self):
        self.app.close()
        if self.master is not None: os.close(self.master)
        os.close(self.slave)
    def pump(self):
        deadline=time.monotonic()+0.08
        while time.monotonic()<deadline:
            self.root.update(); time.sleep(0.005)
    def test_live_screens_export_disconnect(self):
        self.app.connect(self.path)
        with self.assertRaises(ValueError): self.app.connect(self.path)
        os.write(self.master,b'late-suffix\n'+self.lines[0]); self.pump()
        for line in self.lines[1:]:
            os.write(self.master,line); self.pump()
        self.assertEqual(len(self.app.session.rows),5)
        self.assertEqual(len(self.app.tree.get_children()),5)
        self.assertIn('Completed cup sequences: 1',self.app.summary.get())
        self.assertTrue(self.app.session.fresh())
        for frame in (self.app.setup,self.app.live,self.app.history):
            self.app.tabs.select(frame); self.pump()
        self.app.disconnect()
        self.assertEqual(termios.tcgetattr(self.slave),self.original)
        with tempfile.TemporaryDirectory() as d:
            path=Path(d)/'evidence.csv'; export(self.app.session,path)
            self.assertEqual(path.stat().st_mode & 0o777,0o600)
            with path.open() as stream:
                rows=list(csv.reader(stream))
            self.assertIn('"provenance": "live"',rows[1][1])
            self.assertIn('"connected": false',rows[1][1])
            with self.assertRaises(FileExistsError): export(self.app.session,path)
        old=self.app.session
        with self.assertRaises(OSError): self.app.connect('/no/such/port')
        self.assertIs(old,self.app.session)
    def test_replay_and_hangup(self):
        with tempfile.TemporaryDirectory() as d:
            recording=Path(d)/'synthetic.txt'; recording.write_bytes(b''.join(self.lines))
            self.app.replay(recording)
            self.assertEqual(len(self.app.tree.get_children()),5)
            self.assertFalse(self.app.session.fresh())
        self.app.connect(self.path)
        self.assertIn('No evidence',self.app.summary.get())
        os.write(self.master,self.lines[0]); self.pump()
        os.close(self.master); self.master=None; self.pump()
        self.assertIsNone(self.app.serial)
        self.assertFalse(self.app.session.connected)
    def test_constructor_restores_settings(self):
        with patch('workbench.termios.tcflush',side_effect=termios.error('injected')):
            with self.assertRaises(termios.error): Serial(self.path)
        self.assertEqual(termios.tcgetattr(self.slave),self.original)
if __name__=='__main__': unittest.main()
