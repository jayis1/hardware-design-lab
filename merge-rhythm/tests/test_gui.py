# Author: jayis1. SPDX-License-Identifier: MIT
"""Real Tk event loop and Linux PTYs; data are synthetic C-engine traces."""
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
from workbench import Workbench, Serial
ROOT=Path(__file__).resolve().parents[1]
class GuiTest(unittest.TestCase):
    def setUp(self):
        self.root=tk.Tk(); self.app=Workbench(self.root)
        self.master,self.slave=pty.openpty()
        self.original=termios.tcgetattr(self.slave)
        self.data=subprocess.check_output([str(ROOT/'firmware/build/test-engine'),'--fixture'])
    def tearDown(self):
        self.app.close()
        if self.master is not None: os.close(self.master)
        os.close(self.slave)
    def pump(self,seconds=.15):
        end=time.monotonic()+seconds
        while time.monotonic()<end:
            self.root.update(); time.sleep(.005)
    def test_live_export_disconnect_replay(self):
        self.app.connect(os.ttyname(self.slave))
        with self.assertRaises(ValueError): self.app.connect(os.ttyname(self.slave))
        for line in self.data.splitlines(keepends=True):
            os.write(self.master,line); self.pump(.05)
        self.assertEqual(len(self.app.session.rows),8)
        self.assertTrue(self.app.session.fresh(time.monotonic()))
        self.assertIn('Delivered L: 3',self.app.live.get())
        for i in range(3): self.app.tabs.select(i); self.root.update()
        with tempfile.TemporaryDirectory() as d:
            out=Path(d)/'report.csv'; self.app.export(out)
            self.assertEqual(out.stat().st_mode & 0o777,0o600)
            with out.open() as handle:
                rows=list(csv.reader(handle))
            self.assertEqual(len(rows),10); self.assertIn('jayis1',rows[0])
            with self.assertRaises(FileExistsError): self.app.export(out)
            self.app.disconnect()
            self.assertEqual(termios.tcgetattr(self.slave),self.original)
            self.assertFalse(self.app.session.fresh(time.monotonic()))
            capture=Path(d)/'capture'; capture.write_bytes(self.data)
            self.app.load(capture); self.assertEqual(self.app.session.provenance,'replay')
            self.assertEqual(len(self.app.history.get_children()),8)
            self.app.connect(os.ttyname(self.slave))
            self.assertEqual(self.app.live.get(),'No observations')
    def test_hangup(self):
        self.app.connect(os.ttyname(self.slave))
        os.write(self.master,self.data+b'MR1,'); self.pump()
        os.close(self.master); self.master=None; self.pump()
        self.assertIsNone(self.app.serial)
        self.assertFalse(self.app.session.connected)
        self.assertEqual(self.app.session.provenance,'live')
        self.assertEqual(self.app.session.errors,1)
    def test_serial_constructor_rollback(self):
        with patch('termios.tcflush',side_effect=termios.error('injected')):
            with self.assertRaises(termios.error): Serial(os.ttyname(self.slave))
        self.assertEqual(termios.tcgetattr(self.slave),self.original)
    def test_partial_and_corrupt_export(self):
        self.app.connect(os.ttyname(self.slave))
        os.write(self.master,b'noise'+self.data+b'MR1,'); self.pump()
        self.app.disconnect()
        self.assertGreaterEqual(self.app.session.errors,2)
        with tempfile.TemporaryDirectory() as d:
            out=Path(d)/'errors.csv'; self.app.export(out)
            self.assertIn('errors',out.read_text())
if __name__=='__main__': unittest.main()
