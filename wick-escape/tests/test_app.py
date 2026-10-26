# Author: jayis1. SPDX-License-Identifier: MIT
import importlib.util
import pathlib
import tempfile
import unittest
ROOT = pathlib.Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('wick', ROOT/'app/wick.py')
assert spec is not None and spec.loader is not None
wick = importlib.util.module_from_spec(spec)
spec.loader.exec_module(wick)

def frame(seq=7, now=1400, state=3, mask=3, faults=0, elapsed=100,
          adc=(500,500,4000,4000), first=(0,100,4294967295,4294967295)):
    body=','.join(map(str,['W1',seq,now,state,mask,faults,elapsed,*adc,*first]))
    return (body+','+format(wick.crc(body.encode()),'04X')+'\n').encode()

class TestApp(unittest.TestCase):
    def test_real_encoder(self):
        data=(ROOT/'firmware/build/fixture.txt').read_bytes()
        record=wick.parse(data.rstrip(b'\n'))
        self.assertEqual(record['first'][1],100)
        self.assertEqual(record['state'],3)
    def test_impossible(self):
        for data in [frame(elapsed=0), frame(mask=16), frame(faults=1),
                     frame(first=(0,101,4294967295,4294967295)),
                     frame(adc=(4096,0,0,0)), frame(state=9)]:
            with self.assertRaises(ValueError): wick.parse(data.strip())
    def test_fragment_resync(self):
        stream=wick.Stream()
        self.assertEqual(stream.feed(b'W1,broken'),[])
        records=stream.feed(frame()[:9])+stream.feed(frame()[9:])
        self.assertEqual(len(records),1)
        self.assertEqual(stream.errors,1)
        stream.finish()
        stream.feed(b'W1,partial')
        with self.assertRaises(ValueError): stream.finish()
    def test_corruption(self):
        stream=wick.Stream()
        self.assertEqual(stream.feed(frame().replace(b'1400',b'1401')),[])
        self.assertEqual(stream.errors,1)
    def test_rollback_duplicate_and_stale(self):
        model=wick.Model('live')
        r=wick.parse(frame().strip()); self.assertTrue(model.accept(r,10))
        self.assertFalse(model.accept(r,10.2))
        self.assertEqual(model.last_host,10)
        self.assertEqual(model.freshness(12),'stale')
        self.assertFalse(model.accept(wick.parse(frame(seq=8,now=1600).strip()),13))
        self.assertTrue(model.blocked)
        model=wick.Model('live'); model.accept(r,10)
        self.assertFalse(model.accept(wick.parse(frame(seq=6,now=1300).strip()),10.2))
        self.assertTrue(model.blocked)
    def test_same_time_conflict(self):
        model=wick.Model('live'); model.accept(wick.parse(frame().strip()),10)
        self.assertFalse(model.accept(wick.parse(frame(adc=(600,500,4000,4000)).strip()),10.1))
        self.assertTrue(model.blocked)
    def test_export_and_replay(self):
        with tempfile.TemporaryDirectory() as tmp:
            p=pathlib.Path(tmp)/'capture.txt'; p.write_bytes(frame())
            model=wick.replay(p)
            self.assertEqual(len(model.rows),1)
            out=pathlib.Path(tmp)/'result.json'; model.export(out,2)
            self.assertIn('jayis1',out.read_text())
            self.assertIn('replay',out.read_text())
            with self.assertRaises(FileExistsError): model.export(out,2)
            p.write_bytes(frame()+b'W1')
            with self.assertRaises(ValueError): wick.replay(p)
            p.write_bytes(b'')
            with self.assertRaises(ValueError): wick.replay(p)
    def test_fifo_rejected(self):
        import os
        with tempfile.TemporaryDirectory() as tmp:
            p=pathlib.Path(tmp)/'fifo'; os.mkfifo(p)
            with self.assertRaises(ValueError): wick.replay(p)

    def test_impossible_flank_success(self):
        for data in [frame(mask=7),frame(first=(0,100,100,4294967295))]:
            with self.assertRaises(ValueError): wick.parse(data.strip())
    def test_terminal_mutation(self):
        model=wick.Model('replay'); model.accept(wick.parse(frame().strip()),10)
        self.assertFalse(model.accept(wick.parse(frame(seq=8,now=601300,state=4,elapsed=600000).strip()),11))
        self.assertTrue(model.blocked)
    def test_adc_invalid_marker(self):
        r=wick.parse(frame(state=5,faults=16).strip())
        self.assertFalse(r['adc_valid'])

if __name__=='__main__': unittest.main()
