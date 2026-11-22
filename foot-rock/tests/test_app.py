# Author: jayis1. SPDX-License-Identifier: MIT
import binascii
import os
from pathlib import Path
import sys
import tempfile
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'app'))
from model import Session, validate, transition
from transport import Decoder, Serial, load_replay
FIXTURE=Path(__file__).resolve().parents[1]/'firmware/build/demo.fr1'
def encode(row):
    from model import FIELDS
    body=('FR1,'+','.join(str(row[k]) for k in FIELDS)).encode()
    return body+b'*'+f'{binascii.crc_hqx(body,65535):04X}\n'.encode()
class Tests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data=FIXTURE.read_bytes()
        cls.rows=[]; d=Decoder()
        for i in range(0,len(cls.data),4096): cls.rows+=d.feed(cls.data[i:i+4096],True)
        d.finish()
    def test_dense_and_sparse_real_encoder(self):
        for stride in (1,2,7,100,257):
            s=Session('replay')
            for r in self.rows[::stride]: s.accept(r)
            self.assertFalse(s.fresh(connected=True))
        last=self.rows[-1]
        self.assertEqual((last['diagonal'],last['adjacent'],last['seeds']),(2,1,2))
        self.assertEqual(last['settle'],1000)
    def test_all_sparse_pairs(self):
        # Hundreds of endpoint pairs, including raw disturbances and settlement.
        sampled=self.rows[::29]
        for i,a in enumerate(sampled):
            for b in sampled[i+1:]: transition(a,b)
    def test_crc_correct_impossibilities(self):
        for field,value in [('edges0',1),('diagonal',99),('anchor',0),('settle',1001),('dwell4',0)]:
            r=self.rows[-1].copy(); r[field]=value
            with self.assertRaises(ValueError): Decoder().feed(encode(r),True)
    def test_freeze_and_duplicates(self):
        r=self.rows[900].copy(); r['fault']=4
        s=Session('replay'); s.accept(r); s.accept(r.copy())
        bad=r.copy(); bad['now']+=1; bad['samples']+=1; bad['unknown']+=1
        with self.assertRaises(ValueError): s.accept(bad)
    def test_freshness_and_pause(self):
        s=Session('live')
        for i in (100,200,300): s.accept(self.rows[i],host=i/1000)
        self.assertTrue(s.fresh(host=.31,connected=True))
        self.assertFalse(s.fresh(host=.31,connected=False))
        self.assertFalse(s.fresh(host=.9,connected=True))
        with self.assertRaises(ValueError): s.accept(self.rows[400],host=3)
    def test_framing_recovery(self):
        a=encode(self.rows[0]); b=encode(self.rows[1])
        d=Decoder(); rows=d.feed(a[:13]+b)
        self.assertEqual(rows,[self.rows[1]]); self.assertEqual(d.errors,1)
        with self.assertRaises(ValueError):
            d=Decoder(); d.feed(a[:-1]); d.finish()
        d=Decoder(); rows=[]
        for byte in a: rows+=d.feed(bytes([byte]),True)
        self.assertEqual(rows,[self.rows[0]])
    def test_replay_and_serial(self):
        s=load_replay(FIXTURE); self.assertEqual(s.rows[-1],self.rows[-1])
        with tempfile.TemporaryDirectory() as tmp:
            fifo=Path(tmp)/'fifo'; os.mkfifo(fifo)
            with self.assertRaises(ValueError): load_replay(fifo)
            empty=Path(tmp)/'empty'; empty.write_bytes(b'')
            with self.assertRaises(ValueError): load_replay(empty)
        with self.assertRaises(Exception): Serial('/dev/null')
if __name__=='__main__': unittest.main()
