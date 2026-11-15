# Author: jayis1. SPDX-License-Identifier: MIT
"""Exercise actual Tk callbacks with real C-encoded synthetic traffic."""
import os
from pathlib import Path
import pty
import sys
import tempfile
import time
import tkinter as tk
import unittest
from unittest.mock import patch
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'app'))
from workbench import Workbench

class GuiTests(unittest.TestCase):
    def setUp(self):
        self.root=tk.Tk()
        self.app=Workbench(self.root)
        self.fixture=Path(__file__).resolve().parents[1]/'firmware/build/demo.lr1'
    def tearDown(self):
        self.app.close()
    def pump(self):
        for _ in range(10): self.root.update();time.sleep(.01)
    def test_replay_and_reconnect(self):
        self.app.replay_path(self.fixture)
        self.pump()
        self.assertIn('Completed 8',self.app.summary.get())
        self.assertEqual(tuple(self.app.table.item('3')['values']),('NEAR and FAR',1,1,0))
        self.assertIn('replay',self.app.status.get())
        master,slave=pty.openpty()
        try:
            self.app.path.set(os.ttyname(slave));self.app.connect();self.pump()
            self.assertEqual(self.app.summary.get(),'No capture')
            with patch('workbench.messagebox.showerror') as error:
                self.app.connect();error.assert_called_once()
            os.write(master,b'late-fragment'+self.fixture.read_bytes())
            self.pump()
            self.assertIn('Completed 8',self.app.summary.get())
            self.assertGreater(self.app.decoder.errors,0)
            with tempfile.TemporaryDirectory() as td:
                output=Path(td)/'out.csv'
                with patch('workbench.filedialog.asksaveasfilename',return_value=str(output)):
                    self.app.export()
                self.assertIn('jayis1',output.read_text())
                self.assertIn('serial',output.read_text())
            os.close(master);master=None
            self.pump()
            self.assertIsNone(self.app.port)
            self.assertIn('BLOCKED',self.app.status.get())
        finally:
            if master is not None: os.close(master)
            os.close(slave)
    def test_bad_port_preserves_evidence(self):
        self.app.replay_path(self.fixture)
        self.app.path.set('/dev/null')
        with patch('workbench.messagebox.showerror') as error:
            self.app.connect();error.assert_called_once()
        self.assertEqual(self.app.session.latest['completed'],8)
        self.assertIsNone(self.app.port)

if __name__=='__main__': unittest.main()
