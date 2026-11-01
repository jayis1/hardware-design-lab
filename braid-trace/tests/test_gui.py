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
from test_app import row,encode
ROOT=Path(__file__).resolve().parents[1]

class GuiTests(unittest.TestCase):
    def setUp(self):
        self.root=tk.Tk(); self.app=Workbench(self.root); self.root.update()
    def tearDown(self): self.app.close()
    def pump(self,seconds=.08):
        end=time.monotonic()+seconds
        while time.monotonic()<end: self.root.update(); time.sleep(.005)
    def test_encoder_replay_screens_export(self):
        self.app.replay_path(ROOT/'firmware/build/demo.bt1')
        self.root.update()
        self.assertIn('6 / 32',self.app.summary.get())
        self.assertIn('A B C',self.app.summary.get())
        self.assertIn('NOT proof',self.app.summary.get())
        self.assertEqual(len(self.app.diagram.find_withtag('crossing')),6)
        for i in range(4): self.app.tabs.select(i); self.root.update()
        self.assertEqual(len(self.app.history.get_children()),7)
        with tempfile.TemporaryDirectory() as tmp:
            p=Path(tmp)/'out.csv'; self.app.export_to(p)
            self.assertIn('replay',p.read_text())
            with self.assertRaises(FileExistsError): self.app.export_to(p)
        self.app.disconnect(); self.assertEqual(self.app.session.provenance,'replay')
    def test_live_read_hangup_and_reconnect(self):
        master,slave=pty.openpty()
        try:
            self.app.path.set(os.ttyname(slave)); self.app.connect()
            owner=self.app.port; self.app.connect(); self.assertIs(self.app.port,owner)
            os.write(master,encode(row(now=100))); self.pump(.1)
            os.write(master,encode(row([1],now=200))); self.pump(.1)
            self.assertTrue(self.app.session.fresh())
            self.assertEqual(len(self.app.diagram.find_withtag('crossing')),1)
            os.close(master); master=None; self.pump()
            self.assertIsNone(self.app.port); self.assertFalse(self.app.session.connected)
            self.assertEqual(self.app.session.provenance,'live')
            self.app.path.set('/dev/null'); self.app.connect()
            self.assertIn('failed',self.app.status.get())
            self.assertEqual(self.app.session.rows[-1]['accepted'],1)
        finally:
            if master is not None: os.close(master)
            os.close(slave)
    def test_replacement_clears_visuals(self):
        self.app.replay_path(ROOT/'firmware/build/demo.bt1')
        master,slave=pty.openpty()
        try:
            self.app.path.set(os.ttyname(slave)); self.app.connect()
            self.assertEqual(self.app.summary.get(),'No samples')
            self.assertEqual(len(self.app.diagram.find_all()),0)
        finally: self.app.disconnect(); os.close(master); os.close(slave)

if __name__=='__main__': unittest.main()
