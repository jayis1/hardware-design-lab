# Author: jayis1. SPDX-License-Identifier: MIT
import binascii
import csv
import os
from pathlib import Path
import pty
import sys
import tempfile
import termios
import unittest
from unittest.mock import patch
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'app'))
from model import FIELDS, Session, validate
from transport import Decoder, Serial, load_replay
ROOT = Path(__file__).resolve().parents[1]

def frame(row):
    body = ('PC1,' + ','.join(str(row[x]) for x in FIELDS)).encode()
    return body + f'*{binascii.crc_hqx(body, 65535):04X}\n'.encode()

def idle():
    r = dict.fromkeys(FIELDS, 0)
    r.update(sequence=100, now=100, state=1, valid=15, baseline=50)
    return r

class ModelTests(unittest.TestCase):
    def test_real_encoder(self):
        s = load_replay(ROOT / 'firmware/build/demo.pc1')
        r = s.rows[-1]
        self.assertEqual((r['completed'], r['pulses'], r['a'], r['b']), (1, 5, 2, 1))
        self.assertEqual((r['ab'], r['ba'], r['uncertain'], r['neither']), (1, 0, 1, 1))
    def test_crc_and_resync(self):
        data = frame(idle())
        d = Decoder()
        self.assertEqual(len(d.feed(b'PC1,broken' + data)), 1)
        self.assertEqual(d.errors, 1)
        self.assertEqual(d.feed(data.replace(b'*', b'*F')), [])
        self.assertEqual(d.errors, 2)
    def test_partial_tail(self):
        d = Decoder(); d.feed(frame(idle()) + b'PC1,')
        with self.assertRaises(ValueError): d.finish()
    def test_impossible_counts(self):
        for update in ({'pulses': 1}, {'state': 3}, {'age': 60001}, {'anchor': 1},
                       {'starts': 1}, {'gaps': 1}, {'aa': 1}, {'valid': 16}):
            r = idle(); r.update(update)
            with self.subTest(update=update), self.assertRaises(ValueError):
                Decoder().feed(frame(r), strict=True)
    def test_duplicate_and_freshness(self):
        s = Session('live'); s.connected = True
        r = idle(); s.accept(r, 0)
        self.assertFalse(s.fresh(0))
        r = dict(r, now=200, sequence=200); s.accept(r, 0.1)
        self.assertTrue(s.fresh(0.2))
        s.accept(r, 3)
        self.assertFalse(s.fresh(3))
        with self.assertRaises(ValueError): s.accept(dict(r, now=201, sequence=201), 3)
    def test_counter_and_time_rollback(self):
        for update in ({'now': 99, 'sequence': 99}, {'now': 100, 'sequence': 101},
                       {'now': 200, 'sequence': 199}, {'now': 200, 'sequence': 201}):
            s = Session('replay'); s.accept(idle())
            with self.assertRaises(ValueError): s.accept(dict(idle(), **update))
    def test_frozen_run(self):
        r = idle(); r.update(starts=1, completed=1, age=20, a=1, pulses=1, anchor=1)
        for change in ({'age': 21}, {'a': 0, 'b': 1, 'anchor': 2}, {'anchor': 0}):
            s = Session('replay'); s.accept(r)
            with self.assertRaises(ValueError):
                s.accept(dict(r, now=200, sequence=200, **change))
    def test_halt_freeze(self):
        r = idle(); r.update(state=3, fault=1, baseline=0)
        s = Session('replay'); s.accept(r)
        s.accept(dict(r, now=200))
        with self.assertRaises(ValueError): s.accept(dict(r, now=300, raw=1))
    def test_impossible_pair_source(self):
        r = idle()
        r.update(state=2, qualified=8, raw=8, starts=1, baseline=0,
                 age=30, pulses=3, a=2, neither=1, aa=1, ba=1, anchor=1)
        with self.assertRaises(ValueError): Decoder().feed(frame(r), strict=True)
    def test_one_sample_completed_run(self):
        s = Session('replay'); s.accept(idle())
        r = dict(idle(), state=0, sequence=101, now=101, starts=1, completed=1, baseline=0)
        with self.assertRaises(ValueError): s.accept(r)
    def test_export_metadata(self):
        s = Session('live'); s.errors = 3; s.warning = 'test warning'
        with tempfile.TemporaryDirectory() as td:
            path = Path(td) / 'e.csv'; s.export(path)
            with path.open() as handle:
                rows = list(csv.reader(handle))
            self.assertEqual(rows[1][0], 'jayis1')
            self.assertIn('test warning', rows[1])
            self.assertEqual(path.stat().st_mode & 0o777, 0o600)
            with self.assertRaises(FileExistsError): s.export(path)
    def test_replay_special(self):
        with tempfile.TemporaryDirectory() as td:
            path = Path(td) / 'pipe'; os.mkfifo(path)
            with self.assertRaises(ValueError): load_replay(path)
            path = Path(td) / 'empty'; path.write_bytes(b'')
            with self.assertRaises(ValueError): load_replay(path)
    def test_serial_restore(self):
        master, slave = pty.openpty()
        try:
            saved = termios.tcgetattr(slave)
            port = Serial(os.ttyname(slave))
            os.write(master, b'abc')
            self.assertEqual(port.read(), b'abc')
            port.close()
            self.assertEqual(saved, termios.tcgetattr(slave))
            with patch('transport.termios.tcflush', side_effect=OSError('injected')):
                with self.assertRaises(OSError): Serial(os.ttyname(slave))
            self.assertEqual(saved, termios.tcgetattr(slave))
        finally:
            os.close(master); os.close(slave)

if __name__ == '__main__': unittest.main()
