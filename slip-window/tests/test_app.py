# Author: jayis1. SPDX-License-Identifier: MIT
import importlib.util
from pathlib import Path
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('slip_app',ROOT/'app/app.py')
assert spec and spec.loader
app=importlib.util.module_from_spec(spec)
spec.loader.exec_module(app)

class ProtocolTests(unittest.TestCase):
    def setUp(self):
        self.data=(ROOT/'firmware/build/fixture.txt').read_bytes()

    def test_real_firmware_fixture(self):
        model=app.Model('synthetic replay')
        model.feed(self.data,1.0)
        self.assertEqual(model.errors,0)
        self.assertEqual(model.latest['completed'],1)
        self.assertEqual(model.latest['interruptions'],1)
        self.assertGreater(model.latest['drain_ms'],model.latest['longest_drain_ms'])
        self.assertTrue(model.stale(3.0))
        self.assertFalse(model.stale(1.1))

    def test_fragment_corruption_and_resync(self):
        model=app.Model('test')
        first=self.data.splitlines(keepends=True)[0]
        model.feed(first[:12]+first,1)
        self.assertEqual(len(model.rows),1)
        self.assertGreater(model.errors,0)
        model.feed(b'$SW1,broken*0000\n',2)
        self.assertGreaterEqual(model.errors,2)
        model.feed(b'x'*4000,2)
        self.assertLessEqual(len(model.buffer),256)

    def test_sequence_regression_blocks_until_reconnect(self):
        m=app.Model('synthetic replay')
        m.feed(self.data,1)
        m.feed(self.data,2)
        self.assertTrue(m.blocked)
        self.assertEqual(m.latest['completed'],1)

    def test_export_keeps_provenance_and_refuses_overwrite(self):
        m=app.Model('synthetic replay')
        m.feed(self.data,1)
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'cast.csv'
            m.export(p,4,'disconnected')
            text=p.read_text()
            self.assertIn('jayis1',text)
            self.assertIn('synthetic replay',text)
            self.assertIn('disconnected',text)
            self.assertIn('True',text)
            with self.assertRaises(FileExistsError): m.export(p,4,'offline')

    def test_fragment_at_every_boundary(self):
        frame=self.data.splitlines(keepends=True)[0]
        for cut in range(len(frame)+1):
            m=app.Model('split fixture')
            m.feed(frame[:cut],1)
            m.feed(frame[cut:],1)
            self.assertEqual(len(m.rows),1)
            self.assertEqual(m.errors,0)

    def test_counter_rollback_with_forward_timestamps(self):
        m=app.Model('restart fixture')
        def frame(seq,uptime,rejected):
            values=[seq,uptime,1,0,5,5,1,0,0,0,0,0,0,0,rejected,0,0]
            payload=('$SW1,'+','.join(map(str,values))).encode()
            return payload+f'*{app.binascii.crc_hqx(payload,0xffff):04X}\n'.encode()
        m.feed(frame(4,1250,1),1)
        m.feed(frame(5,1500,0),2)
        self.assertTrue(m.blocked)

    def test_legitimate_wrap(self):
        m=app.Model('wrap fixture')
        def frame(seq,uptime):
            values=[seq,uptime,1,0,5,5,1,0,0,0,0,0,0,0,0,0,0]
            payload=('$SW1,'+','.join(map(str,values))).encode()
            return payload+f'*{app.binascii.crc_hqx(payload,0xffff):04X}\n'.encode()
        m.feed(frame(0xffffffff,0xffffff00),1)
        m.feed(frame(0,0x10),2)
        self.assertFalse(m.blocked)
        self.assertEqual(len(m.rows),2)

if __name__=='__main__': unittest.main()
