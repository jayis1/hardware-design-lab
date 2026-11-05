# Author: jayis1. SPDX-License-Identifier: MIT
import binascii
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
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'app'))
from model import Session, validate
from transport import Decoder, load_replay
from workbench import Workbench

def encode(row):
    from model import FIELDS
    body=('FR1,'+','.join(str(row[k]) for k in FIELDS)).encode()
    return body+f'*{binascii.crc_hqx(body,65535):04X}\n'.encode()

class ModelTests(unittest.TestCase):
    def setUp(self):
        self.data=(ROOT/'firmware/build/demo.fr1').read_bytes()
        self.rows=Decoder().feed(self.data,strict=True)
    def test_real_encoder(self):
        s=load_replay(ROOT/'firmware/build/demo.fr1')
        self.assertEqual(s.rows[-1]['phase'],3)
        self.assertEqual(s.rows[-1]['reopens0'],1)
        self.assertEqual(len(s.events()),3)
    def test_bad_frames(self):
        for change in [dict(first0=0),dict(quiet0=251),dict(age=10000),
                       dict(closures0=0),dict(first1=9999),dict(mask=3)]:
            r=self.rows[-1]|change
            with self.subTest(change=change),self.assertRaises(ValueError):
                Decoder().feed(encode(r),strict=True)
    def test_frozen_and_first(self):
        s=Session('replay'); s.accept(self.rows[-1])
        with self.assertRaises(ValueError):
            s.accept(self.rows[-1]|dict(now=self.rows[-1]['now']+1,last0=16))
        s=Session('replay'); s.accept(self.rows[2])
        r=self.rows[4]|dict(first0=6)
        with self.assertRaises(ValueError): s.accept(r)
    def test_framing(self):
        d=Decoder(); result=d.feed(b'fragment'+self.data[:20]+self.data)
        self.assertEqual(result[-1],self.rows[-1]); self.assertGreater(d.errors,0)
        d=Decoder(); d.feed(self.data+b'FR1,')
        with self.assertRaises(ValueError): d.finish()
    def test_live_age(self):
        s=Session('live'); s.accept(self.rows[0],host=0)
        s.accept(self.rows[1],host=.005)
        self.assertTrue(s.fresh(.01))
        s.accept(self.rows[1],host=5)
        self.assertFalse(s.fresh(5))
        with self.assertRaises(ValueError): s.accept(self.rows[2],host=5)
    def test_replay_reject(self):
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'bad'; p.write_bytes(self.data+b'FR1,')
            with self.assertRaises(ValueError): load_replay(p)
            p.unlink(); os.mkfifo(p)
            with self.assertRaises(ValueError): load_replay(p)

class GuiTests(unittest.TestCase):
    def setUp(self):
        self.root=tk.Tk(); self.app=Workbench(self.root)
        self.fixture=ROOT/'firmware/build/demo.fr1'
    def tearDown(self): self.app.close()
    def pump(self,seconds=.06):
        end=time.monotonic()+seconds
        while time.monotonic()<end:
            self.root.update(); time.sleep(.003)
    def test_replay_export(self):
        self.app.replay(self.fixture); self.pump()
        self.assertEqual(len(self.app.tree.get_children()),3)
        self.assertIn('DONE',self.app.summary.get())
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'out.csv'; self.app.export(p)
            with p.open() as f: rows=list(csv.reader(f))
            meta=json.loads(rows[0][1]); self.assertEqual(meta['author'],'jayis1')
            self.assertFalse(meta['fresh']); self.assertEqual(meta['provenance'],'replay')
            self.assertEqual(len(rows),5)
            self.assertEqual(p.stat().st_mode&0o777,0o600)
            with self.assertRaises(FileExistsError): self.app.export(p)
    def test_live_hangup(self):
        master,slave=pty.openpty()
        try:
            self.app.path.set(os.ttyname(slave)); self.app.connect()
            with self.assertRaises(ValueError): self.app.connect()
            for line in self.fixture.read_bytes().splitlines(keepends=True):
                os.write(master,line); self.pump(.04)
            self.assertEqual(len(self.app.tree.get_children()),3)
            self.assertTrue(self.app.session.fresh())
            os.close(master); master=None; self.pump()
            self.assertIsNone(self.app.port)
            self.assertFalse(self.app.session.fresh())
            self.app.path.set('/dev/null')
            with self.assertRaises(Exception): self.app.connect()
            self.assertEqual(len(self.app.tree.get_children()),3)
        finally:
            if master is not None: os.close(master)
            os.close(slave)
    def test_empty_export(self):
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'empty.csv'; self.app.export(p)
            with p.open() as f: meta=json.loads(next(csv.reader(f))[1])
            self.assertIsNone(meta['snapshot'])

if __name__=='__main__': unittest.main()
