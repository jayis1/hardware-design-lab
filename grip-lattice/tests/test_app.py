# Author: jayis1. SPDX-License-Identifier: MIT
import binascii
import os
from pathlib import Path
import sys
import tempfile
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'app'))
from model import FIELDS, Session, validate
from transport import Decoder, Serial, load_replay
ROOT = Path(__file__).resolve().parents[1]

def data():
    return (ROOT / 'firmware/build/demo.gl1').read_bytes()

def frame(row):
    body = ('GL1,' + ','.join(str(row[k]) for k in FIELDS)).encode()
    return body + f'*{binascii.crc_hqx(body,65535):04X}\n'.encode()

class ModelTests(unittest.TestCase):
    def test_real_encoder(self):
        rows = Decoder().feed(data(), strict=True)
        self.assertTrue(rows)
        s = Session('replay')
        for row in rows: s.accept(row)
        r = s.rows[-1]
        self.assertEqual(r['completed'], 1)
        self.assertEqual(r['age'], sum(r[f'h{i}'] for i in range(8)) + r['unknown'])
        self.assertGreater(r['h7'], 100)
        self.assertFalse(s.fresh(0))
    def test_corruption_and_truncation(self):
        good = data().splitlines(keepends=True)[0]
        d = Decoder()
        self.assertEqual(len(d.feed(good[:12] + good)), 1)
        self.assertEqual(d.errors, 1)
        with self.assertRaises(ValueError): Decoder().feed(good.replace(b'GL1',b'GL2'), True)
        d = Decoder(); d.feed(good[:-2])
        with self.assertRaises(ValueError): d.finish()
    def test_impossible_histogram(self):
        row = Decoder().feed(data())[-1]
        row['h7'] += 1
        with self.assertRaises(ValueError): Decoder().feed(frame(row), True)
    def test_frozen_result(self):
        row = Decoder().feed(data())[-1]
        s = Session('replay'); s.accept(row)
        modified = dict(row, now=row['now']+1, samples=row['samples']+1)
        modified['h7'] -= 1; modified['h3'] += 1
        with self.assertRaises(ValueError): s.accept(modified)
    def test_clock_and_freshness(self):
        rows = Decoder().feed(data())
        s = Session('live'); s.connected=True
        s.accept(rows[0], 10)
        self.assertFalse(s.fresh(10))
        s.accept(rows[1], 10.1)
        self.assertTrue(s.fresh(10.1))
        s.accept(rows[1], 10.2)
        self.assertFalse(s.fresh(13))
        with self.assertRaises(ValueError): s.accept(rows[2], 15)
        self.assertTrue(s.blocked)
    def test_sparse_bin_growth(self):
        rows = Decoder().feed(data())
        a = next(r for r in rows if r['phase']==2)
        b = dict(a, now=a['now']+1, samples=a['samples']+1)
        b['age']+=2; b['unknown']+=2
        s=Session('replay'); s.accept(a)
        with self.assertRaises(ValueError): s.accept(b)
    def test_private_export(self):
        s = load_replay(ROOT/'firmware/build/demo.gl1')
        with tempfile.TemporaryDirectory() as folder:
            path=Path(folder)/'result.csv'
            s.export(path, 0, 'synthetic fixture')
            self.assertEqual(path.stat().st_mode & 0o777, 0o600)
            self.assertIn('jayis1', path.read_text())
            self.assertIn('replay', path.read_text())
            with self.assertRaises(FileExistsError): s.export(path,0,'')
    def test_replay_rejects_fifo(self):
        with tempfile.TemporaryDirectory() as folder:
            path=Path(folder)/'pipe'; os.mkfifo(path)
            with self.assertRaises(ValueError): load_replay(path)
    def test_serial_pty_restore_and_hangup(self):
        import pty, termios
        master, slave=pty.openpty(); original=termios.tcgetattr(slave)
        port=Serial(os.ttyname(slave))
        os.write(master,b'abc')
        import time; time.sleep(.01)
        self.assertEqual(port.read(), b'abc')
        port.close(); self.assertEqual(termios.tcgetattr(slave),original)
        port=Serial(os.ttyname(slave)); os.close(master)
        with self.assertRaises(OSError): port.read()
        port.close(); os.close(slave)

if __name__=='__main__': unittest.main()
