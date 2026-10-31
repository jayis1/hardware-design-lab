# Author: jayis1. SPDX-License-Identifier: MIT
import binascii
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'app'))
from model import Session, FIELDS, validate
from transport import Decoder, load_replay, Serial
ROOT=Path(__file__).resolve().parents[1]
def fixture():
    return subprocess.check_output([str(ROOT/'firmware/build/test-engine'),'fixture'])
def encode(row):
    body=('CT1,'+','.join(str(row[k]) for k in FIELDS)).encode()
    return body+f'*{binascii.crc_hqx(body,65535):04X}\n'.encode()
class Tests(unittest.TestCase):
    def test_c_roundtrip(self):
        rows=Decoder().feed(fixture(),True)
        self.assertEqual(len(rows),5)
        s=Session('replay')
        for r in rows: s.accept(r)
        self.assertEqual(rows[-1]['a_delivered'],1)
        self.assertEqual(rows[-1]['b_delivered'],1)
        self.assertEqual(rows[-1]['a_last_ms'],100)
    def test_negative_crc_valid(self):
        row=Decoder().feed(fixture(),True)[-1]
        for key,value in [('a_started',3),('a_last_ms',0),('a_sum_ms',0),
                          ('a_max_ms',5000),('a_state',2),('a_timeouts',1),('fault',1)]:
            bad=dict(row); bad[key]=value
            with self.assertRaises(ValueError): Decoder().feed(encode(bad),True)
    def test_sparse_invalid(self):
        rows=Decoder().feed(fixture(),True)
        s=Session(); s.accept(rows[-1])
        bad=dict(rows[-1]); bad['now']+=100; bad['a_last_ms']-=1
        with self.assertRaises(ValueError): s.accept(bad)
        s=Session(); s.accept(rows[-1])
        bad=dict(rows[-1]); bad['now']+=100; bad['a_sum_ms']+=1; bad['a_max_ms']+=1
        with self.assertRaises(ValueError): s.accept(bad)
    def test_freshness_duplicates_and_backlog(self):
        rows=Decoder().feed(fixture(),True)
        s=Session('live'); s.connected=True
        s.accept(rows[0],0); s.accept(rows[1],.1)
        self.assertEqual(s.freshness(.2),'LIVE')
        s.accept(rows[1],2)
        self.assertEqual(s.freshness(2),'STALE')
        with self.assertRaises(ValueError): s.accept(rows[2],2)
        s=Session('live'); s.connected=True
        s.accept(rows[-1],0)
        for i in range(1,30):
            r=dict(rows[-1]); r['now']+=i
            if i<=26: s.accept(r,i*.02)
            else:
                with self.assertRaises(ValueError): s.accept(r,i*.02)
                break
    def test_sparse_advancing_impossibilities(self):
        rows=Decoder().feed(fixture(),True)
        old=rows[0]
        bad=dict(old); bad['now']+=1
        bad.update(a_started=1000000,a_returned=1000000,a_last_kind=2,a_last_ms=4999)
        s=Session(); s.accept(old)
        with self.assertRaises(ValueError): s.accept(bad)
        old=dict(rows[1]); old['a_elapsed']=500
        bad=dict(old); bad['now']+=100; bad['a_elapsed']=1
        s=Session(); s.accept(old)
        with self.assertRaises(ValueError): s.accept(bad)
        old=dict(rows[-1]); old.update(a_returned=1,a_started=2,a_last_kind=2,a_last_ms=50)
        bad=dict(old); bad['now']+=200
        bad.update(a_started=3,a_delivered=2,a_sum_ms=200)
        s=Session(); s.accept(old)
        with self.assertRaises(ValueError): s.accept(bad)
    def test_framing_replay_export(self):
        data=fixture(); d=Decoder()
        self.assertEqual(len(d.feed(b'fragment'+data[:20]+data)),5)
        self.assertGreater(d.errors,0)
        with tempfile.TemporaryDirectory() as directory:
            p=Path(directory)/'capture'; p.write_bytes(data)
            s=load_replay(p)
            out=Path(directory)/'out.csv'; s.export(out)
            self.assertIn('jayis1',out.read_text())
            self.assertEqual(out.stat().st_mode&0o777,0o600)
            with self.assertRaises(FileExistsError): s.export(out)
            p.write_bytes(data+b'CT1,')
            with self.assertRaises(ValueError): load_replay(p)
            p.write_bytes(b'')
            with self.assertRaises(ValueError): load_replay(p)
            fifo=Path(directory)/'fifo'; os.mkfifo(fifo)
            with self.assertRaises(ValueError): load_replay(fifo)
    def test_serial_failure(self):
        with self.assertRaises(Exception): Serial('/dev/null')
if __name__=='__main__': unittest.main()
