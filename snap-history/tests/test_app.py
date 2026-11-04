# Author: jayis1. SPDX-License-Identifier: MIT
import binascii
import os
from pathlib import Path
import sys
import tempfile
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'app'))
from model import FIELDS, Session, validate
from transport import Decoder, Serial, load_replay

def frame(row):
    body=('SH1,'+','.join(str(row[k]) for k in FIELDS)).encode()
    return body+b'*'+f'{binascii.crc_hqx(body,65535):04X}\n'.encode()

def row():
    result=dict.fromkeys(FIELDS,0)
    result.update(now=1000,phase=2,valid=15,mask=1,raw=1,count=1,age=20,m0=1)
    return result

class ModelTests(unittest.TestCase):
    def test_frame(self):
        r=row(); encoded=frame(r); decoder=Decoder()
        self.assertEqual(decoder.feed(encoded[:15]),[])
        self.assertEqual(decoder.feed(encoded[15:],True),[r])
        decoder.finish()
    def test_resynchronization(self):
        d=Decoder(); r=row()
        self.assertEqual(d.feed(b'garbageSH1,2'+frame(r)),[r])
        self.assertEqual(d.errors,2)
        with self.assertRaises(ValueError): Decoder().feed(frame(r)[:-6]+b'*0000\n',True)
    def test_impossible(self):
        for change in [dict(count=33),dict(t0=1),dict(phase=3),dict(phase=4),
                       dict(t1=2,m1=1,count=2),dict(m2=1),dict(age=60000)]:
            r=row(); r.update(change)
            with self.assertRaises(ValueError): Decoder().feed(frame(r),True)
    def test_history(self):
        a=row()
        b=a.copy(); b.update(now=1100,age=120,count=2,t1=40,m1=0,mask=0,raw=0)
        for change,message in [(dict(m0=2),'history rewrite'),
                               (dict(count=1,t1=0,m1=0,mask=1),'counter rollback'),
                               (dict(now=1099),'timestamp rollback'),
                               (dict(age=119),'age progression')]:
            with self.subTest(change=change):
                s=Session('replay'); s.accept(a); s.accept(b)
                c=b.copy(); c.update(now=1200,age=220); c.update(change)
                validate(c)
                with self.assertRaisesRegex(ValueError,message): s.accept(c)
    def test_settlement_progress(self):
        a=row(); a.update(mask=15,m0=15,raw=14,age=300)
        b=a.copy(); b.update(now=1001,age=301,raw=15,quiet=250,phase=3)
        s=Session('replay'); s.accept(a)
        with self.assertRaisesRegex(ValueError,'settlement credit'): s.accept(b)
    def test_reject_done_deadline(self):
        r=row(); r.update(phase=3,mask=15,raw=15,m0=15,age=60000,quiet=250)
        with self.assertRaises(ValueError): Decoder().feed(frame(r),True)
    def test_duplicate(self):
        s=Session('replay'); a=row(); s.accept(a); s.accept(a.copy())
        self.assertEqual(len(s.rows),1)
        a['raw']=2
        with self.assertRaises(ValueError): s.accept(a)
    def test_freshness(self):
        s=Session('live'); a=row(); s.accept(a,10)
        self.assertFalse(s.fresh(10))
        b=a.copy(); b.update(now=1100,age=120); s.accept(b,10.1)
        self.assertTrue(s.fresh(10.1)); self.assertFalse(s.fresh(12))
        s.connected=False; self.assertFalse(s.fresh(10.1))
    def test_pause(self):
        s=Session('live'); a=row(); s.accept(a,10)
        b=a.copy(); b.update(now=1100,age=120)
        with self.assertRaises(ValueError): s.accept(b,13)
    def test_replay(self):
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'capture'; p.write_bytes(frame(row()))
            self.assertEqual(len(load_replay(p).rows),1)
            p.write_bytes(frame(row())+b'SH1,')
            with self.assertRaises(ValueError): load_replay(p)
            p.write_bytes(b'')
            with self.assertRaises(ValueError): load_replay(p)
            p.unlink(); os.mkfifo(p)
            with self.assertRaises(ValueError): load_replay(p)
    def test_serial(self):
        import pty, termios
        master,slave=pty.openpty(); original=termios.tcgetattr(slave)
        port=Serial(os.ttyname(slave)); os.write(master,b'hello')
        import time; time.sleep(.01)
        self.assertEqual(port.read(),b'hello'); port.close()
        self.assertEqual(termios.tcgetattr(slave),original)
        os.close(master); os.close(slave)
        with self.assertRaises((OSError,termios.error)): Serial('/dev/null')
    def test_fixture(self):
        p=Path(__file__).resolve().parents[1]/'firmware/build/demo.sh1'
        s=load_replay(p)
        self.assertEqual(s.rows[-1]['phase'],3)
        self.assertEqual(s.rows[-1]['count'],5)
        self.assertEqual(s.events()[-1]['occupancy'],'A B C D')
        self.assertEqual(s.events()[-1]['closed'],'A C')
        self.assertEqual(s.events()[2]['opened'],'A')

if __name__=='__main__': unittest.main()
