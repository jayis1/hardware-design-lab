#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Synthetic C frames and real Tk/PTY integration; no physical device implied."""
import binascii
import os
from pathlib import Path
import pty
import subprocess
import sys
import tempfile
import termios
import time
import tkinter as tk
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'app'))
from model import Session, FIELDS, MASK, validate
from transport import Decoder, Serial, load_replay
from workbench import Workbench


def encode(row):
    body = ('TH1,' + ','.join(str(row[k]) for k in FIELDS)).encode()
    return body + ('*%04X\n' % binascii.crc_hqx(body, 65535)).encode()


class Tests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data = subprocess.check_output([str(ROOT / 'firmware/build/fixture')])
        cls.rows = Decoder().feed(cls.data, strict=True)

    def test_real_encoder_both_directions(self):
        for origin in ('A', 'B'):
            data = subprocess.check_output([str(ROOT / 'firmware/build/fixture'), origin])
            rows = Decoder().feed(data, strict=True)
            session = Session('replay')
            for r in rows:
                session.accept(r)
            self.assertEqual(session.rows[-1]['result'], 1)
            self.assertFalse(session.live())

    def test_resynchronization_crc_truncation(self):
        first = self.data.splitlines(keepends=True)[0]
        decoder = Decoder()
        self.assertEqual(len(decoder.feed(first[:12] + first)), 1)
        self.assertEqual(decoder.errors, 1)
        decoder = Decoder()
        self.assertFalse(decoder.feed(first.replace(b'TH1,', b'TH1,9', 1)))
        self.assertEqual(decoder.errors, 1)
        decoder.feed(first[:-1])
        with self.assertRaises(ValueError): decoder.finish()

    def test_impossible_valid_crc(self):
        for key, value in [('hold', 499), ('released_ms', MASK), ('finish_ms', 10000), ('origin', 0)]:
            r = dict(self.rows[-1]); r[key] = value
            with self.subTest(key=key), self.assertRaises(ValueError):
                Decoder().feed(encode(r), strict=True)

    def test_session_frozen_terminal_and_reset(self):
        terminal = dict(self.rows[-1])
        for changes in ({'raw': 1}, {'reached_ms': terminal['reached_ms'] + 1}, {'state': 0}):
            session = Session('replay'); session.accept(terminal)
            row = dict(terminal, now=terminal['now'] + 10, sequence=terminal['sequence'] + 10)
            row.update(changes)
            with self.assertRaises(ValueError): session.accept(row)
            self.assertTrue(session.blocked)
        session = Session('replay'); session.accept(terminal)
        with self.assertRaises(ValueError): session.accept(self.rows[0])

    def test_phase_consistency(self):
        ready = next(r for r in self.rows if r['state'] == 1)
        with self.assertRaises(ValueError):
            validate(dict(ready, reached_ms=0))
        end = self.rows[-1]
        with self.assertRaises(ValueError):
            validate(dict(end, released_ms=end['released_ms']-1))

    def test_adjacent_qualification_and_sample_continuity(self):
        push = next(r for r in self.rows if r['state'] == 2)
        row = dict(push, sequence=push['sequence']+1, now=push['now']+1,
                   age=push['age']+1, state=3, raw=6, qualified=6,
                   reached_ms=push['age']+1)
        session = Session('replay'); session.accept(push)
        with self.assertRaises(ValueError): session.accept(row)
        row = dict(push, sequence=push['sequence']+1, now=push['now']+100,
                   age=push['age']+100)
        session = Session('replay'); session.accept(push)
        with self.assertRaises(ValueError): session.accept(row)
        row = dict(self.rows[-1], sequence=push['sequence']+1, now=push['now']+1000)
        session = Session('replay'); session.accept(push)
        with self.assertRaises(ValueError): session.accept(row)

    def test_unreported_samples_are_not_lost_frames(self):
        session = Session('replay')
        session.accept(self.rows[0]); session.accept(self.rows[1])
        self.assertEqual(session.unreported_samples,
                         self.rows[1]['sequence']-self.rows[0]['sequence']-1)
        with tempfile.TemporaryDirectory() as td:
            path = Path(td)/'export.csv'; session.export(path)
            self.assertIn('unreported_samples', path.read_text())
            self.assertNotIn('sequence_losses', path.read_text())

    def test_live_freshness_duplicate_pause(self):
        session = Session('live'); session.connected = True
        session.accept(self.rows[0], 0)
        session.accept(self.rows[1], .01)
        self.assertTrue(session.live(.02))
        session.accept(self.rows[1], 1)
        self.assertFalse(session.live(2.02))
        with self.assertRaises(ValueError): session.accept(self.rows[2], 3)

    def test_replay_export_bounds(self):
        with tempfile.TemporaryDirectory() as td:
            path = Path(td)/'input.th1'; path.write_bytes(self.data)
            session = load_replay(path)
            output = Path(td)/'out.csv'; session.export(output, 2)
            self.assertEqual(output.stat().st_mode & 0o777, 0o600)
            self.assertIn('jayis1,metadata,replay', output.read_text())
            with self.assertRaises(FileExistsError): session.export(output)
            path.write_bytes(self.data[:-1])
            with self.assertRaises(ValueError): load_replay(path)
            path.write_bytes(b'')
            with self.assertRaises(ValueError): load_replay(path)
            path.write_bytes(b'x' * (2*1024*1024+1))
            with self.assertRaises(ValueError): load_replay(path)
            fifo = Path(td)/'fifo'; os.mkfifo(fifo)
            with self.assertRaises(ValueError): load_replay(fifo)
            Session('none').export(Path(td)/'empty.csv')
            self.assertIn('metadata', (Path(td)/'empty.csv').read_text())

    def test_serial_restoration(self):
        master, slave = pty.openpty()
        saved = termios.tcgetattr(slave)
        port = Serial(os.ttyname(slave)); port.close()
        self.assertEqual(termios.tcgetattr(slave), saved)
        os.close(master); os.close(slave)
        with self.assertRaises((OSError, termios.error)): Serial('/dev/null')

    def test_real_gui_live_replay_export_disconnect(self):
        root = tk.Tk(); root.withdraw(); app = Workbench(root)
        master, slave = pty.openpty()
        saved = termios.tcgetattr(slave)
        try:
            app.connect(os.ttyname(slave))
            with self.assertRaises(ValueError): app.connect(os.ttyname(slave))
            # Short paced blocks model reception; initial attachment has a partial prefix.
            os.write(master, b'fragment')
            for line in self.data.splitlines(keepends=True):
                os.write(master, line)
                root.update(); time.sleep(.011); root.update()
            self.assertEqual(app.session.rows[-1]['result'], 1)
            self.assertIn('retained', app.summary.get())
            self.assertEqual(len(app.table.get_children()), len(app.session.rows))
            self.assertFalse(app.session.live())  # Terminal heartbeats are not acquisition.
            os.close(master); master = None
            for _ in range(5): time.sleep(.025); root.update()
            self.assertIsNone(app.port)
            with tempfile.TemporaryDirectory() as td:
                app.session.export(Path(td)/'live.csv', app.decoder.errors)
                self.assertIn('jayis1,metadata,live,0', (Path(td)/'live.csv').read_text())
                path = Path(td)/'replay.th1'; path.write_bytes(self.data)
                app.replay(path)
                self.assertEqual(app.session.source, 'replay')
                self.assertIn('retained', app.summary.get())
                with self.assertRaises((OSError,termios.error)): app.connect('/dev/null')
                self.assertEqual(app.session.source, 'replay')
            for index in range(3): app.tabs.select(index); root.update()
        finally:
            app.close()
            if master is not None: os.close(master)
            os.close(slave)

if __name__ == '__main__':
    unittest.main(verbosity=2)
