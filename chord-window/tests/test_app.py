# Author: jayis1. SPDX-License-Identifier: MIT
import importlib.util
import pathlib
import unittest
import tempfile
import os
import pty
import time
import tkinter as tk
from unittest import mock
import csv
import io
ROOT = pathlib.Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('chord', ROOT/'app/chord.py')
assert spec is not None and spec.loader is not None
app = importlib.util.module_from_spec(spec)
spec.loader.exec_module(app)

def frame(values):
    body = 'CW1,' + ','.join(map(str, values))
    return (body + '*%04X\n' % app.crc(body.encode())).encode()

BASE = [200, 0, 0, 0, 0, 1, 1, 3, 20, 40, 10, 70]

class ModelTests(unittest.TestCase):
    def test_real_fixture(self):
        m = app.Model('synthetic')
        m.feed((ROOT/'firmware/build/fixture.txt').read_bytes(), 0)
        m.finish()
        self.assertEqual(len(m.rows), 1)
        self.assertEqual(m.rows[0][9], 40)
        for filename, reason in [('retap',2),('gap',4),('timeout',3),('rolling',1)]:
            m = app.replay(ROOT/f'firmware/build/{filename}.txt')
            self.assertEqual(m.errors,0)
            self.assertEqual(m.rows[0][6],reason)
    def test_framing_and_tail(self):
        m = app.Model('synthetic')
        m.feed(b'CW1,broken' + frame(BASE), 0)
        self.assertEqual(len(m.rows), 1)
        m.feed(b'CW1,tail', 1)
        with self.assertRaises(ValueError): m.finish()
    def test_duplicate_not_fresh(self):
        m = app.Model('live')
        m.feed(frame(BASE), 1); m.feed(frame(BASE), 10)
        self.assertEqual(m.received, 1)
        bad = BASE.copy(); bad[9] = 20
        m.feed(frame(bad), 11)
        self.assertGreater(m.errors, 0)
    def test_reset_and_contradiction(self):
        m = app.Model('live'); m.feed(frame(BASE), 1)
        b = BASE.copy(); b[0] += 50; b[8:12] = [10,50,10,70]
        m.feed(frame(b), 2)
        self.assertTrue(m.blocked)
        m = app.Model('live'); m.feed(frame(BASE), 1)
        b = BASE.copy(); b[0] = 1
        m.feed(frame(b), 2); self.assertTrue(m.blocked)
    def test_impossible_and_lost(self):
        m = app.Model('live')
        b = BASE.copy(); b[9] = 99
        m.feed(frame(b), 0); self.assertEqual(m.errors, 1)
        m.feed(frame(BASE), 1)
        b = BASE.copy(); b[0] += 500; b[5] = 4
        m.feed(frame(b), 2); self.assertEqual(m.missed, 2)
    def test_export_and_replay(self):
        with tempfile.TemporaryDirectory() as d:
            p = pathlib.Path(d)/'out.csv'
            m = app.Model('synthetic'); m.feed(frame(BASE), 0)
            app.export(m, p, 10, False)
            text = p.read_text()
            self.assertIn('jayis1', text)
            self.assertIn('synthetic', text)
            with self.assertRaises(FileExistsError): app.export(m,p,10,False)
            fifo = pathlib.Path(d)/'fifo'; os.mkfifo(fifo)
            with self.assertRaises(ValueError): app.replay(fifo)
    def test_empty_metadata_and_recording(self):
        with tempfile.TemporaryDirectory() as d:
            p = pathlib.Path(d)/'empty.txt'; p.write_bytes(b'')
            with self.assertRaises(ValueError): app.replay(p)
            idle = [200,1,0,0,0,0,0,0,0,0,0,0]
            p.write_bytes(frame(idle))
            self.assertIsNotNone(app.replay(p).last)
            m = app.Model('live'); m.errors = 3; m.blocked = True
            out = pathlib.Path(d)/'empty.csv'; app.export(m,out,1,False)
            rows = list(csv.DictReader(io.StringIO(out.read_text())))
            self.assertEqual(rows[0]['record_type'], 'session')
            self.assertEqual(rows[0]['parser_errors'], '3')
            self.assertEqual(rows[0]['blocked'], 'True')
    def test_buffered_hangup(self):
        serial = app.Serial.__new__(app.Serial)
        serial.fd = 123
        serial.poller = mock.Mock()
        serial.poller.poll.return_value = [(123, app.select.POLLIN | app.select.POLLHUP)]
        with mock.patch.object(app.os,'read',side_effect=[b'a'*8192,b'b'*808,b'']):
            data, hungup = serial.read()
        self.assertEqual(len(data),9000)
        self.assertTrue(hungup)
    def test_valid_contradiction_counters_and_wrap(self):
        for index in [0,3,4,5]:
            m = app.Model('live')
            base = BASE.copy(); base[3:6] = [2,2,2]
            m.feed(frame(base),1)
            b = base.copy(); b[0] += 50; b[index] = base[index]-1
            m.feed(frame(b),2); self.assertTrue(m.blocked)
        m = app.Model('live'); m.feed(frame(BASE),1)
        b = BASE.copy(); b[2] = 1
        m.feed(frame(b),2); self.assertTrue(m.blocked)
        m = app.Model('live')
        b = BASE.copy(); b[0] = 0xfffffff0
        m.feed(frame(b),1); b[0] = 20; m.feed(frame(b),2)
        self.assertFalse(m.blocked); self.assertEqual(m.received,2)
    def test_history_bound(self):
        m = app.Model('synthetic')
        for i in range(1,1003):
            b = BASE.copy(); b[0] = i*200; b[5] = i
            m.feed(frame(b),i)
        self.assertEqual(len(m.rows),1000)
        self.assertEqual(m.evicted,2)
    def test_actual_ui_serial(self):
        master, slave = pty.openpty()
        root = tk.Tk(); ui = app.Window(root)
        try:
            ui.path.set(os.ttyname(slave)); ui.connect()
            os.write(master, frame(BASE))
            end = time.monotonic()+0.2
            while time.monotonic()<end:
                root.update(); time.sleep(0.01)
            self.assertEqual(len(ui.model.rows), 1)
            self.assertEqual(len(ui.table.get_children()), 1)
            for i in range(4): ui.tabs.select(i); root.update()
            self.assertIn('1 / 1', ui.summary.get())
            os.close(master); master = None
            end = time.monotonic()+0.2
            while time.monotonic()<end:
                root.update(); time.sleep(0.01)
            self.assertIsNone(ui.serial)
            self.assertEqual(ui.model.provenance, 'live')
        finally:
            ui.close(); os.close(slave)
            if master is not None: os.close(master)

if __name__ == '__main__': unittest.main()
