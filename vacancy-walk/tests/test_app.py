# Author: jayis1. SPDX-License-Identifier: MIT
import binascii
import os
from pathlib import Path
import pty
import sys
import tempfile
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'app'))
from model import Session, FIELDS, validate
from transport import Decoder, Serial, load_replay

def frame(row):
    body=('VW1,'+','.join(str(row[k]) for k in FIELDS)).encode()
    return body+b'*'+f'{binascii.crc_hqx(body,65535):04X}\n'.encode()

class Tests(unittest.TestCase):
    def setUp(self):
        self.path=Path(__file__).resolve().parents[1]/'firmware/build/demo.vw1'
        self.session=load_replay(self.path)
    def test_real_encoder(self):
        row=self.session.rows[-1]
        self.assertEqual((row['moves'],row['canceled'],row['abandoned']),(2,1,1))
        self.assertEqual(row['state'],3)
    def test_framing(self):
        data=self.path.read_bytes()
        decoder=Decoder(); rows=[]
        for byte in data: rows+=decoder.feed(bytes([byte]),True)
        decoder.finish(); self.assertEqual(len(rows),len(self.session.rows))
        with self.assertRaises(ValueError): Decoder().feed(data.replace(b'VW1,',b'VW1,9',1),True)
        decoder=Decoder(); decoder.feed(data+b'VW1,')
        with self.assertRaises(ValueError): decoder.finish()
        decoder=Decoder(); self.assertTrue(decoder.feed(b'badVW1,0'+data))
        self.assertGreater(decoder.errors,0)
    def test_invariants(self):
        row=dict(self.session.rows[-1]); row['clockwise']+=1
        with self.assertRaises(ValueError): Decoder().feed(frame(row),True)
        row=dict(self.session.rows[-1]); row['starts']+=1
        with self.assertRaises(ValueError): validate(row)
    def test_reset_and_terminal_freeze(self):
        last=dict(self.session.rows[-1]); last['sequence']+=1; last['now']+=1
        self.session.accept(last)
        changed=dict(last); changed['sequence']+=1; changed['now']+=1
        changed['vacancy']=(changed['vacancy']+1)%4
        with self.assertRaises(ValueError): self.session.accept(changed)
        s=Session('replay'); s.accept(last)
        last['moves']=0
        with self.assertRaises(ValueError): s.accept(last)
    def test_empty_tail_and_fifo(self):
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'record'; p.write_bytes(b'')
            with self.assertRaises(ValueError): load_replay(p)
            p.write_bytes(self.path.read_bytes()+b'VW1,')
            with self.assertRaises(ValueError): load_replay(p)
            p.unlink(); os.mkfifo(p)
            with self.assertRaises(ValueError): load_replay(p)
    def test_export(self):
        with tempfile.TemporaryDirectory() as d:
            path=Path(d)/'out.csv'; self.session.export(path,False)
            text=path.read_text(); self.assertIn('jayis1',text); self.assertIn('replay',text)
            self.assertEqual(path.stat().st_mode & 0o777,0o600)
            with self.assertRaises(FileExistsError): self.session.export(path,False)
    def test_serial(self):
        master,slave=pty.openpty()
        serial=Serial(os.ttyname(slave))
        try:
            os.write(master,b'hello\n')
            self.assertEqual(serial.read(),b'hello\n')
            os.close(master); master=None
            with self.assertRaises(OSError): serial.read()
        finally:
            serial.close(); os.close(slave)
            if master is not None: os.close(master)
    def test_live_pause(self):
        s=Session('live'); rows=self.session.rows
        s.accept(rows[0],1.0); s.accept(rows[1],1.01)
        self.assertTrue(s.fresh(1.02,True))
        with self.assertRaises(ValueError): s.accept(rows[2],5.0)
        self.assertFalse(s.fresh(5.0,True))

if __name__=='__main__': unittest.main()
