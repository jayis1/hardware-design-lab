# Author: jayis1. SPDX-License-Identifier: MIT
import binascii
import itertools
from pathlib import Path
import sys
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'app'))
from model import project, suggest, validate, Session
from transport import Decoder, load_replay
ROOT = Path(__file__).resolve().parents[1]
class ModelTests(unittest.TestCase):
    def test_real_encoder(self):
        self.session = load_replay(ROOT / 'firmware/build/demo.pc1')
        self.assertEqual(self.session.rows[-1]['coverage'], 0xffffff)
        self.assertEqual(self.session.rows[-1]['suggestion'], 16)
    def test_all_masks(self):
        for mask in range(16):
            want = 0
            for p, (a,b) in enumerate(itertools.combinations(range(4), 2)):
                bits = [bool(mask & (1 << a)), bool(mask & (1 << b))]
                want |= 1 << (4*p + (2 if bits[0] else 0) + (1 if bits[1] else 0))
            self.assertEqual(project(1 << mask), want)
    def test_truncation_resync(self):
        data = (ROOT / 'firmware/build/demo.pc1').read_bytes().splitlines(keepends=True)
        decoder = Decoder()
        rows = decoder.feed(data[0][:13] + data[1])
        self.assertEqual(len(rows), 1)
        self.assertEqual(decoder.errors, 1)
        decoder.feed(b'PC1,')
        with self.assertRaises(ValueError): decoder.finish()
    def test_invalid_coverage_and_history(self):
        rows = load_replay(ROOT / 'firmware/build/demo.pc1').rows
        row = dict(rows[-1]); row['coverage'] ^= 1
        with self.assertRaises(ValueError): validate(row)
        session = Session('test')
        session.accept(rows[-1])
        with self.assertRaises(ValueError): session.accept(rows[0])
    def test_contradictory_duplicate(self):
        row = dict(load_replay(ROOT / 'firmware/build/demo.pc1').rows[0])
        session = Session('test'); session.accept(row)
        row['tx_errors'] += 1
        with self.assertRaises(ValueError): session.accept(row)
    def test_exhaustive_projection(self):
        for subset in range(65536):
            expected = 0
            for m in range(16):
                if subset & (1 << m): expected |= project(1 << m)
            self.assertEqual(project(subset), expected)
    def test_crc_valid_impossible(self):
        data = (ROOT / 'firmware/build/demo.pc1').read_bytes().splitlines()[0]
        body = data.split(b'*')[0].split(b',')
        body[13] = b'1' # coverage without a setting
        body = b','.join(body)
        frame = body + b'*%04X\n' % binascii.crc_hqx(body, 65535)
        with self.assertRaises(ValueError): Decoder().feed(frame, strict=True)
    def test_suggestions(self):
        for settings in range(0,65536,17):
            coverage = project(settings)
            for raw in range(16):
                result = suggest(coverage,raw)
                if coverage == 0xffffff: self.assertEqual(result,16)
                else:
                    candidates = [m for m in range(16) if project(1<<m) & ~coverage]
                    want = min(candidates, key=lambda m: ((m^raw).bit_count(), -(project(1<<m)&~coverage).bit_count(),m))
                    self.assertEqual(result,want)
if __name__ == '__main__': unittest.main()
