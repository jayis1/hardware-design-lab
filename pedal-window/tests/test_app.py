# Author: jayis1. SPDX-License-Identifier: MIT
import importlib.util
from pathlib import Path
import unittest
ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('pedal_app', ROOT/'app/app.py')
assert spec is not None and spec.loader is not None
app = importlib.util.module_from_spec(spec)
spec.loader.exec_module(app)


class ProtocolTests(unittest.TestCase):
    def setUp(self):
        self.lines = (ROOT/'firmware/build/fixture.txt').read_bytes().splitlines(keepends=True)

    def test_real_c_fixture(self):
        model = app.Model()
        model.source = 'synthetic fixture'
        model.feed(b''.join(self.lines), 1.0)
        self.assertEqual(len(model.history), 2)
        self.assertEqual(model.history[-1]['release_delta'], -40)
        self.assertEqual(model.history[0]['release_delta'], 20)
        self.assertEqual(model.errors, 0)
        self.assertFalse(model.stale(1.5))
        self.assertTrue(model.stale(3.0))

    def test_truncated_frame_and_bad_crc(self):
        model = app.Model()
        model.feed(self.lines[0][:12] + self.lines[-1], 1.0)
        self.assertEqual(model.latest['completed'], 2)
        self.assertEqual(model.errors, 1)
        model.feed(self.lines[0].replace(b'PW1,0', b'PW1,9'), 1.1)
        self.assertEqual(model.errors, 2)

    def test_reboot_blocks_until_explicit_new_session(self):
        model = app.Model()
        model.feed(b''.join(self.lines), 1)
        model.feed(self.lines[0], 2)
        self.assertTrue(model.blocked)
        self.assertEqual(len(model.history), 2)
        model.feed(self.lines[-1], 3)
        self.assertTrue(model.blocked)

    def test_duplicate_does_not_refresh(self):
        model = app.Model()
        model.feed(self.lines[0], 1)
        model.feed(self.lines[0], 3)
        self.assertTrue(model.stale(3))

    def test_semantically_impossible_completion(self):
        model = app.Model()
        body = b'PW1,0,100,0,10,0,1,0,0,0,0,1,1,2,0,0,-789,8,5000'
        model.feed(body + ('*%04X\n' % app.crc(body)).encode(), 1)
        self.assertEqual(model.errors, 1)
        self.assertEqual(model.history, [])

    def test_invalid_fields_and_policy(self):
        body = b'PW1,0,100,0,10,0,1,1,0,0,0,1,1,0,0,-4000,4000,8,5000'
        encoded = body + ('*%04X\n' % app.crc(body)).encode()
        with self.assertRaises(ValueError):
            app.parse(encoded)
        payload = self.lines[0].split(b'*')[0]
        for candidate in [payload.replace(b',8,5000', b',9,5000'), payload+b',5', b'XX1,0']:
            encoded = candidate + ('*%04X\n' % app.crc(candidate)).encode()
            with self.assertRaises(ValueError):
                app.parse(encoded)
        model = app.Model()
        model.feed(b'x'*5000, 1)
        self.assertLessEqual(len(model.pending), 256)


if __name__ == '__main__':
    unittest.main()
