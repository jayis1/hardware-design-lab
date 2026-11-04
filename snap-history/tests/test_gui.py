# Author: jayis1. SPDX-License-Identifier: MIT
import csv
import json
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

class GuiTests(unittest.TestCase):
    def setUp(self):
        self.root=tk.Tk(); self.app=Workbench(self.root)
        self.fixture=Path(__file__).resolve().parents[1]/'firmware/build/demo.sh1'
    def tearDown(self): self.app.close()
    def pump(self,seconds=.08):
        end=time.monotonic()+seconds
        while time.monotonic()<end:
            self.root.update(); time.sleep(.003)
    def test_replay_export(self):
        self.app.replay(self.fixture); self.pump()
        self.assertEqual(len(self.app.tree.get_children()),5)
        self.assertIn('DONE',self.app.summary.get())
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'out.csv'; self.app.export(p)
            with p.open() as f: rows=list(csv.reader(f))
            meta=json.loads(rows[0][1])
            self.assertEqual(meta['author'],'jayis1')
            self.assertFalse(meta['fresh']); self.assertEqual(meta['provenance'],'replay')
            self.assertEqual(len(rows),7)
            self.assertEqual(p.stat().st_mode&0o777,0o600)
            with self.assertRaises(FileExistsError): self.app.export(p)
    def test_live_and_hangup(self):
        master,slave=pty.openpty()
        try:
            self.app.path.set(os.ttyname(slave)); self.app.connect()
            with self.assertRaises(ValueError): self.app.connect()
            # Real C encoder records through the real serial and Tk read loop.
            for line in self.fixture.read_bytes().splitlines(keepends=True):
                os.write(master,line); self.pump(.04)
            self.assertEqual(len(self.app.tree.get_children()),5)
            self.assertEqual(self.app.session.provenance,'live')
            self.assertTrue(self.app.session.fresh())
            os.close(master); master=None; self.pump()
            self.assertIsNone(self.app.port)
            self.assertFalse(self.app.session.fresh())
            self.assertEqual(len(self.app.tree.get_children()),5)
            # Failed replacement does not discard retained results.
            self.app.path.set('/dev/null')
            with self.assertRaises(Exception): self.app.connect()
            self.assertEqual(len(self.app.tree.get_children()),5)
        finally:
            if master is not None: os.close(master)
            os.close(slave)
    def test_empty_export(self):
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'empty.csv'; self.app.export(p)
            with p.open() as f: meta=json.loads(next(csv.reader(f))[1])
            self.assertIsNone(meta['snapshot'])

if __name__=='__main__': unittest.main()
