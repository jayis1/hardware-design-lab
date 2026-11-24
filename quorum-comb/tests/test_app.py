# Author: jayis1. SPDX-License-Identifier: MIT
import binascii
import os
from pathlib import Path
import sys
import tempfile
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'app'))
from model import FIELDS, Session, validate
from transport import Decoder, Serial, load_replay
ROOT = Path(__file__).resolve().parents[1]

def encode(row):
    body = ('QC1,' + ','.join(str(row[k]) for k in FIELDS)).encode()
    return body + f'*{binascii.crc_hqx(body,65535):04X}\n'.encode()

class Tests(unittest.TestCase):
    def setUp(self):
        self.data = (ROOT / 'firmware/build/demo.qc1').read_bytes()
        self.rows = Decoder().feed(self.data, strict=True)
    def test_real_encoder_replay(self):
        s = load_replay(ROOT / 'firmware/build/demo.qc1')
        self.assertEqual(s.rows[-1]['samples'], 1600)
        self.assertEqual(s.source, 'replay')
        self.assertEqual(len(s.rows), 160)
    def test_crc_and_tail(self):
        with self.assertRaises(ValueError): Decoder().feed(self.data.replace(b'QC1,10',b'QC1,11',1),True)
        d = Decoder(); d.feed(self.data[:-1])
        with self.assertRaises(ValueError): d.finish()
    def test_resynchronization(self):
        d = Decoder()
        rows = d.feed(b'QC1,20,broken' + self.data)
        self.assertEqual(len(rows), len(self.rows))
        self.assertEqual(d.errors,1)
    def test_semantic_corruption(self):
        row = self.rows[-1].copy(); row['total0'] = row['voter_known']+1
        with self.assertRaises(ValueError): Decoder().feed(encode(row), True)
    def test_rollback_and_duplicate(self):
        s=Session('replay'); s.accept(self.rows[0]); s.accept(self.rows[0])
        self.assertEqual(len(s.rows),1)
        row=self.rows[1].copy(); row['now']=self.rows[0]['now']
        with self.assertRaises(ValueError): s.accept(row)
        with self.assertRaises(ValueError): s.accept(self.rows[2])
    def test_credit_cannot_migrate(self):
        a=self.rows[12]; b=self.rows[13].copy()
        b['total0']=a['total0']-1
        s=Session('replay'); s.accept(a)
        with self.assertRaises(ValueError): s.accept(b)
    def test_timestamp_only_not_fresh(self):
        s=Session('live'); s.accept(self.rows[0],0); s.accept(self.rows[1],0.01)
        self.assertTrue(s.fresh(0.02))
        self.assertFalse(s.fresh(3))
        s.connected=False
        self.assertFalse(s.fresh(0.02))
    def test_serial_restore_hangup(self):
        import termios
        m,slave=os.openpty(); path=os.ttyname(slave); old=termios.tcgetattr(slave)
        port=Serial(path); os.write(m,self.data[:100])
        import time; time.sleep(.01)
        self.assertEqual(port.read(),self.data[:100])
        port.close(); self.assertEqual(termios.tcgetattr(slave),old)
        port=Serial(path); os.close(m)
        with self.assertRaises(OSError): port.read()
        port.close(); os.close(slave)
    def test_replay_bounds(self):
        with tempfile.TemporaryDirectory() as directory:
            path=Path(directory)/'pipe'; os.mkfifo(path)
            with self.assertRaises(ValueError): load_replay(path)
            path=Path(directory)/'empty'; path.write_bytes(b'')
            with self.assertRaises(ValueError): load_replay(path)

if __name__ == '__main__': unittest.main()
