# Author: jayis1. SPDX-License-Identifier: MIT
import binascii
import itertools
import os
from pathlib import Path
import sys
import tempfile
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'app'))
from model import FIELDS, Session, validate, word, analyse
from transport import Decoder, Serial, load_replay

def row(codes=(),now=100):
    r=dict.fromkeys(FIELDS,0)
    r.update(now=now,state=1,valid=15,levels=1,accepted=len(codes),starts=len(codes))
    for i,c in enumerate(codes): r['word'+str(i//8)]|=c<<(4*(i%8))
    stack,p,pos,neg=analyse(codes)
    r.update(depth=len(stack),cancelled=(len(codes)-len(stack))//2,
             permutation=p,positive=pos,negative=neg)
    return r

def encode(r):
    body=('BT1,'+','.join(str(r[k]) for k in FIELDS)).encode()
    return body+b'*'+f'{binascii.crc_hqx(body,65535):04X}'.encode()+b'\n'

class ModelTests(unittest.TestCase):
    def test_words(self):
        for n in range(6):
            for codes in itertools.product(range(1,5),repeat=n):
                r=row(codes); validate(r)
                self.assertEqual(word(r),list(codes))
        self.assertEqual(analyse([1,3])[0],[])
        self.assertEqual(analyse([1,1])[1],21)
        self.assertNotEqual(analyse([1,1])[0],[])
    def test_crc_valid_impossible(self):
        for field,value in [('depth',1),('permutation',0),('word0',5),('positive',3),('starts',3)]:
            r=row([1,2]); r[field]=value
            with self.assertRaises(ValueError): Decoder().feed(encode(r),strict=True)
    def test_prefix_and_halt(self):
        s=Session('replay'); s.accept(row([1])); s.accept(row([1,2],200))
        with self.assertRaises(ValueError): s.accept(row([2,1,2],300))
        r=row([1]); r.update(state=3,faults=8,gaps=1)
        s=Session('replay'); s.accept(r)
        bad=row([1,2],200); bad.update(state=3,faults=8,gaps=1)
        with self.assertRaises(ValueError): s.accept(bad)
    def test_active_capture_and_taint(self):
        r=row(); r.update(state=2,levels=5,starts=1,selected=1,outcome=4,age=10)
        s=Session('replay'); s.accept(r)
        with self.assertRaises(ValueError): s.accept(row([2],200))
        r['tainted']=1; s=Session('replay'); s.accept(r)
        with self.assertRaises(ValueError): s.accept(row([1],200))
    def test_duplicate_and_stale(self):
        # A completion cannot materialize on the first sample after READY.
        s=Session('replay'); s.accept(row())
        with self.assertRaises(ValueError): s.accept(row([1],101))
        # Release at the rejecting deadline cannot turn ACTIVE into accepted.
        r=row(); r.update(state=2,levels=5,starts=1,selected=1,outcome=4,age=9999)
        s=Session('replay'); s.accept(r)
        final=row([1],101); final['age']=10000
        with self.assertRaises(ValueError): s.accept(final)
        # A prior almost-qualified release can complete one tick earlier.
        r['age']=9998; s=Session('replay'); s.accept(r)
        final['age']=9999; s.accept(final)
        s=Session('replay'); s.accept(row()); s.accept(row([1],106))
        s=Session('live'); s.accept(row(),0); s.accept(row(now=200),.1)
        self.assertTrue(s.fresh(.2)); s.accept(row(now=200),1)
        self.assertFalse(s.fresh(2.2))
        with self.assertRaises(ValueError): s.accept(row(now=300),3)
        s=Session('replay'); s.accept(row())
        with self.assertRaises(ValueError): s.accept(row([1]))
    def test_drift(self):
        s=Session('live'); s.accept(row(),0)
        with self.assertRaises(ValueError):
            for n in range(1,100): s.accept(row(now=100+10*n),n*.2)
    def test_stream_and_replay(self):
        frame=encode(row([1,2,4]))
        d=Decoder(); self.assertEqual(d.feed(frame[:12]),[])
        self.assertEqual(len(d.feed(frame[12:])),1); d.finish()
        d=Decoder(); self.assertEqual(len(d.feed(frame[:12]+frame)),1); self.assertEqual(d.errors,1)
        with tempfile.TemporaryDirectory() as tmp:
            path=Path(tmp)/'trial.bt1'; path.write_bytes(frame)
            self.assertEqual(load_replay(path).rows[-1]['accepted'],3)
            path.write_bytes(frame+frame[:12])
            with self.assertRaises(ValueError): load_replay(path)
            path.write_bytes(b'')
            with self.assertRaises(ValueError): load_replay(path)
            path.unlink(); os.mkfifo(path)
            with self.assertRaises(ValueError): load_replay(path)
    def test_export(self):
        with tempfile.TemporaryDirectory() as tmp:
            p=Path(tmp)/'out.csv'; s=Session('replay'); s.accept(row([1,3])); s.export(p,'=danger')
            self.assertEqual(p.stat().st_mode&0o777,0o600)
            self.assertIn('jayis1',p.read_text()); self.assertIn("'=danger",p.read_text())
            with self.assertRaises(FileExistsError): s.export(p,'test')
    def test_serial_restore(self):
        import pty,termios
        m,slave=pty.openpty(); original=termios.tcgetattr(slave)
        serial=Serial(os.ttyname(slave)); os.write(m,b'hello')
        import time; time.sleep(.01)
        self.assertEqual(serial.read(),b'hello'); serial.close()
        self.assertEqual(termios.tcgetattr(slave),original)
        os.close(m); os.close(slave)
        with self.assertRaises((OSError,termios.error)): Serial('/dev/null')

if __name__=='__main__': unittest.main()
