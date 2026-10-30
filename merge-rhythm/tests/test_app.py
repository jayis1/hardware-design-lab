# Author: jayis1. SPDX-License-Identifier: MIT
import binascii
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'app'))
from protocol import FIELDS, Stream, Session, decode, replay
ROOT = Path(__file__).resolve().parents[1]
def frame(row):
    body = ('MR1,' + ','.join(str(row[x]) for x in FIELDS)).encode()
    return body + f'*{binascii.crc_hqx(body,65535):04X}\n'.encode()
class ProtocolTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data = subprocess.check_output([str(ROOT/'firmware/build/test-engine'), '--fixture'])
        cls.rows = Stream().feed(cls.data)
    def test_real_encoder(self):
        self.assertEqual(len(self.rows), 8)
        self.assertEqual(self.rows[-1]['left']+self.rows[-1]['right'],8)
    def test_fragments(self):
        s=Stream(); result=[]
        for byte in self.data: result += s.feed(bytes([byte]))
        self.assertEqual(result,self.rows); self.assertEqual(s.errors,0)
        s=Stream(); self.assertEqual(s.feed(b'noiseMR1,broken'+self.data),self.rows)
        self.assertGreater(s.errors,0)
    def test_bad_crc_and_invariants(self):
        with self.assertRaises(ValueError): decode(self.data.splitlines()[0]+b'00')
        for key,value in [('pending',9),('left',200),('state',9),('latency_sum',0),('fault',1)]:
            r=self.rows[0].copy(); r[key]=value
            with self.assertRaises(ValueError): decode(frame(r).strip())
    def test_session(self):
        s=Session('live'); s.accept(self.rows[0],10); self.assertFalse(s.fresh(10))
        delta=(self.rows[1]['now']-self.rows[0]['now'])/1000
        s.accept(self.rows[1],10+delta); self.assertTrue(s.fresh(10+delta))
        s.connected=False; self.assertFalse(s.fresh(10+delta))
        self.assertEqual(s.provenance,'live')
    def test_duplicate_and_reset(self):
        s=Session('replay'); s.accept(self.rows[0],0); s.accept(self.rows[0],1)
        self.assertEqual(len(s.rows),1)
        r=self.rows[0].copy(); r['tx']=1
        with self.assertRaises(ValueError): s.accept(r,2)
        s=Session('replay'); s.accept(self.rows[1],0)
        with self.assertRaises(ValueError): s.accept(self.rows[0],1)
    def test_backlog(self):
        s=Session('live'); s.accept(self.rows[0],0)
        with self.assertRaises(ValueError): s.accept(self.rows[1],10)
    def test_impossible_progress(self):
        for key in ('alternations','longest'):
            p=self.rows[-1].copy(); r=p.copy(); r['now']+=100; r[key]+=1
            s=Session('replay'); s.accept(p,0)
            with self.assertRaises(ValueError): s.accept(r,0)
        p=self.rows[-1].copy(); p['state']=0; p['streak']=0
        r=p.copy(); r['now']+=100; r['state']=1
        s=Session('replay'); s.accept(p,0)
        with self.assertRaises(ValueError): s.accept(r,0)
    def test_wrap_stale_and_terminal(self):
        p=self.rows[0].copy(); p['now']=0xfffffff0
        r=p.copy(); r['now']=16
        s=Session('live'); s.accept(p,0); s.accept(r,.032)
        self.assertTrue(s.fresh(.032)); s.accept(r,2)
        self.assertFalse(s.fresh(2))
        p=self.rows[0].copy(); p.update(state=2,streak=0,fault=1)
        for changes in ({'fault':0,'state':0},{'mask':1}):
            r=p.copy(); r.update(changes); r['now']+=100
            s=Session('replay'); s.accept(p,0)
            with self.assertRaises(ValueError): s.accept(r,0)
    def test_replay_limits(self):
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'capture'; p.write_bytes(self.data)
            self.assertEqual(len(replay(p).rows),8)
            for data in [b'',self.data+b'MR1,',self.data+b'junk']:
                p.write_bytes(data)
                with self.assertRaises(ValueError): replay(p)
            p.unlink(); os.mkfifo(p)
            with self.assertRaises(ValueError): replay(p)
if __name__=='__main__': unittest.main()
