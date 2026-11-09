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
        self.root=tk.Tk(); self.app=Workbench(self.root)
    def tearDown(self): self.app.close()
    def pump(self):
        until=time.monotonic()+0.12
        while time.monotonic()<until: self.root.update(); time.sleep(.005)
    def test_replay_export_screens(self):
        self.app.replay(ROOT/'firmware/build/demo.ss1'); self.root.update()
        self.assertEqual(len(self.app.tabs.tabs()),3)
        self.assertIn('16',self.app.summary.get())
        self.assertIn('HALT',self.app.summary.get())
        self.assertEqual(len(self.app.trace.get_children()),42)
        self.assertEqual(len(self.app.bins.get_children()),16)
        with tempfile.TemporaryDirectory() as folder:
            path=Path(folder)/'result.csv'; self.app.export(path)
            self.assertIn('jayis1',path.read_text())
            self.assertIn('replay',path.read_text())
    def test_connection_and_hangup(self):
        master,slave=pty.openpty()
        try:
            self.app.connect(os.ttyname(slave))
            with self.assertRaises(ValueError): self.app.connect(os.ttyname(slave))
            data=(ROOT/'firmware/build/demo.ss1').read_bytes().splitlines(keepends=True)
            os.write(master,data[0]); self.pump()
            os.write(master,data[1]); self.pump()
            self.assertIn('TRACK',self.app.summary.get())
            self.assertTrue(self.app.session.fresh())
            os.close(master); master=None; self.pump()
            self.assertFalse(self.app.session.connected)
            self.assertIn('disconnected',self.app.session.warning)

        finally:
            if master is not None: os.close(master)
            os.close(slave)
    def test_bad_port_preserves_replay(self):
        self.app.replay(ROOT/'firmware/build/demo.ss1')
        old=self.app.session
        import termios
        with self.assertRaises((OSError,termios.error)): self.app.connect('/dev/null')
        self.assertIs(old,self.app.session)
        with self.assertRaises(OSError): self.app.connect('/no-such-sector-soak-port')
        self.assertIs(old,self.app.session)
if __name__=='__main__': unittest.main()
