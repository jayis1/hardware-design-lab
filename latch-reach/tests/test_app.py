# Author: jayis1. SPDX-License-Identifier: MIT
import copy
import os
from pathlib import Path
import pty
import sys
import tempfile
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'app'))
from model import Session, validate
from transport import Decoder, Serial, load_replay

class AppTests(unittest.TestCase):
    def setUp(self):
        self.data = (Path(__file__).resolve().parents[1] / 'firmware/build/demo.lr1').read_bytes()
        self.rows = Decoder().feed(self.data, strict=True)
    def test_real_encoder_matrix(self):
        s = Session('synthetic')
        for row in self.rows:
            s.accept(row)
        self.assertEqual(s.latest['completed'], 8)
        self.assertEqual(sum(s.latest['b%d'%i] for i in range(12)), 8)
        self.assertEqual(s.latest['fault'], 0)
    def test_framing_and_crc(self):
        d = Decoder()
        self.assertEqual(len(d.feed(b'LR1,partial'+self.data)), len(self.rows))
        self.assertEqual(d.errors, 1)
        with self.assertRaises(ValueError):
            Decoder().feed(self.data.replace(b'*',b'*Z',1), strict=True)
        d = Decoder()
        d.feed(self.data+b'LR1,')
        with self.assertRaises(ValueError): d.finish()
    def test_impossible_and_rollback(self):
        row = copy.copy(self.rows[-1])
        row['b0'] += 1
        with self.assertRaises(ValueError): validate(row)
        s = Session('synthetic')
        s.accept(self.rows[-1])
        with self.assertRaises(ValueError): s.accept(self.rows[0])
        self.assertTrue(s.error)
    def test_export_and_replay(self):
        s = Session('synthetic')
        for row in self.rows: s.accept(row)
        with tempfile.TemporaryDirectory() as td:
            path = Path(td)/'out.csv'
            s.export(path, False, 2)
            self.assertIn('jayis1',path.read_text())
            self.assertIn('synthetic',path.read_text())
            self.assertEqual(path.stat().st_mode & 0o777, 0o600)
            with self.assertRaises(FileExistsError): s.export(path,False,2)
            fixture = Path(td)/'test.lr1'
            fixture.write_bytes(self.data)
            self.assertEqual(load_replay(fixture).latest['completed'],8)
            fixture.write_bytes(self.data+b'LR1,')
            with self.assertRaises(ValueError): load_replay(fixture)
            fifo=Path(td)/'fifo'
            os.mkfifo(fifo)
            with self.assertRaises(ValueError): load_replay(fifo)
    def test_serial(self):
        master,slave = pty.openpty()
        try:
            import termios
            before=termios.tcgetattr(slave)
            serial=Serial(os.ttyname(slave))
            os.write(master,self.data)
            import time
            time.sleep(.02)
            self.assertTrue(serial.read().startswith(b'LR1,'))
            serial.close()
            self.assertEqual(before,termios.tcgetattr(slave))
        finally:
            os.close(master);os.close(slave)

    def test_sparse_acquisition_gap_is_not_telemetry_loss(self):
        a=dict(self.rows[0]); b=dict(a)
        b['now']+=100; b['sequence']+=1
        s=Session('synthetic'); s.accept(a)
        with self.assertRaises(ValueError): s.accept(b)

    def active_row(self,phase=3,seen=0):
        r=dict(self.rows[0])
        r.update(sequence=1000,now=1000,phase=phase,starts=1,completed=0,
                 rejected=0,seen=seen,quiet=0,age=10,raw=1,qualified=1)
        for i in range(12): r['b%d'%i]=0
        return r

    def test_completion_needs_remaining_window(self):
        a=self.active_row(); b=dict(a)
        b.update(sequence=1001,now=1001,phase=0,completed=1,age=0,b0=1)
        s=Session('synthetic'); s.accept(a)
        with self.assertRaises(ValueError): s.accept(b)

    def test_return_cannot_gain_checkpoints(self):
        a=self.active_row(); b=dict(a)
        b.update(sequence=1001,now=1001,age=11,quiet=1,seen=3)
        s=Session('synthetic'); s.accept(a)
        with self.assertRaises(ValueError): s.accept(b)

    def test_completion_cannot_forget_travel_checkpoint(self):
        a=self.active_row(2,1); b=dict(a)
        b.update(sequence=1101,now=1101,phase=0,completed=1,age=0,seen=0,b0=1)
        s=Session('synthetic'); s.accept(a)
        with self.assertRaises(ValueError): s.accept(b)

if __name__ == '__main__': unittest.main()
