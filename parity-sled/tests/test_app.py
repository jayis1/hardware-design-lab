# Author: jayis1. SPDX-License-Identifier: MIT
import binascii
import pathlib
import sys
import unittest
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / 'app'))
from model import FIELDS, Session, validate, parity_odd
from transport import Decoder, load_replay


def blank():
    return dict.fromkeys(FIELDS, 0)


def encoded(row):
    body = ('PY1,' + ','.join(str(row[k]) for k in FIELDS)).encode()
    return body + f'*{binascii.crc_hqx(body,65535):04X}\n'.encode()


class ModelTests(unittest.TestCase):
    def test_all_words(self):
        for mask in range(16):
            self.assertEqual(parity_odd(mask), bin(mask).count('1') % 2)

    def test_fixture_and_fragment_recovery(self):
        path = pathlib.Path(__file__).resolve().parents[1] / 'firmware/build/demo.py1'
        session = load_replay(path)
        self.assertGreater(session.rows[-1]['bins7'], 0)
        self.assertGreater(session.rows[-1]['bins15'], 0)
        data = encoded(blank())
        decoder = Decoder()
        self.assertEqual(decoder.feed(b'PY1,1,2' + data), [blank()])
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
                     valid=1, run=4, bins7=4, segments7=1, peaks7=4,
                     starts=1, active=1, age=4)
        good = first.copy()
        good.update(now=11, samples=11, run=5, bins7=5, peaks7=5, age=5)
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

    def test_episode_and_sparse_real_encoder(self):
        import subprocess
        root = pathlib.Path(__file__).resolve().parents[1]
        data = subprocess.check_output([str(root/'firmware/build/fixture'), 'pairs'])
        decoder = Decoder(); rows = []
        for offset in range(0, len(data), 4096):
            rows.extend(decoder.feed(data[offset:offset+4096], strict=True))
        decoder.finish()
        self.assertEqual(len(rows), 4096)
        for stride in (1, 2, 7, 19, 101):
            session = Session('replay')
            for row in rows[::stride]: session.accept(row)
        session = load_replay(root/'firmware/build/demo.py1')
        end = session.rows[-1]
        self.assertGreater(end['same'], 0)
        self.assertGreater(end['changed'], 0)
        self.assertEqual(end['orphan'], 1)
        self.assertEqual(end['aborted'], 1)
        for mutation in ({'active': 1}, {'last_age': 0}, {'anchor': 1}, {'last_kind': 4}):
            bad = end.copy(); bad.update(mutation)
            with self.assertRaises(ValueError): Decoder().feed(encoded(bad), strict=True)

    def test_retained_result_and_active_age(self):
        root = pathlib.Path(__file__).resolve().parents[1]
        rows = load_replay(root/'firmware/build/demo.py1').rows
        for index in (30, 380, 440):
            first, next_row = rows[index:index+2]
            bad = next_row.copy()
            if bad['active']: bad['age'] += 1
            else: bad['last_age'] += 1
            validate(bad)
            session = Session('replay'); session.accept(first)
            with self.assertRaises(ValueError): session.accept(bad)

    def test_first_halt_abandons_exactly_once(self):
        root = pathlib.Path(__file__).resolve().parents[1]
        first = load_replay(root/'firmware/build/demo.py1').rows[30]
        self.assertEqual(first['active'], 1)
        stopped = first.copy()
        stopped.update(now=first['now']+1, fault=4, active=0, age=0,
                       aborted=first['aborted']+1)
        session = Session('replay'); session.accept(first); session.accept(stopped)
        for field in ('same', 'changed', 'orphan'):
            bad = stopped.copy(); bad[field] += 1
            with self.assertRaises(ValueError): Decoder().feed(encoded(bad), strict=True)

    def test_qualification_cannot_skip_new_word_window(self):
        root = pathlib.Path(__file__).resolve().parents[1]
        first = load_replay(root/'firmware/build/demo.py1').rows[19]
        bad = first.copy()
        bad.update(now=first['now']+1, samples=first['samples']+1,
                   unknown=first['unknown']+1, raw=1, qualified=1,
                   valid=1, run=0, starts=1, active=1, age=0)
        validate(bad)
        session = Session('replay'); session.accept(first)
        with self.assertRaises(ValueError): session.accept(bad)

    def test_single_acquisition_gap_must_abandon(self):
        root = pathlib.Path(__file__).resolve().parents[1]
        rows = load_replay(root/'firmware/build/demo.py1').rows
        first, bad = rows[25].copy(), rows[26].copy()
        bad['now'] += 1; bad['missed'] += 1
        validate(bad)
        session = Session('replay'); session.accept(first)
        with self.assertRaises(ValueError): session.accept(bad)
        # The fixture includes a real engine gap, accepted even in isolation.
        gap_index = next(i for i, r in enumerate(rows) if r['missed'])
        session = Session('replay')
        session.accept(rows[gap_index-1]); session.accept(rows[gap_index])

    def test_six_observations_cannot_credit_new_word(self):
        root = pathlib.Path(__file__).resolve().parents[1]
        rows = load_replay(root/'firmware/build/demo.py1').rows
        first, actual = rows[19], rows[25]
        session = Session('replay'); session.accept(first); session.accept(actual)
        session.accept(rows[26])  # First genuinely creditable interval.
        for mutation in ({'unknown': 11, 'bins1': 1, 'segments1': 1,
                          'peaks1': 1, 'run': 1}, {'age': 1}):
            bad = actual.copy(); bad.update(mutation)
            validate(bad)
            session = Session('replay'); session.accept(first)
            with self.assertRaises(ValueError): session.accept(bad)

if __name__ == '__main__':
    unittest.main()
