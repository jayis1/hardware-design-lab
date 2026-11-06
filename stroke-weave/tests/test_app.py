# Author: jayis1. SPDX-License-Identifier: MIT
import binascii
import os
from pathlib import Path
import sys
import tempfile
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'app'))
from model import Session, FIELDS, validate
from transport import Decoder, Serial, load_replay
ROOT = Path(__file__).resolve().parents[1]
def frame(row):
    body = ('TW1,' + ','.join(str(row[x]) for x in FIELDS)).encode()
    return body + ('*%04X\n' % binascii.crc_hqx(body, 65535)).encode()
class Tests(unittest.TestCase):
    def setUp(self):
        self.data = (ROOT / 'firmware/build/demo.sw1').read_bytes()
        self.rows = Decoder().feed(self.data, strict=True)
    def test_real_encoder(self):
        s = load_replay(ROOT / 'firmware/build/demo.sw1')
        r = s.rows[-1]
        self.assertEqual((r['a_deliveries'], r['b_deliveries'], r['ties']), (2,3,1))
        self.assertEqual((r['alternating'], r['repeated'], r['a_canceled']), (1,1,1))
        self.assertFalse(s.fresh())
    def test_corruption_and_resync(self):
        d = Decoder()
        rows = d.feed(self.data[:17] + self.data)
        self.assertEqual(rows, self.rows)
        self.assertGreater(d.errors, 0)
        with self.assertRaises(ValueError): Decoder().feed(b'junk\n'+self.data, strict=True)
    def test_tail(self):
        d = Decoder(); d.feed(self.data+b'TW1,')
        with self.assertRaises(ValueError): d.finish()
    def test_impossible_frames(self):
        for name, value in [('a_deliveries', 99), ('peak', 99), ('ties', 50), ('anchor', 1), ('a_last_ms', 1)]:
            r = self.rows[-1].copy(); r[name] = value
            with self.subTest(name=name), self.assertRaises(ValueError):
                Decoder().feed(frame(r), strict=True)
    def test_counter_rollback(self):
        s = Session('replay'); s.accept(self.rows[-1])
        r = self.rows[0].copy(); r['now'] = 1000; r['sequence'] = 1000
        with self.assertRaises(ValueError): s.accept(r)
    def test_no_teleport_duration(self):
        active = next(r for r in self.rows if r['a_phase'] == 3)
        s = Session('replay'); s.accept(active)
        r = active.copy(); r['now'] += 1; r['sequence'] += 1; r['a_age'] += 2
        with self.assertRaises(ValueError): s.accept(r)
    def test_completion_cannot_backdate(self):
        active = next(r for r in self.rows if r['a_phase'] == 3)
        completed = next(r for r in self.rows if r['a_deliveries'] == 1)
        r = completed.copy(); r['a_last_ms'] = 500
        validate(r)
        s = Session('replay'); s.accept(active)
        with self.assertRaises(ValueError): s.accept(r)
    def test_tie_cannot_retain_order(self):
        previous = next(r for r in self.rows if r['a_phase'] == 3 and r['b_phase'] == 3)
        tied = next(r for r in self.rows if r['ties'] == 1)
        r = tied.copy(); r['anchor'] = 1; r['streak'] = 1
        validate(r)
        s = Session('replay'); s.accept(previous)
        with self.assertRaises(ValueError): s.accept(r)
    def test_repeated_delivery_requires_refill(self):
        p = next(r for r in self.rows if r['a_deliveries'] == 1)
        r = p.copy(); r['now'] += 100; r['sequence'] += 100
        r['a_starts'] += 1; r['a_deliveries'] += 1
        r.update(repeated=1, streak=2, peak=2)
        s = Session('replay'); s.accept(p)
        with self.assertRaises(ValueError): s.accept(r)
    def test_one_delivery_has_exact_order(self):
        p = next(r for r in self.rows if r['sequence'] == 50)
        r = next(r for r in self.rows if r['sequence'] == 65).copy()
        r.update(alternating=0, repeated=1, anchor=1, streak=2, peak=2)
        s = Session('replay'); s.accept(p)
        with self.assertRaises(ValueError): s.accept(r)
    def test_abandonment_requires_halt(self):
        r = self.rows[0].copy(); r['a_starts'] = r['a_abandoned'] = 1
        with self.assertRaises(ValueError): validate(r)
    def test_halt_cannot_invent_retained_age(self):
        p = next(r for r in self.rows if r['a_phase'] == 3)
        for outcome in ('canceled','abandoned'):
            r = p.copy(); r['now'] += 1; r['sequence'] += 1
            r.update(state=1, fault=16, a_phase=0, b_phase=0, a_age=54321)
            r['a_'+outcome] += 1
            s = Session('replay'); s.accept(p)
            with self.subTest(outcome=outcome), self.assertRaises(ValueError): s.accept(r)
    def test_freshness_and_export(self):
        s = Session('live'); s.connected = True
        for r in self.rows: s.accept(r, r['now']/1000)
        self.assertTrue(s.fresh(self.rows[-1]['now']/1000))
        self.assertFalse(s.fresh(10))
        s.connected = False; self.assertFalse(s.fresh(0.2))
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp)/'result.csv'; s.export(path)
            self.assertIn('jayis1,live,False', path.read_text())
            self.assertEqual(path.stat().st_mode & 0o777, 0o600)
            with self.assertRaises(FileExistsError): s.export(path)
    def test_backlog(self):
        s = Session('live'); s.accept(self.rows[0], 0)
        with self.assertRaises(ValueError): s.accept(self.rows[1], 5)
    def test_nonregular_replay(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp)/'fifo'; os.mkfifo(path)
            with self.assertRaises(ValueError): load_replay(path)
    def test_serial_restoration(self):
        import pty, termios
        master, slave = pty.openpty()
        original = termios.tcgetattr(slave)
        serial = Serial(os.ttyname(slave))
        os.write(master, self.data)
        import time; time.sleep(0.01)
        self.assertEqual(Decoder().feed(serial.read()), self.rows)
        serial.close(); self.assertEqual(termios.tcgetattr(slave), original)
        os.close(master); os.close(slave)
if __name__ == '__main__': unittest.main()
