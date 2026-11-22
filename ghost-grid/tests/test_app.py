# Author: jayis1. SPDX-License-Identifier: MIT
import binascii
import pathlib
import sys
import unittest
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / 'app'))
from model import FIELDS, Session, validate, predicted
from transport import Decoder, load_replay


def blank():
    return dict.fromkeys(FIELDS, 0)


def encoded(row):
    body = ('GG1,' + ','.join(str(row[k]) for k in FIELDS)).encode()
    return body + f'*{binascii.crc_hqx(body,65535):04X}\n'.encode()


class ModelTests(unittest.TestCase):
    def test_all_topologies(self):
        for mask in range(16):
            expected = 15 if mask.bit_count() == 3 else mask
            self.assertEqual(predicted(mask), expected)

    def test_fixture_and_fragment_recovery(self):
        path = pathlib.Path(__file__).resolve().parents[1] / 'firmware/build/demo.gg1'
        session = load_replay(path)
        self.assertGreater(session.rows[-1]['bins7'], 0)
        self.assertGreater(session.rows[-1]['bins15'], 0)
        data = encoded(blank())
        decoder = Decoder()
        self.assertEqual(decoder.feed(b'GG1,1,2' + data), [blank()])
        self.assertEqual(decoder.errors, 1)
        decoder.feed(data[:10])
        with self.assertRaises(ValueError):
            decoder.finish()

    def test_impossible_frames(self):
        for change in ({'bins7': 1}, {'valid': 1, 'raw': 7}, {'samples': 1},
                       {'fault': 32}, {'run': 1}, {'now': 2**32}):
            row = blank(); row.update(change)
            with self.assertRaises(ValueError):
                Decoder().feed(encoded(row), strict=True)

    def test_sparse_growth_and_halt_freeze(self):
        first = blank(); first.update(now=10, samples=10, unknown=10)
        for change in ({'now': 11, 'samples': 12, 'unknown': 12},
                       {'now': 11, 'tx_errors': 0, 'unknown': 9, 'samples': 9}):
            s = Session('replay'); s.accept(first)
            row = first.copy(); row.update(change)
            with self.assertRaises(ValueError): s.accept(row)
        first['fault'] = 1
        s = Session('replay'); s.accept(first)
        row = first.copy(); row.update(now=20, samples=11, unknown=11)
        with self.assertRaises(ValueError): s.accept(row)

    def test_freshness_requires_acquisition(self):
        s = Session('live')
        row = blank()
        for i in range(3):
            row = blank(); row.update(now=i*100+1, samples=i*100+1, unknown=i*100+1)
            s.accept(row, i*.1)
        self.assertTrue(s.fresh(.2, True))
        for i in range(3, 9):
            row = row.copy(); row['now'] = i*100+1; row['fault'] = 1
            s.accept(row, i*.1)
        self.assertFalse(s.fresh(.8, True))
        self.assertFalse(s.fresh(.2, False))

    def test_qualification_requires_observations(self):
        for count in range(6):
            row = blank()
            row.update(samples=count, unknown=count, raw=7, qualified=7, valid=1)
            with self.assertRaises(ValueError): Decoder().feed(encoded(row), strict=True)

    def test_adjacent_segment_and_peak_accounting(self):
        first = blank()
        first.update(now=10, samples=10, unknown=6, raw=7, qualified=7,
                     valid=1, run=4, bins7=4, segments7=1, peaks7=4)
        good = first.copy()
        good.update(now=11, samples=11, run=5, bins7=5, peaks7=5)
        session = Session('replay'); session.accept(first); session.accept(good)
        for changes in ({'run': 0, 'segments7': 2, 'peaks7': 4},
                        {'run': 4, 'peaks7': 4, 'segments7': 2}):
            session = Session('replay'); session.accept(first)
            bad = good.copy(); bad.update(changes)
            validate(bad)
            with self.assertRaises(ValueError): session.accept(bad)

    def test_duplicate_contradiction(self):
        s = Session('replay'); s.accept(blank())
        row = blank(); row['raw'] = 1
        with self.assertRaises(ValueError): s.accept(row)

if __name__ == '__main__':
    unittest.main()
