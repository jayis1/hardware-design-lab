# Author: jayis1. SPDX-License-Identifier: MIT
import binascii
import os
from pathlib import Path
import sys
import tempfile
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'app'))
from model import FIELDS, Session, validate
from transport import Decoder, load_replay, Serial
ROOT = Path(__file__).resolve().parents[1]

def encode(row):
    body = ('RW1,' + ','.join(str(row[k]) for k in FIELDS)).encode()
    return body + ('*%04X\n' % binascii.crc_hqx(body, 65535)).encode()

class ApplicationTests(unittest.TestCase):
    def setUp(self):
        self.data = (ROOT/'firmware/build/demo.rw1').read_bytes()
        self.rows = Decoder().feed(self.data, strict=True)
    def test_real_encoder(self):
        self.assertGreater(len(self.rows), 3)
        self.assertEqual(self.rows[-1]['good'], 2)
        self.assertEqual(self.rows[-1]['same'], 1)
        self.assertEqual(self.rows[-1]['opposite'], 1)
        self.assertEqual(self.rows[-1]['last'], 48)
        session = load_replay(ROOT/'firmware/build/demo.rw1')
        self.assertEqual(session.provenance, 'replay')
        self.assertEqual(session.status(0), 'REPLAY - NOT LIVE')
    def test_fragment_recovery(self):
        d = Decoder(); rows=[]
        for byte in self.data:
            rows += d.feed(bytes([byte]), strict=True)
        d.finish(); self.assertEqual(rows,self.rows)
        d=Decoder(); line=encode(self.rows[0])
        self.assertEqual(d.feed(line[:12]+line),[self.rows[0]])
        self.assertEqual(d.errors,1)
        with self.assertRaises(ValueError):
            Decoder().feed(line.replace(b'RW1,',b'RW1,9'),strict=True)
        d=Decoder(); d.feed(line[:-1])
        with self.assertRaises(ValueError): d.finish()
    def test_semantics(self):
        row=dict(self.rows[-1]); row['starts']+=1
        with self.assertRaises(ValueError): Decoder().feed(encode(row),strict=True)
        row=dict(self.rows[-1]); row['last_ms']=120000
        with self.assertRaises(ValueError): validate(row)
        row=dict(self.rows[-1]); row['last_output']=0
        with self.assertRaises(ValueError): validate(row)
        row=dict(self.rows[-1]); row['same']=2; row['opposite']=0
        with self.assertRaises(ValueError): validate(row)
    def test_stale_and_duplicate(self):
        s=Session('live'); s.connected=True
        s.accept(self.rows[0],0); s.accept(self.rows[1],.1)
        self.assertEqual(s.status(.2),'ADVANCING')
        s.accept(self.rows[1],.9)
        self.assertEqual(s.status(1.2),'STALE - RECONNECT')
        self.assertFalse(s.accept(self.rows[2],1.3))
    def test_regression_and_change(self):
        s=Session('replay'); s.accept(self.rows[-1])
        with self.assertRaises(ValueError): s.accept(self.rows[0])
        s=Session('replay'); s.accept(self.rows[-1])
        row=dict(self.rows[-1]); row['last']=49; row['maximum']=96
        # Same timestamp with a different, individually valid payload is forbidden.
        with self.assertRaises(ValueError): s.accept(row)
    def test_sparse_impossibility(self):
        a=self.rows[-1]; b=dict(a)
        b['samples']+=1; b['now']+=1; b['good']+=1; b['starts']+=1
        b['opposite']+=1; b['total']+=48
        s=Session('replay'); s.accept(a)
        with self.assertRaises(ValueError): s.accept(b)
    def test_export_and_bounds(self):
        s=load_replay(ROOT/'firmware/build/demo.rw1')
        with tempfile.TemporaryDirectory() as tmp:
            p=Path(tmp)/'capture.csv'; s.export(p,0)
            self.assertEqual(p.stat().st_mode & 0o777,0o600)
            text=p.read_text(); self.assertIn('jayis1',text); self.assertIn('replay',text)
            with self.assertRaises(FileExistsError): s.export(p,0)
            bad=Path(tmp)/'bad'; bad.write_bytes(self.data+b'RW1,')
            with self.assertRaises(ValueError): load_replay(bad)
            fifo=Path(tmp)/'fifo'; os.mkfifo(fifo)
            with self.assertRaises(ValueError): load_replay(fifo)
    def test_serial_configuration_restored(self):
        import pty,termios
        master,slave=pty.openpty(); initial=termios.tcgetattr(slave)
        serial=Serial(os.ttyname(slave)); serial.close()
        self.assertEqual(termios.tcgetattr(slave),initial)
        os.close(master); os.close(slave)

if __name__=='__main__': unittest.main()
