# Author: jayis1. SPDX-License-Identifier: MIT
import binascii
import os
from pathlib import Path
import pty
import sys
import tempfile
import time
import unittest
from unittest.mock import patch
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'app'))
from model import project, subsets, minimal, witness, suggest, validate, Session, transition
from transport import Decoder, Serial, load_replay
FIXTURE=Path(__file__).resolve().parents[1]/'firmware/build/demo.mk1'

def encode(row):
    from model import FIELDS
    body=('MK1,'+','.join(str(row[f]) for f in FIELDS)).encode()
    return body+f'*{binascii.crc_hqx(body,65535):04X}\n'.encode()

class ModelTests(unittest.TestCase):
    def test_all_relations(self):
        for settings in range(65536):
            expected=0
            for mask in range(8):
                buckets=[set() for _ in range(8)]
                for w in range(16):
                    if settings&(1<<w): buckets[w&mask].add(w>>3)
                good=all(len(bucket)<2 for bucket in buckets)
                if good: expected|=1<<mask
                pair=witness(settings,mask)
                self.assertEqual(pair is None,good)
                if pair is not None:
                    a,b=pair
                    self.assertNotEqual(a>>3,b>>3)
                    self.assertEqual(a&mask,b&mask)
                    self.assertTrue(settings&(1<<a) and settings&(1<<b))
            self.assertEqual(subsets(settings),expected)
            want=sum(1<<m for m in range(8) if expected&(1<<m) and not any(
                s!=m and s&m==s and expected&(1<<s) for s in range(8)))
            self.assertEqual(minimal(settings),want)
    def test_real_encoder(self):
        session=load_replay(FIXTURE)
        self.assertEqual(session.total,72)
        final=session.rows[-1]
        self.assertEqual(final['compatible'],0)
        self.assertEqual(final['minimal'],0)
        self.assertEqual(suggest(final['coverage'],final['raw']),8)
        xor=next(r for r in session.rows if r['visits']==8)
        self.assertEqual(xor['compatible'],(1<<3)|(1<<7))
        self.assertEqual(xor['minimal'],1<<3)
    def test_crc_fragment_and_tail(self):
        raw=FIXTURE.read_bytes().splitlines(keepends=True)[0]
        d=Decoder(); self.assertEqual(len(d.feed(raw[:13]+raw)),1)
        self.assertEqual(d.errors,1)
        self.assertEqual(d.feed(raw.replace(b'MK1,',b'MK1,9',1)),[])
        d.feed(raw[:8])
        with self.assertRaises(ValueError): d.finish()
    def test_impossible_valid_crc(self):
        row=load_replay(FIXTURE).rows[-1]
        for key,value in [('coverage',0),('suggestion',0),('raw',16),('hold',101),
                          ('visits',0),('compatible',255),('minimal',128)]:
            wrong=dict(row); wrong[key]=value
            with self.subTest(key=key), self.assertRaises(ValueError):
                Decoder().feed(encode(wrong),strict=True)
    def test_rollback_and_disappearing_evidence(self):
        a=dict(load_replay(FIXTURE).rows[-1]); b=dict(a)
        b['now']+=1; b['samples']+=1; b['qualified_ms']+=1
        transition(a,b)
        b['settings']=0; b['coverage']=0
        with self.assertRaises(ValueError): transition(a,b)
        b=dict(a); b['now']-=1
        with self.assertRaises(ValueError): transition(a,b)
    def test_replay_bounds_and_special_files(self):
        with tempfile.TemporaryDirectory() as tmp:
            path=Path(tmp)/'fifo'; os.mkfifo(path)
            with self.assertRaises(ValueError): load_replay(path)
            path=Path(tmp)/'empty'; path.write_bytes(b'')
            with self.assertRaises(ValueError): load_replay(path)
            path.write_bytes(FIXTURE.read_bytes()+b'MK1')
            with self.assertRaises(ValueError): load_replay(path)
            path.write_bytes(b'X'*(2*1024*1024+1))
            with self.assertRaises(ValueError): load_replay(path)
    def test_serial_restore(self):
        import termios
        master,slave=pty.openpty()
        try:
            old=termios.tcgetattr(slave)
            port=Serial(os.ttyname(slave)); os.write(master,b'hello')
            time.sleep(.02); self.assertEqual(port.read(),b'hello')
            port.close(); self.assertEqual(termios.tcgetattr(slave),old)
        finally: os.close(master); os.close(slave)

class GuiTests(unittest.TestCase):
    def setUp(self):
        import tkinter as tk
        from main import App
        self.root=tk.Tk(); self.root.withdraw(); self.app=App(self.root)
    def tearDown(self):
        self.app.close(); self.root.destroy()
    def test_replay_screens_and_export(self):
        self.app.replay(FIXTURE); self.root.update()
        self.assertIn('0 compatible',self.app.summary.get())
        rows=[self.app.table.item(i)['values'] for i in self.app.table.get_children()]
        self.assertEqual(len(rows),8)
        self.assertIn('REFUTED',str(rows[0]))
        self.assertIn('0 / 8',str(rows[0]))
        with tempfile.TemporaryDirectory() as tmp:
            path=Path(tmp)/'evidence.csv'; self.app.export(path)
            self.assertEqual(path.stat().st_mode & 0o777,0o600)
            self.assertIn('author,jayis1',path.read_text())
            self.assertIn('replay',path.read_text())
            self.assertIn('compatible,minimal',path.read_text())
            with self.assertRaises(FileExistsError): self.app.export(path)
    def test_xor_screen(self):
        rows=load_replay(FIXTURE).rows
        self.app.session=Session('synthetic test')
        self.app.session.accept(next(r for r in rows if r['visits']==8))
        self.app.render(); self.root.update()
        self.assertIn('A+B',self.app.summary.get())
        self.assertIn('2 compatible',self.app.summary.get())
    def test_live_reconnect_hangup(self):
        master,slave=pty.openpty()
        try:
            self.app.replay(FIXTURE)
            self.app.connect(os.ttyname(slave))
            self.assertEqual(self.app.summary.get(),'No acquired settings')
            with self.assertRaises(ValueError): self.app.connect(os.ttyname(slave))
            data=FIXTURE.read_bytes().splitlines(keepends=True)
            os.write(master,b''.join(data[:5])); self.app.poll()
            self.assertEqual(len(self.app.session.rows),5)
            self.app.last_progress=time.monotonic()-3
            self.app.poll(); self.assertIsNone(self.app.serial)
            self.assertIn('stale',self.app.warning)
            self.app.connect(os.ttyname(slave))
            self.assertFalse(self.app.session.rows)
            os.close(master); master=None
            self.app.poll(); self.assertIsNone(self.app.serial)
            self.assertIn('disconnect',self.app.warning)
        finally:
            if master is not None: os.close(master)
            os.close(slave)
    def test_failed_open_preserves_evidence(self):
        import termios
        self.app.replay(FIXTURE); old=self.app.session
        with self.assertRaises((OSError,termios.error)): self.app.connect('/dev/null')
        self.assertIs(self.app.session,old)
        with patch('transport.os.open',side_effect=PermissionError('denied')):
            with self.assertRaises(PermissionError): self.app.connect('/dev/ttyACM0')
        self.assertIs(self.app.session,old)

if __name__=='__main__': unittest.main()
