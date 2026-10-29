# Author: jayis1. SPDX-License-Identifier: MIT
import binascii
import importlib.util
import os
from pathlib import Path
import pty
import subprocess
import tempfile
import time
import unittest
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'app'))
from protocol import decode, Session, Stream, read_replay

def frame(**changes):
    fields = dict(now=100, phase=1, mask=12, starts=0, completed=0,
                  aborted=0, duration=0, forward=0, reverse=0,
                  seated_reverse=0, rollback=0, faults=0, gaps=0, tx_errors=0)
    fields.update(changes)
    body = ('RA1,' + ','.join(str(x) for x in fields.values())).encode()
    return body + f'*{binascii.crc_hqx(body, 65535):04X}\n'.encode()

class ProtocolTest(unittest.TestCase):
    def test_fixture(self):
        root = Path(__file__).resolve().parents[1]
        wire = subprocess.check_output([str(root/'firmware/build/test-engine'), 'fixture'])
        row = decode(wire)
        self.assertEqual((row['forward'], row['reverse'], row['rollback']), (4, 2, 2))

    def test_invalid(self):
        for args in [dict(seated_reverse=1), dict(starts=1), dict(mask=4),
                     dict(phase=2), dict(reverse=1), dict(faults=128),
                     dict(gaps=1), dict(duration=30001), dict(phase=3),
                     dict(starts=1, completed=1, reverse=2, rollback=1),
                     dict(starts=1, phase=2, mask=8, duration=30000)]:
            with self.subTest(args=args), self.assertRaises(ValueError):
                decode(frame(**args))
        with self.assertRaises(ValueError):
            decode(frame().replace(b'RA1', b'RA2'))

    def test_framing(self):
        stream = Stream()
        self.assertEqual(stream.feed(b'noiseRA1,100,' + frame()), [decode(frame())])
        self.assertEqual(stream.errors, 2)  # noise prefix plus truncated frame
        stream.feed(b'RA1,')
        with self.assertRaises(ValueError): stream.finish()

    def test_duplicate_reset_stale(self):
        s = Session('live')
        s.accept(decode(frame()), 1.0)
        s.accept(decode(frame(now=200)), 1.1)
        self.assertTrue(s.fresh(1.2, True))
        self.assertFalse(s.fresh(1.2, False))
        s.accept(decode(frame(now=200)), 1.9)
        self.assertFalse(s.fresh(2.2, True))
        with self.assertRaises(ValueError): s.accept(decode(frame(now=100)), 2.3)
        self.assertTrue(s.blocked)

    def test_contradiction(self):
        s = Session('replay')
        s.accept(decode(frame()), 0)
        with self.assertRaises(ValueError): s.accept(decode(frame(faults=4)), 0)

    def test_host_pause(self):
        s = Session('live')
        s.accept(decode(frame()), 1)
        with self.assertRaises(ValueError): s.accept(decode(frame(now=200)), 4)

    def test_counter_rollback(self):
        s = Session('replay')
        s.accept(decode(frame(starts=1, completed=1)), 0)
        with self.assertRaises(ValueError): s.accept(decode(frame(now=200)), 0)

    def test_retained_evidence_immutable(self):
        s = Session('replay')
        s.accept(decode(frame(starts=1, completed=1, forward=2)), 0)
        with self.assertRaises(ValueError):
            s.accept(decode(frame(now=200, starts=1, completed=1, forward=3)), 0)

    def test_replay_bounds(self):
        with tempfile.TemporaryDirectory() as td:
            path = Path(td)/'r'
            path.write_bytes(frame())
            self.assertEqual(len(read_replay(path).rows), 1)
            path.write_bytes(frame()+b'RA1,')
            with self.assertRaises(ValueError): read_replay(path)
            path.unlink(); os.mkfifo(path)
            with self.assertRaises(ValueError): read_replay(path)

    def test_export(self):
        with tempfile.TemporaryDirectory() as td:
            s = Session('replay')
            path = Path(td)/'out.json'
            s.export(path, False, 0)
            self.assertIn('jayis1', path.read_text())
            self.assertIn('parser_errors', path.read_text())
            with self.assertRaises(FileExistsError): s.export(path, False, 0)

class GuiTest(unittest.TestCase):
    def test_real_event_loop_pty(self):
        import tkinter as tk
        import termios
        from audit import App
        root = tk.Tk()
        app = App(root)
        master, slave = pty.openpty()
        original = termios.tcgetattr(slave)
        try:
            app.connect(os.ttyname(slave))
            with self.assertRaises(ValueError): app.connect(os.ttyname(slave))
            os.write(master, frame() + frame(now=200))
            until = time.monotonic() + .25
            while time.monotonic() < until:
                root.update(); time.sleep(.01)
            self.assertEqual(len(app.session.rows), 2)
            self.assertIn('forward', app.live.get())
            self.assertEqual(len(app.history.get_children()), 2)
            for index in range(3):
                app.tabs.select(index); root.update()
            app.disconnect()
            self.assertEqual(termios.tcgetattr(slave), original)
            self.assertEqual(app.session.source, 'live')
            app.connect(os.ttyname(slave))
            os.close(master); master = -1
            until = time.monotonic() + .25
            while time.monotonic() < until:
                root.update(); time.sleep(.01)
            self.assertIsNone(app.port)
            self.assertIn('disconnected', app.status.get())
        finally:
            app.close()
            if master >= 0: os.close(master)
            os.close(slave)

if __name__ == '__main__':
    unittest.main()
