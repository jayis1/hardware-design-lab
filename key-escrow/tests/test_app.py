# Author: jayis1. SPDX-License-Identifier: MIT
import binascii
import os
from pathlib import Path
import sys
import tempfile
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'app'))
from model import Session, FIELDS, validate
from transport import Decoder, load_replay, Serial
ROOT = Path(__file__).resolve().parents[1]

def frame(row):
    body = ('KE1,' + ','.join(str(row[x]) for x in FIELDS)).encode()
    return body + ('*%04X\n' % binascii.crc_hqx(body, 65535)).encode()

class AppTests(unittest.TestCase):
    def setUp(self):
        self.data = (ROOT / 'firmware/build/demo.ke1').read_bytes()
        self.rows = Decoder().feed(self.data)
    def test_real_encoder(self):
        session = load_replay(ROOT / 'firmware/build/demo.ke1')
        self.assertEqual(session.rows[-1]['completed'], 1)
        self.assertEqual(session.rows[-1]['canceled'], 1)
        self.assertEqual(session.rows[-1]['aborted'], 1)
    def test_incremental_and_resync(self):
        d = Decoder(); rows = []
        for b in self.data: rows += d.feed(bytes([b]))
        self.assertEqual(rows, self.rows)
        d = Decoder()
        self.assertEqual(len(d.feed(b'KE1,2,3' + frame(self.rows[0]))), 1)
        self.assertEqual(d.errors, 1)
        with self.assertRaises(ValueError):
            Decoder().feed(frame(self.rows[0])[:-2] + b'X\n', strict=True)
        d = Decoder(); d.feed(b'KE1,')
        with self.assertRaises(ValueError): d.finish()
    def test_impossible(self):
        row = dict(self.rows[-1]); row['completed'] += 1
        with self.assertRaises(ValueError): Decoder().feed(frame(row), strict=True)
        row = next(dict(r) for r in self.rows if r['state'] == 2)
        row['age'] = 60000
        with self.assertRaises(ValueError): validate(row)
    def test_frozen_and_reset(self):
        s = Session('replay'); s.accept(self.rows[-1])
        row = dict(self.rows[-1]); row['now'] += 1; row['qualified'] ^= 1
        with self.assertRaises(ValueError): s.accept(row)
        s = Session('replay'); s.accept(self.rows[50])
        with self.assertRaises(ValueError): s.accept(self.rows[0])
    def test_freshness_and_export(self):
        s = Session('live'); s.connected = True
        s.accept(self.rows[0], 0)
        self.assertFalse(s.fresh(0))
        s.accept(self.rows[1], .001)
        self.assertTrue(s.fresh(.002))
        s.accept(self.rows[1], 1)
        self.assertFalse(s.fresh(1.1))
        s.connected = False
        with tempfile.TemporaryDirectory() as d:
            p = Path(d) / 'evidence.csv'; s.export(p)
            self.assertIn('jayis1', p.read_text())
            self.assertIn('live', p.read_text())
            self.assertEqual(p.stat().st_mode & 0o777, 0o600)
            with self.assertRaises(FileExistsError): s.export(p)
        s = Session('live'); s.accept(self.rows[0], 0)
        with self.assertRaises(ValueError): s.accept(self.rows[1], 3)
    def test_files_and_serial(self):
        with tempfile.TemporaryDirectory() as d:
            p = Path(d) / 'fifo'; os.mkfifo(p)
            with self.assertRaises(ValueError): load_replay(p)
            p = Path(d) / 'empty'; p.write_bytes(b'')
            with self.assertRaises(ValueError): load_replay(p)
            p.write_bytes(self.data + b'KE1,')
            with self.assertRaises(ValueError): load_replay(p)
            with p.open('wb') as handle: handle.truncate(2*1024*1024+1)
            with self.assertRaises(ValueError): load_replay(p)
        import termios
        with self.assertRaises((OSError, termios.error)): Serial('/dev/null')
        import pty, termios
        master, slave = pty.openpty(); before = termios.tcgetattr(slave)
        serial = Serial(os.ttyname(slave)); serial.close()
        self.assertEqual(before, termios.tcgetattr(slave))
        os.close(master); os.close(slave)

if __name__ == '__main__': unittest.main()
