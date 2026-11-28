# Author: jayis1. SPDX-License-Identifier: MIT
import binascii
import os
from pathlib import Path
import pty
import subprocess
import sys
import tempfile
import time
import unittest
from unittest.mock import patch
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'app'))
from model import FIELDS, distance, candidates, ambiguous, witness, Session, transition
from transport import Decoder, Serial, load_replay
FIXTURE=Path(__file__).resolve().parents[1]/'firmware/build/demo.cm1'

def encode(row):
    body=('CM1,'+','.join(str(row[f]) for f in FIELDS)).encode()
    return body+f'*{binascii.crc_hqx(body,65535):04X}\n'.encode()

class ModelTests(unittest.TestCase):
    def test_all_sets(self):
        # Independent sphere intersections: populate all readings reachable by
        # zero or one flips from each code, then count overlapping memberships.
        for history in range(65536):
            words=[w for w in range(16) if history&(1<<w)]
            buckets=[[] for _ in range(16)]
            for w in words:
                buckets[w].append(w)
                for bit in range(4): buckets[w^(1<<bit)].append(w)
            expect=sum(1<<r for r in range(16) if len(buckets[r])>1)
            self.assertEqual(ambiguous(history),expect)
            minimum=min((sum(a[i]!=b[i] for i in range(4))
                for n,x in enumerate(words) for y in words[n+1:]
                for a,b in [(f'{x:04b}',f'{y:04b}')]), default=5)
            self.assertEqual(distance(history),minimum)
            for reading in range(16):
                self.assertEqual(candidates(history,reading),sorted(buckets[reading]))
    def test_boundaries(self):
        self.assertEqual(distance(0),5)
        self.assertEqual(distance(1),5)
        self.assertEqual(witness(0),None)
        self.assertEqual(witness(0x8001),(0,15))
        self.assertEqual(candidates(0x8001,3),[])
        self.assertEqual(ambiguous(0x8001),0)
        self.assertEqual(distance(0x8003),1)
        self.assertEqual(candidates(3,0),[0,1])
        for value in [-1,16,True,'0']:
            with self.assertRaises(ValueError): candidates(1,value)
    def test_real_encoder(self):
        session=load_replay(FIXTURE)
        self.assertEqual(session.total,48)
        data=subprocess.check_output([str(FIXTURE.parent/'fixture'),'--dense'])
        decoder=Decoder(); dense=Session('synthetic dense')
        for offset in range(0,len(data),4096):
            for row in decoder.feed(data[offset:offset+4096],strict=True): dense.accept(row)
        decoder.finish()
        self.assertEqual(dense.total,960)
        self.assertEqual(dense.rows[-1],session.rows[-1])
        for i,a in enumerate(session.rows):
            for b in session.rows[i+1:]: transition(a,b)
    def test_crc_valid_false_summary(self):
        row=load_replay(FIXTURE).rows[-1]
        for key,value in [('distance',5),('ambiguous',65535),('coverage',0),('raw',16),('hold',101)]:
            wrong=dict(row); wrong[key]=value
            with self.subTest(key=key),self.assertRaises(ValueError): Decoder().feed(encode(wrong),strict=True)
        data=encode(row); decoder=Decoder()
        self.assertEqual(len(decoder.feed(data[:10]+data)),1)
        self.assertEqual(decoder.errors,1)
        decoder.feed(data[:8])
        with self.assertRaises(ValueError): decoder.finish()
    def test_sparse_freeze(self):
        a=load_replay(FIXTURE).rows[-1]; b=dict(a); b['now']+=100
        with self.assertRaises(ValueError): transition(a,b)
        b['fault']=4; transition(a,b)
        b['settings']^=1
        with self.assertRaises(ValueError): transition(a,b)
    def test_replay_bounds(self):
        with tempfile.TemporaryDirectory() as tmp:
            path=Path(tmp)/'fifo'; os.mkfifo(path)
            with self.assertRaises(ValueError): load_replay(path)
            path=Path(tmp)/'file'
            for data in (b'',FIXTURE.read_bytes()+b'CM1',b'x'*(2*1024*1024+1)):
                path.write_bytes(data)
                with self.assertRaises(ValueError): load_replay(path)
    def test_serial_restore(self):
        import termios
        master,slave=pty.openpty()
        try:
            old=termios.tcgetattr(slave)
            with patch('transport.termios.tcflush',side_effect=termios.error('failure')):
                with self.assertRaises(termios.error): Serial(os.ttyname(slave))
            self.assertEqual(termios.tcgetattr(slave),old)
            port=Serial(os.ttyname(slave)); os.write(master,b'hello'); time.sleep(.02)
            self.assertEqual(port.read(),b'hello'); port.close()
            self.assertEqual(termios.tcgetattr(slave),old)
        finally: os.close(master); os.close(slave)

class GuiTests(unittest.TestCase):
    def setUp(self):
        import tkinter as tk
        from main import App
        self.root=tk.Tk(); self.root.withdraw(); self.app=App(self.root)
    def tearDown(self):
        self.app.close(); self.root.destroy()
    def test_screens_inspector_export(self):
        self.app.replay(FIXTURE); self.root.update()
        self.assertIn('minimum distance 1',self.app.summary.get())
        self.assertEqual(len(self.app.table.get_children()),16)
        self.app.reading.set('0'); self.app.inspect()
        self.assertIn('AMBIGUOUS: 0, 1',self.app.result.get())
        self.app.reading.set('6'); self.app.inspect()
        self.assertIn('outside radius: none',self.app.result.get())
        self.assertIn('not automatic correction',self.app.details.get('1.0','end'))
        with tempfile.TemporaryDirectory() as tmp:
            path=Path(tmp)/'evidence.csv'; self.app.export(path)
            self.assertEqual(path.stat().st_mode&0o777,0o600)
            self.assertIn('author,jayis1',path.read_text())
            self.assertIn('distance,ambiguous',path.read_text())
            with self.assertRaises(FileExistsError): self.app.export(path)
    def test_live_reconnect_hangup_stale(self):
        master,slave=pty.openpty()
        try:
            self.app.replay(FIXTURE); self.app.connect(os.ttyname(slave))
            self.assertEqual(self.app.summary.get(),'No acquired settings')
            with self.assertRaises(ValueError): self.app.connect(os.ttyname(slave))
            data=FIXTURE.read_bytes().splitlines(keepends=True)
            os.write(master,b''.join(data[:16])); self.app.poll(); self.root.update()
            self.assertEqual(len(self.app.session.rows),16)
            self.assertIn('minimum distance 4',self.app.summary.get())
            self.app.reading.set('1'); self.app.inspect()
            self.assertIn('unique candidate: 0',self.app.result.get())
            self.app.last_progress=time.monotonic()-3; self.app.poll()
            self.assertIsNone(self.app.serial); self.assertIn('stale',self.app.warning)
            self.app.connect(os.ttyname(slave)); self.assertFalse(self.app.session.rows)
            os.close(master); master=None; self.app.poll()
            self.assertIsNone(self.app.serial); self.assertIn('disconnect',self.app.warning)
        finally:
            if master is not None: os.close(master)
            os.close(slave)
    def test_failed_open_preserves(self):
        import termios
        self.app.replay(FIXTURE); old=self.app.session
        with self.assertRaises((OSError,termios.error)): self.app.connect('/dev/null')
        self.assertIs(self.app.session,old)
    def test_disconnect_accounts_tail(self):
        master,slave=pty.openpty()
        try:
            self.app.connect(os.ttyname(slave)); os.write(master,b'CM1,123'); self.app.poll()
            self.app.disconnect(); self.assertEqual(self.app.decoder.errors,1)
            self.app.disconnect(); self.assertEqual(self.app.decoder.errors,1)
        finally: os.close(master); os.close(slave)

if __name__=='__main__': unittest.main()
