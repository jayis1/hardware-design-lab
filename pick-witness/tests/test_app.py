# Author: jayis1. SPDX-License-Identifier: MIT
import importlib.util
import pathlib
import unittest
import tempfile
ROOT = pathlib.Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('pick_app', ROOT/'app'/'app.py')
assert spec and spec.loader
app = importlib.util.module_from_spec(spec)
spec.loader.exec_module(app)

class ProtocolTests(unittest.TestCase):
    def setUp(self):
        self.lines = (ROOT/'firmware/build/fixture.txt').read_bytes().splitlines(keepends=True)

    def test_real_c_fixture(self):
        model = app.Model()
        model.start('synthetic replay')
        for line in self.lines:
            model.feed(line)
        self.assertEqual(model.latest['accepted'], 2)
        self.assertEqual(model.latest['rejected'], 1)
        self.assertEqual(model.errors, 0)

    def test_resync_and_crc(self):
        m = app.Model(); m.start('synthetic replay')
        m.feed(b'junk@PW1,broken'+self.lines[0])
        self.assertIsNotNone(m.latest)
        self.assertGreater(m.errors, 0)
        m.feed(self.lines[1].replace(b'@PW1', b'@PW2'))
        m.feed(self.lines[1].replace(b',0,', b',9,', 1))
        self.assertEqual(len(m.rows), 1)
        m.feed(b'x'*10000)
        self.assertLessEqual(len(m.buffer), 256)

    def test_reset_blocks_and_export_preserves_provenance(self):
        m=app.Model(); m.start('synthetic replay')
        m.feed(b''.join(self.lines))
        m.feed(self.lines[0])
        self.assertTrue(m.blocked)
        m.connected=False
        with tempfile.TemporaryDirectory() as d:
            path=pathlib.Path(d)/'report.csv'
            m.export(path)
            text=path.read_text()
            self.assertIn('synthetic replay',text)
            self.assertIn('jayis1',text)
            self.assertIn('True',text)
            with self.assertRaises(FileExistsError): m.export(path)

    def test_bounds_and_version(self):
        for body in ['@PW2,0', '@PW1,'+','.join(['4294967296']*18), '@PW1,'+','.join(['-1']*18)]:
            data=(body+'*%04X\n'%app.crc(body.encode())).encode()
            with self.assertRaises(ValueError): app.decode(data)

if __name__=='__main__':
    unittest.main()
