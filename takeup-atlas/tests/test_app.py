# Author: jayis1. SPDX-License-Identifier: MIT
import binascii
import importlib.util
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('atlas',ROOT/'app/atlas.py')
assert spec is not None and spec.loader is not None
atlas=importlib.util.module_from_spec(spec)
spec.loader.exec_module(atlas)
def frame(seq=0, stamp=100, **changes):
    row=dict(zip(atlas.FIELDS,[seq,stamp,-2,0,0,0,0,1,1,0,3,5,5,-1,1,0,0,0]))
    row.update(changes)
    body='TKA1,'+','.join(str(row[k]) for k in atlas.FIELDS)
    return (body+'*%04X\n'%binascii.crc_hqx(body.encode(),65535)).encode()
class Tests(unittest.TestCase):
    def test_c_encoder(self):
        output=subprocess.check_output([str(ROOT/'firmware/build/test-engine')])
        row=atlas.parse(output.splitlines()[0])
        self.assertEqual((row['first'],row['confirm']),(3,5))
    def test_bad_frames(self):
        for data in [frame().replace(b'TKA1',b'TKA2'),frame(first=6),frame(events=2),frame(direction=0),frame(state=2,travel=0),frame(flags=1),frame(first=-1)]:
            with self.assertRaises(ValueError): atlas.parse(data)
    def test_stream(self):
        s=atlas.Stream()
        self.assertEqual(s.feed(b'TKA1,broken'+frame()[:9]),[])
        self.assertEqual(len(s.feed(frame()[9:])),1)
        s.feed(b'x'*1000)
        self.assertLessEqual(len(s.pending),256)
    def test_session(self):
        m=atlas.Model(); m.accept(atlas.parse(frame()),1)
        m.accept(atlas.parse(frame(1,200)),2)
        self.assertTrue(m.stale(4))
        for data in [frame(2,200),frame(2,201,first=4),frame(2,300,reversals=0,events=0,first=0,confirm=0,duration=0,direction=0)]:
            with self.assertRaises(ValueError): m.accept(atlas.parse(data),3)
    def test_file_safety(self):
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'fifo'; os.mkfifo(p)
            with self.assertRaises(ValueError): atlas.read_replay(p)
            p=Path(d)/'big'; p.write_bytes(b'x'*2_000_001)
            with self.assertRaises(ValueError): atlas.read_replay(p)
    def test_export(self):
        m=atlas.Model();m.accept(atlas.parse(frame()),1)
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'out.csv'
            m.export(p,'REPLAY',False,400,'bad frame count 1')
            self.assertIn('jayis1',p.read_text())
            self.assertIn('bad frame count 1',p.read_text())
            self.assertEqual(p.stat().st_mode&0o777,0o600)
            with self.assertRaises(FileExistsError): m.export(p,'REPLAY',False,400,'')
if __name__=='__main__': unittest.main()
