# Author: jayis1. SPDX-License-Identifier: MIT
import binascii
import os
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'app'))
from model import FIELDS, Session, validate
from transport import Decoder, Serial, load_replay
ROOT=Path(__file__).resolve().parents[1]

def frame(row):
    body=('SS1,'+','.join(str(row[k]) for k in FIELDS)).encode()
    return body+f'*{binascii.crc_hqx(body,65535):04X}\n'.encode()

class AppTests(unittest.TestCase):
    def setUp(self):
        self.data=(ROOT/'firmware/build/demo.ss1').read_bytes()
        self.rows=Decoder().feed(self.data,strict=True)
    def test_real_encoder(self):
        s=load_replay(ROOT/'firmware/build/demo.ss1')
        self.assertEqual(len(s.rows),42)
        self.assertEqual(s.rows[-1]['forward'],16)
        self.assertEqual(s.rows[-1]['backward'],2)
        self.assertEqual(s.rows[-1]['skipped'],1)
        self.assertFalse(s.fresh())
    def test_fragments(self):
        d=Decoder(); rows=[]
        for byte in self.data: rows.extend(d.feed(bytes([byte]),strict=True))
        d.finish(); self.assertEqual(rows,self.rows)
        d=Decoder(); self.assertEqual(d.feed(b'SS1,broken'+frame(self.rows[0])),[self.rows[0]])
        self.assertGreater(d.errors,0)
    def test_invalid_wire(self):
        for data in (b'PB1,1*0000\n',self.data[:25]+b'\n',b'SS1,'+b'0'*800+b'\n'):
            with self.assertRaises(ValueError): Decoder().feed(data,strict=True)
        d=Decoder(); d.feed(self.data[:-1],strict=True)
        with self.assertRaises(ValueError): d.finish()
    def test_partition_and_bounds(self):
        for key,value in [('dwell0',999999),('longest0',99999),('sector',16),('valid',0),('fault',4),('sequence',0)]:
            row=dict(self.rows[0]); row[key]=value
            with self.assertRaises(ValueError): Decoder().feed(frame(row),strict=True)
    def test_sparse_migration(self):
        s=Session('replay'); s.accept(self.rows[0])
        row=dict(self.rows[1]); row['dwell0']-=1; row['dwell1']+=1; row['longest1']=1
        row['longest0']-=1; row['run']-=1
        # New sector dwell without any motion is impossible.
        validate(row)
        with self.assertRaises(ValueError): s.accept(row)
    def test_frozen_halt(self):
        s=Session('replay'); s.accept(self.rows[-2])
        row=dict(self.rows[-1]); row['forward']+=1
        with self.assertRaises(ValueError): s.accept(row)
    def test_duplicates_and_reset(self):
        s=Session('replay'); s.accept(self.rows[1]); s.accept(self.rows[1])
        self.assertEqual(len(s.rows),1)
        with self.assertRaises(ValueError): s.accept(self.rows[0])
        self.assertTrue(s.warning)
    def test_export(self):
        with tempfile.TemporaryDirectory() as folder:
            p=Path(folder)/'e.csv'; s=Session('replay'); s.export(p)
            self.assertIn('jayis1',p.read_text()); self.assertIn('replay',p.read_text())
            self.assertEqual(p.stat().st_mode & 0o777,0o600)
            with self.assertRaises(FileExistsError): s.export(p)
    def test_freshness(self):
        s=Session('live'); s.connected=True
        with patch('model.time.monotonic',return_value=10): s.accept(self.rows[0])
        with patch('model.time.monotonic',return_value=10.05):
            s.accept(self.rows[1]); self.assertTrue(s.fresh())
        with patch('model.time.monotonic',return_value=13): self.assertFalse(s.fresh())
        with patch('model.time.monotonic',return_value=13.1):
            with self.assertRaises(ValueError): s.accept(self.rows[2])
        self.assertFalse(s.fresh())
    def test_serial_failures_and_pty(self):
        import pty, termios
        master,slave=pty.openpty(); original=termios.tcgetattr(slave)
        try:
            with patch('transport.termios.tcflush',side_effect=OSError('injected')):
                with self.assertRaises(OSError): Serial(os.ttyname(slave))
            self.assertEqual(original,termios.tcgetattr(slave))
            link=Serial(os.ttyname(slave)); os.write(master,frame(self.rows[0]))
            import select
            select.select([link.fd],[],[],1)
            self.assertEqual(Decoder().feed(link.read()),[self.rows[0]])
            link.close(); self.assertEqual(original,termios.tcgetattr(slave))
            with self.assertRaises(PermissionError):
                with patch('transport.os.open',side_effect=PermissionError()): Serial('/denied')
        finally: os.close(master); os.close(slave)

if __name__=='__main__': unittest.main()
