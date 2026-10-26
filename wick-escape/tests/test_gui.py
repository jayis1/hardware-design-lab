# Author: jayis1. SPDX-License-Identifier: MIT
import json
import os
import pathlib
import tempfile
import termios
import time
import tkinter as tk
import unittest
from unittest.mock import patch
from test_app import wick, frame

class TestGUI(unittest.TestCase):
    def setUp(self):
        self.root=tk.Tk(); self.app=wick.App(self.root)
        self.master,self.slave=os.openpty()
        self.app.path.set(os.ttyname(self.slave))
    def pump(self):
        for _ in range(8): self.root.update(); time.sleep(.02)
    def tearDown(self):
        self.app.close()
        if self.master is not None: os.close(self.master)
        os.close(self.slave)
    def test_connection_screens_export_hangup(self):
        assert self.master is not None
        self.app.connect_button.invoke(); self.assertTrue(self.app.model.connected)
        os.write(self.master,frame()); self.pump()
        self.assertEqual(len(self.app.history.get_children()),1)
        self.assertIn('Downstream observed',self.app.detail.get())
        for page in self.app.pages: self.app.tabs.select(page); self.root.update()
        with tempfile.TemporaryDirectory() as tmp:
            out=pathlib.Path(tmp)/'evidence.json'
            with patch('tkinter.filedialog.asksaveasfilename',return_value=str(out)):
                self.app.export_dialog()
            data=json.loads(out.read_text())
            self.assertEqual(data['records'][0]['first'][1],100)
            self.assertEqual(data['author'],'jayis1')
            self.assertEqual(out.stat().st_mode&0o777,0o600)
        os.close(self.master); self.master=None; self.pump()
        self.assertFalse(self.app.model.connected)
        self.assertIn('hangup',self.app.model.warning)
        self.assertEqual(len(self.app.model.rows),1)
    def test_replay_and_reconnect_clear(self):
        with tempfile.TemporaryDirectory() as tmp:
            p=pathlib.Path(tmp)/'raw.txt'; p.write_bytes(frame())
            with patch('tkinter.filedialog.askopenfilename',return_value=str(p)):
                self.app.open_replay()
            self.assertEqual(self.app.model.provenance,'replay')
            self.app.connect_button.invoke()
            self.assertEqual(len(self.app.model.rows),0)
            self.assertEqual(self.app.detail.get(),'No coupon observations')
            self.assertEqual(self.app.bars[0]['value'],0)
    def test_setup_failure_restores_tty(self):
        before=termios.tcgetattr(self.slave)
        with patch.object(wick.termios,'tcflush',side_effect=OSError('injected')):
            with self.assertRaises(OSError): wick.Port(os.ttyname(self.slave))
        self.assertEqual(termios.tcgetattr(self.slave),before)

if __name__=='__main__': unittest.main()
