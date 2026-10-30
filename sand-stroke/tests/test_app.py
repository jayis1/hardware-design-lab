# Author: jayis1. SPDX-License-Identifier: MIT
import binascii
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'app'))
from protocol import FIELDS, Session, decode, replay, Stream
ROOT = Path(__file__).resolve().parents[1]
def fixture():
    return subprocess.check_output([str(ROOT / 'firmware/build/test-engine'), 'fixture'])
def encode(values):
    payload = b'SS1' + b''.join(b',%08X' % values[k] for k in FIELDS)
    return payload + b'*%04X\n' % binascii.crc_hqx(payload, 65535)
class ProtocolTests(unittest.TestCase):
    def test_real_encoder(self):
        lines = fixture().splitlines(keepends=True)
        session = Session('replay')
        for line in lines:
            session.accept(decode(line), 0)
        self.assertEqual(session.rows[-1]['full_a'], 3)
        self.assertFalse(session.fresh(0))
    def test_corruption_and_conservation(self):
        line = fixture().splitlines(keepends=True)[0]
        with self.assertRaises(ValueError):
            decode(line.replace(b'SS1', b'SS2'))
        data = decode(line)
        data['full_a'] += 1
        with self.assertRaises(ValueError):
            decode(encode(data))
    def test_inconsistent_duration(self):
        data = decode(fixture().splitlines(keepends=True)[0])
        data['ms_b'] = 1
        with self.assertRaises(ValueError):
            decode(encode(data))
    def test_freshness(self):
        lines = [decode(x) for x in fixture().splitlines(keepends=True)]
        session = Session('live')
        session.accept(lines[0], 0)
        self.assertFalse(session.fresh(0))
        session.accept(lines[1], .1)
        self.assertTrue(session.fresh(.1))
        session.accept(lines[1], .7)
        self.assertFalse(session.fresh(1))
        with self.assertRaises(ValueError):
            session.accept(lines[2], 2)
        self.assertTrue(session.warning)
    def test_duplicate_conflict(self):
        row = decode(fixture().splitlines(keepends=True)[0])
        session = Session('live')
        session.accept(row, 0)
        with self.assertRaises(ValueError):
            session.accept(dict(row, tx=1), .1)
    def test_fragment_resync(self):
        line = fixture().splitlines(keepends=True)[0]
        stream = Stream()
        self.assertEqual(stream.feed(b'garbage' + line[:30]), [])
        self.assertEqual(stream.feed(line), [decode(line)])
        self.assertGreater(stream.errors, 0)
        self.assertEqual(stream.buffer, b'')
    def test_replay_restrictions(self):
        with tempfile.TemporaryDirectory() as folder:
            p = Path(folder)/'record'
            p.write_bytes(fixture())
            self.assertEqual(len(replay(p).rows), 4)
            p.write_bytes(fixture() + b'SS1,')
            with self.assertRaises(ValueError): replay(p)
            p.write_bytes(b'')
            with self.assertRaises(ValueError): replay(p)
            p.unlink()
            os.mkfifo(p)
            with self.assertRaises(ValueError): replay(p)
    def test_counter_rollback(self):
        lines = [decode(x) for x in fixture().splitlines(keepends=True)]
        session = Session('replay')
        session.accept(lines[-1], 0)
        with self.assertRaises(ValueError):
            session.accept(dict(lines[0], now=lines[-1]['now']+100), 0)
    def test_interframe_duration_conservation(self):
        old = decode(fixture().splitlines(keepends=True)[0])
        changes = [dict(ms_a=old['ms_a']+1),
                   dict(full_a=old['full_a']+1, started=old['started']+1),
                   dict(full_a=old['full_a']+1, full_b=1,
                        started=old['started']+2, ms_a=old['ms_a']+100, ms_b=100)]
        for change in changes:
            session = Session('replay')
            session.accept(old, 0)
            new = dict(old, now=old['now']+100, **change)
            with self.assertRaises(ValueError): session.accept(decode(encode(new)), 0)

    def test_sustained_drift_is_conservatively_blocked(self):
        # Fixed allowance intentionally does not license unlimited backlog.
        row = decode(fixture().splitlines(keepends=True)[0])
        session = Session('live')
        session.accept(row, 0)
        with self.assertRaises(ValueError):
            for i in range(1, 1000):
                session.accept(dict(row, now=row['now']+100*i), .101*i)
        self.assertFalse(session.fresh(101))
if __name__ == '__main__': unittest.main()
