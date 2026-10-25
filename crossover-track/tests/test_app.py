# Author: jayis1. SPDX-License-Identifier: MIT
import importlib.util
from pathlib import Path
import unittest
ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('crossapp', ROOT/'app/app.py')
assert spec is not None and spec.loader is not None
app = importlib.util.module_from_spec(spec)
spec.loader.exec_module(app)

class Tests(unittest.TestCase):
    def test_fixture(self):
        m = app.Model()
        m.begin_study()
        for line in (ROOT/'firmware/build/fixture.txt').read_bytes().splitlines(True):
            m.feed(line, 1.0)
        self.assertEqual(m.errors, 0)
        self.assertEqual(len(m.results), 4)
        self.assertEqual(len(m.study), 4)
        self.assertEqual(m.analysis(), (0.0, 0.0))

    def test_split_and_corrupt(self):
        data = (ROOT/'firmware/build/fixture.txt').read_bytes()
        m = app.Model()
        for char in data:
            m.feed(bytes([char]), 1.0)
        self.assertEqual(len(m.results), 4)
        m.feed(b'garbage\nCT1,broken*0000\n', 1.0)
        self.assertGreater(m.errors, 0)
        self.assertLess(len(m.buffer), 513)

    def test_duplicate_not_fresh(self):
        line = (ROOT/'firmware/build/fixture.txt').read_bytes().splitlines(True)[1]
        m = app.Model()
        m.feed(line, 1.0)
        m.feed(line, 10.0)
        self.assertEqual(m.received, 1.0)
        self.assertTrue(m.stale(10.0))

    def test_reset_blocks(self):
        lines = (ROOT/'firmware/build/fixture.txt').read_bytes().splitlines(True)
        m = app.Model()
        m.feed(lines[-1], 1.0)
        m.feed(lines[0], 2.0)
        self.assertTrue(m.blocked)

    def test_resync(self):
        line = (ROOT/'firmware/build/fixture.txt').read_bytes().splitlines(True)[1]
        m = app.Model()
        m.feed(b'CT1,partial'+line, 1.0)
        self.assertIsNotNone(m.current)

    def test_abba(self):
        m = app.Model()
        m.study = [dict(sl=0,sr=0,fl=l,fr=r) for l,r in [(120,100),(110,110),(110,110),(120,100)]]
        # 10 ms cart effect and 10 ms lane effect, not raw 20 ms.
        self.assertEqual(m.analysis(), (10.0,10.0))

    def test_unsafe_replay(self):
        with self.assertRaises(ValueError):
            app.read_replay('/dev/null')

    def test_stale_live_study_stops_before_resumed_frame(self):
        lines = (ROOT/'firmware/build/fixture.txt').read_bytes().splitlines(True)
        m = app.Model()
        m.provenance = 'live'
        m.begin_study()
        m.feed(lines[0], 1.0)
        m.feed(lines[1], 4.0)
        self.assertFalse(m.collecting)
        self.assertIn('Stale', m.warning)

    def test_empty_replay_fails(self):
        self.assertFalse(app.replay_valid(app.Model()))

if __name__ == '__main__':
    unittest.main()
