# Author: jayis1. SPDX-License-Identifier: MIT
import binascii
import os
from pathlib import Path
import sys
import tempfile
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'app'))
from model import FIELDS, Session, validate
from transport import Decoder, load_replay, Serial

def row(t=100):
    r=dict.fromkeys(FIELDS,0); r['now']=t; r['state']=1
    return r

def frame(r):
    b=('PS1,'+','.join(str(r[f]) for f in FIELDS)).encode()
    return b+b'*'+f'{binascii.crc_hqx(b,65535):04X}'.encode()+b'\n'

class Tests(unittest.TestCase):
    def test_wire(self):
        d=Decoder(); r=row(); self.assertEqual(d.feed(frame(r)),[r])
        self.assertEqual(d.feed(b'PS1,broken'+frame(r)),[r]); self.assertEqual(d.errors,1)
        with self.assertRaises(ValueError): d.feed(frame(r)[:-2]+b'0\n',True)
    def test_ranges(self):
        for key,value in [('raw',16),('state',4),('fault',10),('bad',1),('ref0',8),('latest0',1),('errors0',1)]:
            r=row(); r[key]=value
            with self.subTest(key=key), self.assertRaises(ValueError): validate(r)
    def test_progress(self):
        s=Session('live'); s.accept(row(100),0)
        self.assertFalse(s.fresh(0))
        s.accept(row(200),.1); self.assertTrue(s.fresh(.2))
        s.accept(row(200),.3); self.assertFalse(s.fresh(2))
        with self.assertRaises(ValueError): s.accept(row(201),10)
        self.assertTrue(s.blocked)
    def test_mutation(self):
        s=Session('replay'); a=row(); a['words']=16
        s.accept(a); b=a.copy(); b['now']=200; b['ref0']=1
        with self.assertRaises(ValueError): s.accept(b)
    def test_pulse_transitions(self):
        s=Session('replay'); a=row(); a.update(state=2,raw=1,high_ms=900)
        s.accept(a); b=a.copy(); b['now']=300
        with self.assertRaises(ValueError): s.accept(b)
        s=Session('replay'); s.accept(row())
        b=row(101); b['words']=1
        with self.assertRaises(ValueError): s.accept(b)
        s=Session('replay'); a=row(); a.update(state=2,raw=1,high_ms=20)
        s.accept(a); b=a.copy(); b.update(now=110,high_ms=30)
        s.accept(b)
        c=b.copy(); c.update(now=111,high_ms=31,raw=3)
        with self.assertRaises(ValueError): s.accept(c)
    def test_unchanged_slot(self):
        s=Session('replay'); a=row(); a['words']=16; s.accept(a)
        b=a.copy(); b['now']=200; b['words']=17; b['latest1']=1
        with self.assertRaises(ValueError): s.accept(b)
    def test_halt_freeze(self):
        s=Session('replay'); a=row(); a.update(state=3,fault=3); s.accept(a)
        b=a.copy(); b.update(now=200,words=1)
        with self.assertRaises(ValueError): s.accept(b)
    def test_replay(self):
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'capture'; p.write_bytes(frame(row()))
            self.assertEqual(len(load_replay(p).rows),1)
            p.write_bytes(frame(row())+b'PS1,')
            with self.assertRaises(ValueError): load_replay(p)
            p.unlink(); os.mkfifo(p)
            with self.assertRaises(ValueError): load_replay(p)
    def test_export(self):
        with tempfile.TemporaryDirectory() as d:
            s=Session('replay'); s.accept(row()); s.errors=2
            p=Path(d)/'export.csv'; s.export(p,0)
            self.assertEqual(p.stat().st_mode&0o777,0o600)
            self.assertIn('jayis1',p.read_text()); self.assertIn('replay',p.read_text())
            with self.assertRaises(FileExistsError): s.export(p,0)
    def test_serial(self):
        import pty,termios
        m,s=pty.openpty(); before=termios.tcgetattr(s)
        port=Serial(os.ttyname(s)); port.close()
        self.assertEqual(termios.tcgetattr(s),before)
        os.close(m); os.close(s)
        with self.assertRaises((OSError,termios.error)): Serial('/dev/null')

if __name__=='__main__': unittest.main()
