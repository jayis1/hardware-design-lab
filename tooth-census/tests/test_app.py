# Author: jayis1. SPDX-License-Identifier: MIT
import binascii
import os
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'app'))
from model import Session, validate, transition, FIELDS
from transport import Decoder, Serial, load_replay

def encode(row):
    body=('TC1,'+','.join(str(row[k]) for k in FIELDS)).encode()
    return body+b'*'+f'{binascii.crc_hqx(body,65535):04X}\n'.encode()

class Contracts(unittest.TestCase):
    def setUp(self):
        self.data=(ROOT/'firmware/build/demo.tc1').read_bytes()
        self.rows=Decoder().feed(self.data,strict=True)
    def test_real_encoder(self):
        session=load_replay(ROOT/'firmware/build/demo.tc1')
        r=session.rows[-1]
        self.assertEqual((r['good'],r['bad'],r['mismatch'],r['total']),(3,1,2,33))
        self.assertEqual((r['minimum'],r['maximum'],r['reference']),(10,12,12))
        self.assertFalse(session.fresh())
    def test_crc_and_partial(self):
        with self.assertRaises(ValueError): Decoder().feed(self.data.replace(b'TC1,',b'TC1,9',1),True)
        decoder=Decoder(); decoder.feed(self.data[:-1])
        with self.assertRaises(ValueError): decoder.finish()
        decoder=Decoder()
        rows=decoder.feed(b'TC1,123'+self.data)
        self.assertEqual(len(rows),len(self.rows)); self.assertEqual(decoder.errors,1)
    def test_valid_crc_impossible(self):
        row=dict(self.rows[-1]); row['total']=999999
        with self.assertRaises(ValueError): Decoder().feed(encode(row),True)
        row=dict(self.rows[-1]); row['starts']+=1
        with self.assertRaises(ValueError): Decoder().feed(encode(row),True)
    def test_frozen_reference_and_result(self):
        a=self.rows[-1]; b=dict(a); b['now']+=100; b['sequence']+=100
        b['reference']=11
        validate(b)
        with self.assertRaises(ValueError): transition(a,b)
        b=dict(a); b['now']+=100; b['sequence']+=100; b['last']=11
        validate(b)
        with self.assertRaises(ValueError): transition(a,b)
    def test_duplicate_and_regression(self):
        s=Session('live'); s.connected=True
        with patch('model.time.monotonic',return_value=10): s.accept(self.rows[0])
        with patch('model.time.monotonic',return_value=10.1): s.accept(self.rows[1])
        with patch('model.time.monotonic',return_value=10.2): s.accept(self.rows[1])
        self.assertEqual(s.last_host,10.1)
        bad=dict(self.rows[1]); bad['raw']^=1
        with self.assertRaises(ValueError): s.accept(bad)
    def test_pause(self):
        s=Session('live'); s.connected=True
        with patch('model.time.monotonic',return_value=10): s.accept(self.rows[0])
        with patch('model.time.monotonic',return_value=13):
            with self.assertRaises(ValueError): s.accept(self.rows[1])
        self.assertTrue(s.warning)
    def test_export_and_replay_limits(self):
        with tempfile.TemporaryDirectory() as d:
            path=Path(d)/'out.csv'; s=load_replay(ROOT/'firmware/build/demo.tc1')
            s.export(path)
            self.assertEqual(path.stat().st_mode&0o777,0o600)
            self.assertIn('jayis1',path.read_text())
            with self.assertRaises(FileExistsError): s.export(path)
            fifo=Path(d)/'fifo'; os.mkfifo(fifo)
            with self.assertRaises(ValueError): load_replay(fifo)
            empty=Path(d)/'empty'; empty.write_text('')
            with self.assertRaises(ValueError): load_replay(empty)
    def test_serial(self):
        import pty, termios
        master,slave=pty.openpty(); saved=termios.tcgetattr(slave)
        port=Serial(os.ttyname(slave))
        os.write(master,self.data[:100])
        import time
        time.sleep(.02)
        self.assertEqual(port.read(),self.data[:100])
        port.close(); self.assertEqual(termios.tcgetattr(slave),saved)
        os.close(master); os.close(slave)

if __name__=='__main__': unittest.main()
