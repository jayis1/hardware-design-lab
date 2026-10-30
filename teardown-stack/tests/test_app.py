# Author: jayis1. SPDX-License-Identifier: MIT
import os
from pathlib import Path
import subprocess
import sys
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'app'))
from protocol import decode, encode, Session, read_replay
ROOT = Path(__file__).resolve().parents[1]
class ProtocolTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.lines = subprocess.check_output([str(ROOT/'firmware/build/test-engine'), '--fixture']).splitlines(keepends=True)
    def test_real_encoder(self):
        s = Session('replay')
        for line in self.lines:
            s.feed(line)
        s.finish()
        self.assertEqual(s.rows[-1]['completed'], 1)
        self.assertEqual(s.rows[-1]['pops'], 2)
    def test_fragment_resync_and_tail(self):
        s = Session('live')
        s.feed(b'junkTS1,0000'+self.lines[0][:20])
        s.feed(self.lines[0][20:])
        self.assertEqual(len(s.rows), 1)
        self.assertGreater(s.errors, 0)
        s.feed(b'TS1,')
        with self.assertRaises(ValueError): s.finish()
    def test_impossible(self):
        row=decode(self.lines[1]); row['depth']=4
        with self.assertRaises(ValueError): decode(encode(row))
        row=decode(self.lines[1]); row['stack1']=1
        with self.assertRaises(ValueError): decode(encode(row))
    def test_duplicate_and_rollback(self):
        s=Session('replay'); s.feed(self.lines[1]); s.feed(self.lines[1])
        self.assertEqual(len(s.rows),1)
        bad=decode(self.lines[1]); bad['tx']=1
        s.feed(encode(bad)); self.assertTrue(s.blocked)
        s=Session('replay'); s.feed(self.lines[-1]); s.feed(self.lines[0])
        self.assertTrue(s.blocked)
    def test_freshness_progress(self):
        s=Session('live'); s.connected=True
        s.feed(self.lines[0],100.0)
        self.assertFalse(s.fresh(100.0))
        s.feed(self.lines[1],100.012)
        self.assertTrue(s.fresh(100.1))
        s.feed(self.lines[1],101.0)
        self.assertFalse(s.fresh(101.0))
        s.feed(self.lines[2],103.0)
        self.assertTrue(s.blocked)
    def test_empty_and_fifo(self):
        with self.assertRaises(ValueError): Session('replay').finish()
        import tempfile
        with tempfile.TemporaryDirectory() as d:
            fifo=Path(d)/'fifo'; os.mkfifo(fifo)
            with self.assertRaises(ValueError): read_replay(fifo)
    def test_crc_and_bounded_buffer(self):
        with self.assertRaises(ValueError): decode(self.lines[0].replace(b'TS1',b'TS2'))
        s=Session('replay'); s.feed(b'x'*100000)
        self.assertLess(len(s.buffer),256)
        self.assertGreater(s.errors,0)
if __name__=='__main__': unittest.main()
