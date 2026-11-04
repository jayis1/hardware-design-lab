# Author: jayis1. SPDX-License-Identifier: MIT
import binascii
import os
from pathlib import Path
import pty
import sys
import tempfile
import termios
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'app'))
from model import FIELDS, Session, validate
from transport import Decoder, Serial, load_replay

def encoded(row):
    body=('DL1,'+','.join(str(row[k]) for k in FIELDS)).encode()
    return body+b'*'+f'{binascii.crc_hqx(body,65535):04X}\n'.encode()

class Tests(unittest.TestCase):
    def setUp(self):
        self.path=Path(__file__).resolve().parents[1]/'firmware/build/demo.dl1'
        self.rows=Decoder().feed(self.path.read_bytes(),strict=True)
    def test_real_fixture(self):
        s=load_replay(self.path)
        self.assertEqual(s.latest['completed'],3)
        self.assertEqual(s.latest['full'],2)
        self.assertEqual(s.latest['partial'],1)
        self.assertEqual(s.latest['disturbed'],1)
    def test_bad_frame(self):
        frame=encoded(self.rows[-1]); d=Decoder()
        self.assertFalse(d.feed(frame.replace(b'DL1,',b'DL1,9',1)))
        self.assertEqual(d.errors,1)
        with self.assertRaises(ValueError): Decoder().feed(frame[:-2]+b'Z\n',True)
    def test_resync(self):
        d=Decoder(); frame=encoded(self.rows[-1])
        self.assertEqual(len(d.feed(b'partial'+frame[:30]+frame)),1)
        self.assertEqual(d.errors,2)
    def test_tail(self):
        d=Decoder(); d.feed(encoded(self.rows[-1])[:-1])
        with self.assertRaises(ValueError): d.finish()
    def test_invariants(self):
        for key,value in [('seen',0),('offset_b',0),('full',3),('rebound',8),('age',10001)]:
            row=dict(self.rows[-1]); row[key]=value
            with self.assertRaises(ValueError,msg=key): validate(row)
    def test_frozen_result(self):
        row=next(r for r in self.rows if r['outcome']==1)
        s=Session('replay'); s.accept(row)
        changed=dict(row, now=row['now']+100, samples=row['samples']+100)
        changed['offset_b']+=1
        with self.assertRaises(ValueError): s.accept(changed)
    def test_duplicate(self):
        s=Session('replay'); s.accept(self.rows[0]); s.accept(self.rows[0])
        self.assertEqual(len(s.rows),1)
        changed=dict(self.rows[0],tx_errors=1)
        with self.assertRaises(ValueError): s.accept(changed)
    def test_disturbed_growth_requires_completion(self):
        row=next(r for r in self.rows if r['outcome']==1)
        s=Session('replay'); s.accept(row)
        changed=dict(row,now=row['now']+100,samples=row['samples']+100,disturbed=1)
        with self.assertRaises(ValueError): s.accept(changed)
    def test_private_export(self):
        s=load_replay(self.path)
        with tempfile.TemporaryDirectory() as directory:
            path=Path(directory)/'out.csv'
            s.export(path,False,2)
            self.assertEqual(path.stat().st_mode&0o777,0o600)
            self.assertIn('jayis1',path.read_text())
            self.assertIn('replay',path.read_text())
            with self.assertRaises(FileExistsError): s.export(path,False,2)
    def test_serial_restore(self):
        master,slave=pty.openpty(); original=termios.tcgetattr(slave)
        try:
            port=Serial(os.ttyname(slave)); os.write(master,b'DL1,test\n')
            import select
            select.select([port.fd],[],[],1)
            self.assertEqual(port.read(),b'DL1,test\n')
            port.close(); self.assertEqual(termios.tcgetattr(slave),original)
        finally: os.close(master); os.close(slave)
    def test_non_tty(self):
        with self.assertRaises((OSError,termios.error)): Serial('/dev/null')
    def test_replay_limits(self):
        with tempfile.TemporaryDirectory() as directory:
            fifo=Path(directory)/'fifo'; os.mkfifo(fifo)
            with self.assertRaises(ValueError): load_replay(fifo)
    def test_live_pause(self):
        s=Session('live'); s.accept(self.rows[0],0)
        with self.assertRaises(ValueError): s.accept(self.rows[1],5)
    def test_counter_rollback(self):
        s=Session('replay'); s.accept(self.rows[-1])
        row=dict(self.rows[-1]); row['now']+=100; row['samples']-=1
        with self.assertRaises(ValueError): s.accept(row)

if __name__=='__main__': unittest.main()
