# Author: jayis1. SPDX-License-Identifier: MIT
import importlib.util
from pathlib import Path
import tempfile
import time
import unittest
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('swing_app',ROOT/'app/app.py')
assert spec is not None and spec.loader is not None
app=importlib.util.module_from_spec(spec)
spec.loader.exec_module(app)

def line(sequence=1, now=250, crossings=0):
    fields=[sequence,now,1,0,0,crossings,0,0,0,0,0,0,0,0,0,0,0]
    body='SE1,'+','.join(map(str,fields))
    return (body+'*'+format(app.crc(body.encode()),'04X')+'\n').encode()

class ProtocolTests(unittest.TestCase):
    def test_fixture(self):
        m=app.Model(); m.start('synthetic replay')
        m.feed((ROOT/'firmware/build/fixture.txt').read_bytes())
        self.assertGreater(len(m.rows),20)
        self.assertEqual(m.latest['crossings'],16)
        self.assertEqual(m.errors,0)
        self.assertFalse(m.blocked)
    def test_partial_and_resync(self):
        m=app.Model(); m.start('live')
        m.feed(b'SE1,12,truncated'+line())
        self.assertEqual(len(m.rows),1)
        self.assertGreater(m.errors,0)
        raw=line(2,500)
        for b in raw: m.feed(bytes([b]))
        self.assertEqual(len(m.rows),2)
    def test_invalid(self):
        for raw in [b'x\n',line().replace(b'SE1,',b'SE2,'),line()[:-5]+b'0000\n',b'x'*1000+b'\n']:
            m=app.Model(); m.feed(raw)
            self.assertEqual(len(m.rows),0)
            self.assertGreater(m.errors,0)
    def test_reset_and_retention(self):
        m=app.Model(); m.start('live')
        m.feed(line(10,2500,10)); m.feed(line(11,2750,2))
        self.assertTrue(m.blocked)
        self.assertEqual(len(m.rows),1)
        m.disconnect()
        self.assertEqual(m.source,'live')
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'data.csv'; m.export(p)
            text=p.read_text()
            self.assertIn('jayis1',text)
            self.assertIn('suspected reset',text)
            with self.assertRaises(FileExistsError): m.export(p)
        m.start('synthetic replay')
        self.assertIsNone(m.latest)
    def test_stale_and_wrap(self):
        m=app.Model(); m.start('live')
        m.feed(line(0xffffffff,0xfffffff0))
        m.feed(line(0,250))
        self.assertFalse(m.blocked)
        self.assertIn('stale',m.status(time.monotonic()+3))
    def test_crc(self):
        self.assertEqual(app.crc(b'123456789'),0x29b1)

if __name__=='__main__': unittest.main()
