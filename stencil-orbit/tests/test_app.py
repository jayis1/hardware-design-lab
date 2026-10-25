# Author: jayis1. SPDX-License-Identifier: MIT
import importlib.util
from pathlib import Path
import tempfile
import unittest
root=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('orbit',root/'app/app.py')
assert spec is not None and spec.loader is not None
a=importlib.util.module_from_spec(spec)
spec.loader.exec_module(a)

class ProtocolTests(unittest.TestCase):
    def setUp(self):
        self.frames=(root/'firmware/build/fixture.txt').read_bytes().splitlines(keepends=True)
    def test_real_encoder(self):
        model=a.Model()
        for line in self.frames: model.feed(line,1.0)
        self.assertEqual(model.latest['accepted'],2)
        self.assertEqual(model.latest['station1'],1)
        self.assertEqual(model.errors,0)
    def test_corrupt_and_truncated(self):
        model=a.Model()
        model.feed(b'SO1,broken'+self.frames[0],1.0)
        self.assertIsNotNone(model.latest)
        model.feed(self.frames[-1].replace(b'SO1',b'SO9'),2.0)
        self.assertGreater(model.errors,0)
    def test_restart_and_stale(self):
        model=a.Model()
        model.feed(self.frames[-1],1.0)
        model.feed(self.frames[0],2.0)
        self.assertTrue(model.blocked)
        self.assertEqual(model.latest['accepted'],2)
        self.assertTrue(model.stale(5.0))
    def test_bounds(self):
        model=a.Model()
        model.feed(b'x'*10000,1.0)
        self.assertLessEqual(len(model.buffer),512)
        self.assertRaises(ValueError,a.parse,b'SO1,0*0000\n')
    def test_export_no_overwrite(self):
        model=a.Model(); model.origin='replay'
        model.feed(self.frames[0],1.0)
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'out.csv'; model.export(p,5.0)
            self.assertIn('jayis1',p.read_text())
            self.assertIn('replay',p.read_text())
            self.assertRaises(FileExistsError,model.export,p,5.0)

if __name__=='__main__': unittest.main()
