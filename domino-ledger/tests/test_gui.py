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
from transport import Decoder

class GuiTests(unittest.TestCase):
    def setUp(self):
        self.root=tk.Tk(); self.app=Workbench(self.root)
        self.fixture=Path(__file__).resolve().parents[1]/'firmware/build/demo.dl1'
    def tearDown(self):
        self.app.close()
    def test_replay_screens_export(self):
        self.app.replay(self.fixture); self.root.update()
        self.assertIn('3',self.app.summary.get())
        self.assertIn('partial',self.app.result.get())
        for tab in self.app.tabs.tabs():
            self.app.tabs.select(tab); self.root.update()
        self.assertTrue(self.app.history.get_children())
        with tempfile.TemporaryDirectory() as directory:
            destination=Path(directory)/'result.csv'
            self.app.export(destination)
            self.assertIn('jayis1',destination.read_text())
            self.app.export(destination)
            self.assertIn('exist',self.app.message.get().lower())
    def test_real_serial_and_hangup(self):
        master,slave=pty.openpty()
        try:
            self.app.connect(os.ttyname(slave))
            frames=self.fixture.read_bytes().splitlines(keepends=True)
            os.write(master,b'late suffix'+b''.join(frames[:4]))
            for _ in range(8):
                self.root.update(); time.sleep(.02)
            self.assertTrue(self.app.session.latest)
            self.assertGreater(self.app.decoder.errors,0)
            self.app.connect(os.ttyname(slave))
            self.assertIn('Disconnect',self.app.message.get())
            os.close(master); master=None
            for _ in range(8):
                self.root.update(); time.sleep(.02)
            self.assertIsNone(self.app.port)
            self.assertIn('disconnected',self.app.message.get())
            self.assertFalse(self.app.session.fresh(False))
        finally:
            if master is not None: os.close(master)
            os.close(slave)
    def test_failed_replacement_preserves_evidence(self):
        self.app.replay(self.fixture)
        old=self.app.session
        self.app.connect('/dev/null')
        self.assertIs(self.app.session,old)
        self.assertIsNone(self.app.port)
    def test_reconnect_clears_labels(self):
        self.app.replay(self.fixture)
        master,slave=pty.openpty()
        try:
            self.app.connect(os.ttyname(slave)); self.root.update()
            self.assertIn('No accepted',self.app.summary.get())
            self.assertFalse(self.app.history.get_children())
        finally:
            self.app.disconnect(); os.close(master); os.close(slave)

if __name__=='__main__': unittest.main()
