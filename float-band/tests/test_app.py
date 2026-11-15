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

def frame(row):
    body=('FB1,'+','.join(str(row[k]) for k in FIELDS)).encode()
    return body+b'*'+f'{binascii.crc_hqx(body,65535):04X}\n'.encode()

def idle():
    row=dict.fromkeys(FIELDS,0)
    row.update(sequence=10,now=10,ref_valid=1,dut_valid=1,band=1)
    return row

class ModelTest(unittest.TestCase):
    def test_first_halt_has_no_acquisition(self):
        s=Session('replay'); r=idle(); s.accept(r)
        with self.assertRaises(ValueError):
            s.accept(dict(r,now=12,fault=4,gaps=1,raw=7,band=3))
    def test_diagnostic_growth(self):
        for field in ('noise','skips','reversals'):
            s=Session('replay'); r=idle(); s.accept(r)
            q=dict(r,sequence=11,now=11); q[field]=10
            with self.assertRaises(ValueError): s.accept(q)
    def test_startup_qualification(self):
        r=idle(); r.update(sequence=5,now=5,edges=1,close1=1,dut=1)
        with self.assertRaises(ValueError): validate(r)
        r=idle(); r.update(sequence=4,now=4)
        with self.assertRaises(ValueError): validate(r)
    def test_conservation(self):
        r=idle(); validate(r)
        r['edges']=1
        with self.assertRaises(ValueError): validate(r)
    def test_monotonic_bins(self):
        s=Session('replay'); r=idle(); r.update(edges=1,close1=1,dut=1)
        s.accept(r)
        q=dict(r,now=20,sequence=20,close1=0,close2=1)
        with self.assertRaises(ValueError): s.accept(q)
    def test_halt_freezes(self):
        s=Session('replay'); r=idle(); r['fault']=4; r['gaps']=1
        s.accept(r)
        with self.assertRaises(ValueError): s.accept(dict(r,now=20,sequence=11))
    def test_duplicate_conflict(self):
        s=Session('replay'); r=idle(); s.accept(r); s.accept(r)
        with self.assertRaises(ValueError): s.accept(dict(r,raw=3))
    def test_sparse_edge_polarity(self):
        s=Session('replay'); r=idle(); s.accept(r)
        q=dict(r,now=20,sequence=20,edges=1,open1=1)
        with self.assertRaises(ValueError): s.accept(q)
    def test_transport(self):
        r=idle(); f=frame(r); d=Decoder()
        self.assertEqual(d.feed(f[:12]),[])
        self.assertEqual(d.feed(f[12:]),[r]); d.finish()
        d=Decoder(); self.assertEqual(d.feed(f[:12]+f),[r]); self.assertEqual(d.errors,1)
        with self.assertRaises(ValueError): Decoder().feed(f.replace(b'FB1,10',b'FB1,11'),True)
    def test_fixture(self):
        path=Path(__file__).resolve().parents[1]/'firmware/build/demo.fb1'
        s=load_replay(path)
        self.assertEqual(s.last['edges'],4)
        self.assertEqual(s.last['close1'],1)
        self.assertEqual(s.last['open2'],1)
        self.assertEqual(s.last['close4'],1)
        self.assertEqual(s.last['open1'],1)
        with tempfile.TemporaryDirectory() as directory:
            p=Path(directory)/'report.csv'; s.export(p)
            self.assertIn('jayis1',p.read_text())
            self.assertIn('replay',p.read_text())
            with self.assertRaises(FileExistsError): s.export(p)
            p=Path(directory)/'partial'; p.write_bytes(path.read_bytes()+b'FB1,')
            with self.assertRaises(ValueError): load_replay(p)
    def test_serial(self):
        import pty, termios
        a,b=pty.openpty(); before=termios.tcgetattr(b)
        port=Serial(os.ttyname(b)); os.write(a,frame(idle()))
        import time; time.sleep(.02)
        self.assertEqual(Decoder().feed(port.read()),[idle()])
        port.close(); self.assertEqual(termios.tcgetattr(b),before)
        os.close(a); os.close(b)
        with self.assertRaises((OSError,termios.error)): Serial('/dev/null')
if __name__=='__main__': unittest.main()
