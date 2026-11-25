# Author: jayis1. SPDX-License-Identifier: MIT
"""Exercise a real Tk event loop and synthetic serial PTY, not real hardware."""
import os
from pathlib import Path
import pty
import sys
import tempfile
import time
import tkinter as tk
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'app'))
from main import App
ROOT = Path(__file__).resolve().parents[1]
class GuiTests(unittest.TestCase):
    def setUp(self):
        self.root = tk.Tk(); self.app = App(self.root)
    def tearDown(self):
        self.app.close(); self.root.destroy()
    def pump(self,seconds=.08):
        end=time.monotonic()+seconds
        while time.monotonic()<end:
            self.root.update(); time.sleep(.005)
    def test_replay_export(self):
        self.app.replay(ROOT / 'firmware/build/demo.pc1'); self.pump()
        self.assertIn('24 / 24', self.app.summary.get())
        self.assertEqual(len(self.app.table.get_children()),6)
        self.assertIn('replay', self.app.status.get())
        with tempfile.TemporaryDirectory() as d:
            out=Path(d)/'result.csv'; self.app.export(out)
            self.assertIn('jayis1',out.read_text())
            self.assertIn('replay',out.read_text())
            with self.assertRaises(FileExistsError): self.app.export(out)
    def test_live_and_hangup(self):
        master,slave=pty.openpty()
        try:
            self.app.connect(os.ttyname(slave))
            with self.assertRaises(ValueError): self.app.connect(os.ttyname(slave))
            frames=(ROOT/'firmware/build/demo.pc1').read_bytes().splitlines(keepends=True)
            # Fragments at late attachment are diagnostics, not fabricated rows.
            os.write(master,b'junk'+frames[0]); self.pump()
            os.write(master,b''.join(frames[1:5])); self.pump()
            self.assertEqual(self.app.session.total,5)
            self.assertIn('live',self.app.status.get())
            self.assertGreater(self.app.decoder.errors,0)
            os.close(master); master=None; self.pump()
            self.assertIsNone(self.app.serial)
            self.assertEqual(self.app.session.provenance,'live serial')
            self.assertIn('disconnected',self.app.status.get())
        finally:
            if master is not None: os.close(master)
            os.close(slave)
    def test_failed_connect_preserves_evidence(self):
        self.app.replay(ROOT/'firmware/build/demo.pc1')
        before=self.app.session
        with self.assertRaises(Exception): self.app.connect('/dev/null')
        self.assertIs(self.app.session,before)
    def test_stale_and_invalid_replay(self):
        master,slave=pty.openpty()
        try:
            self.app.connect(os.ttyname(slave))
            frame=(ROOT/'firmware/build/demo.pc1').read_bytes().splitlines(keepends=True)[0]
            os.write(master,frame); self.pump()
            self.app.last_progress=time.monotonic()-3; self.pump()
            self.assertIn('stale',self.app.status.get())
            self.assertIsNone(self.app.serial)
        finally: os.close(master); os.close(slave)
        with tempfile.TemporaryDirectory() as d:
            fifo=Path(d)/'fifo'; os.mkfifo(fifo)
            with self.assertRaises(ValueError): self.app.replay(fifo)
if __name__=='__main__': unittest.main()
