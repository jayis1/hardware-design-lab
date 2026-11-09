# Author: jayis1. SPDX-License-Identifier: MIT
import binascii
import pathlib
import sys
import tempfile
import unittest
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / 'app'))
from model import FIELDS, Session, validate
from transport import Decoder, load_replay

def frame(row):
    body = ('LV1,' + ','.join(str(row[f]) for f in FIELDS)).encode()
    return body + ('*%04X\n' % binascii.crc_hqx(body, 65535)).encode()

def idle():
    row = dict.fromkeys(FIELDS, 0)
    row.update(sequence=10, now=10, state=1, valid=15)
    return row

class Tests(unittest.TestCase):
    def test_roundtrip_and_fragments(self):
        data = frame(idle())
        d = Decoder()
        self.assertEqual(d.feed(data[:7]), [])
        self.assertEqual(d.feed(data[7:]), [idle()])
        self.assertEqual(d.feed(b'LV1,broken' + data), [idle()])
        self.assertGreater(d.errors, 0)
        with self.assertRaises(ValueError): Decoder().feed(data.replace(b'LV1', b'LV2'), True)
    def test_invalid_accounting(self):
        for field in ('completed', 'rejected', 'aa', 'last_truth', 'fault'):
            row = idle(); row[field] = 1
            with self.assertRaises(ValueError): validate(row)
    def test_rollback_and_duplicates(self):
        s = Session('replay'); s.accept(idle())
        self.assertFalse(s.accept(idle()))
        row = idle(); row['state'] = 0
        with self.assertRaises(ValueError): s.accept(row)
        s = Session('replay'); s.accept(idle())
        row = idle(); row['sequence'] = 9; row['now'] = 11
        with self.assertRaises(ValueError): s.accept(row)
    def test_retained_result_cannot_change(self):
        row = idle(); row.update(starts=1, completed=1, aa=1, last_truth=1, last_answer=1, last_ms=20, sequence=50, now=50)
        s = Session('replay'); s.accept(row)
        changed = dict(row, now=51, sequence=51, last_ms=21)
        with self.assertRaises(ValueError): s.accept(changed)
    def test_real_fixture(self):
        path = pathlib.Path(__file__).resolve().parents[1] / 'firmware/build/demo.lv1'
        s = load_replay(path)
        self.assertEqual([s.rows[-1][f] for f in ('aa','ab','ba','bb')], [1,1,1,1])
        self.assertEqual(s.rows[-1]['rejected'], 1)
        with tempfile.TemporaryDirectory() as tmp:
            for name, data in [('empty', b''), ('partial', path.read_bytes()+b'LV1,')]:
                p = pathlib.Path(tmp)/name; p.write_bytes(data)
                with self.assertRaises(ValueError): load_replay(p)
    def test_freshness_requires_progress(self):
        s = Session('live'); s.connected = True
        s.accept(idle(), 100)
        self.assertFalse(s.fresh(100))
        row = dict(idle(), sequence=110, now=110)
        s.accept(row, 100.1)
        self.assertTrue(s.fresh(100.2))
        self.assertFalse(s.fresh(103))
        with self.assertRaises(ValueError): s.accept(dict(row, sequence=210, now=210), 105)

    def test_impossible_sparse_transitions(self):
        old = dict(idle(), sequence=100, now=100, state=2, starts=1,
                   age=20, answer=1, tainted=1)
        new = dict(old, sequence=101, now=101, state=0, age=0, answer=0,
                   tainted=0, completed=1, aa=1, last_truth=1, last_answer=1, last_ms=21)
        s = Session('replay'); s.accept(old)
        with self.assertRaises(ValueError): s.accept(new)
        old = idle()
        new = dict(old, sequence=11, now=11, starts=1, completed=1, aa=1,
                   last_truth=1, last_answer=1, last_ms=10)
        s = Session('replay'); s.accept(old)
        with self.assertRaises(ValueError): s.accept(new)
        s = Session('replay'); s.accept(old)
        with self.assertRaises(ValueError): s.accept(dict(old, sequence=11, now=11, valid=0))

if __name__ == '__main__': unittest.main()
