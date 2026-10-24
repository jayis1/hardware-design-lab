# Author: jayis1. SPDX-License-Identifier: MIT
import binascii
import importlib.util
from pathlib import Path
import unittest
ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('ledger', ROOT / 'app' / 'ledger.py')
ledger = importlib.util.module_from_spec(spec)
spec.loader.exec_module(ledger)
def frame(seq=0, stamp=0, up=4, down=2):
    body=f'WSL1,{seq},{stamp},{up},{down},{up-down},0,0,0,1,4,100,0,0,0'
    return (body+'*%04X\n'%binascii.crc_hqx(body.encode(),65535)).encode()
class ProtocolTests(unittest.TestCase):
    def test_crc(self):
        self.assertEqual(ledger.parse(frame())['difference'],2)
        with self.assertRaises(ValueError): ledger.parse(frame().replace(b'WSL1',b'WSL2'))
        with self.assertRaises(ValueError): ledger.parse(frame()[:-4]+b'000\n')
    def test_split_resync(self):
        stream=ledger.Stream()
        self.assertEqual(stream.feed(b'WSL1,garbage'+frame()[:10]),[])
        self.assertEqual(stream.feed(frame()[10:])[0]['up'],4)
        self.assertEqual(stream.feed(b'x'*1000),[])
        self.assertLessEqual(len(stream.pending),256)
    def test_history(self):
        model=ledger.Model()
        model.accept(ledger.parse(frame(3,300)),10)
        model.accept(ledger.parse(frame(4,400)),11)
        self.assertFalse(model.stale(11.5))
        self.assertTrue(model.stale(13))
        with self.assertRaises(ValueError): model.accept(ledger.parse(frame(4,500)),12)
        with self.assertRaises(ValueError): model.accept(ledger.parse(frame(5,1)),12)
    def test_wrap(self):
        model=ledger.Model()
        model.accept(ledger.parse(frame(4294967295,4294967290)),10)
        model.accept(ledger.parse(frame(0,10)),11)
        self.assertEqual(len(model.rows),2)
    def test_c_fixture(self):
        import subprocess
        result=subprocess.check_output([str(ROOT/'firmware/build/test-engine')])
        line=next(x for x in result.splitlines() if x.startswith(b'WSL1'))
        row=ledger.parse(line)
        self.assertEqual(row['up'],39999)
        self.assertEqual(row['difference'],0)
if __name__=='__main__': unittest.main()
