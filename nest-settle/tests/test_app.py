# Author: jayis1. SPDX-License-Identifier: MIT
import importlib.util
import os
from pathlib import Path
import tempfile
import unittest
import binascii
import sys
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'app'))
import nest

def frame(values):
    body = 'NS1,' + ','.join(map(str, values))
    return (body + '*%04X\n' % binascii.crc_hqx(body.encode(), 0xffff)).encode()

class ProtocolTests(unittest.TestCase):
    def test_real_encoder(self):
        m = nest.Model('replay')
        m.feed((ROOT / 'firmware/build/fixture.txt').read_bytes(), 1)
        m.finish()
        self.assertEqual(len(m.rows), 1)
        self.assertEqual(m.rows[0]['drops'], 1)
        self.assertEqual(m.errors, 0)
    def test_resync_and_eof(self):
        data = (ROOT / 'firmware/build/fixture.txt').read_bytes()
        m = nest.Model('replay')
        m.feed(data[:12] + data, 1)
        self.assertEqual(len(m.rows), 1)
        m.feed(b'NS1,12', 2); m.finish()
        self.assertGreater(m.errors, 0)
    def test_invalid_and_duplicate(self):
        values = [100, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
        m = nest.Model('live'); m.feed(frame(values), 1)
        m.feed(frame(values), 5)
        self.assertFalse(m.fresh(5))
        values[2] = 15
        m.feed(frame(values), 6)
        self.assertEqual(m.errors, 1)
        values[2] = 0; values[0] = 90
        m.feed(frame(values), 7)
        self.assertTrue(m.blocked)
    def test_impossible_result(self):
        with self.assertRaises(ValueError):
            nest.decode(frame([100, 0, 0, 0, 0, 1, 1, 100, 250, 100, 100, 0, 0, 15]).strip())
    def test_host_pause_blocks_buffered_snapshots(self):
        m = nest.Model('live')
        values = [100, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
        m.feed(frame(values), 1)
        values[0] += 50
        m.feed(frame(values), 11)
        self.assertTrue(m.blocked)
        self.assertFalse(m.fresh(11))
    def test_export_metadata_even_empty(self):
        with tempfile.TemporaryDirectory() as d:
            p = Path(d) / 'out.csv'; m = nest.Model('replay'); m.errors = 2
            nest.export(m, p, 2)
            self.assertIn('jayis1', p.read_text())
            self.assertIn('replay', p.read_text())
            self.assertEqual(p.stat().st_mode & 0o777, 0o600)
            with self.assertRaises(FileExistsError): nest.export(m, p, 2)
    def test_replay_reject_fifo(self):
        with tempfile.TemporaryDirectory() as d:
            p = Path(d) / 'fifo'; os.mkfifo(p)
            with self.assertRaises(ValueError): nest.load_replay(p)

if __name__ == '__main__': unittest.main()
