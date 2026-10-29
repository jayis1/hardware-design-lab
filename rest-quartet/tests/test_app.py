# Author: jayis1. SPDX-License-Identifier: MIT
import os
from pathlib import Path
import pty
import subprocess
import sys
import tempfile
import time
import tkinter as tk
import unittest
from unittest.mock import patch
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'app'))
import protocol as p
import quartet as q
ROOT = Path(__file__).resolve().parents[1]

class Checks(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data = subprocess.check_output([str(ROOT/'firmware/build/test-engine'), 'fixture'])
        cls.lines = cls.data.splitlines(keepends=True)

    def test_real_encoder_and_fragmentation(self):
        parser = p.Parser()
        frames = []
        for byte in self.data:
            frames.extend(parser.feed(bytes([byte])))
        parser.finish()
        self.assertEqual(len(frames), 2)
        self.assertEqual(frames[0][7:11], (1, 2, 1, 1))
        self.assertEqual(parser.errors, 0)

    def test_corruption_and_resync(self):
        parser = p.Parser()
        self.assertEqual(len(parser.feed(b'RQ1,broken'+self.data)), 2)
        self.assertGreater(parser.errors, 0)
        parser.feed(b'RQ1,unfinished')
        parser.finish()
        self.assertGreater(parser.errors, 1)
        parser = p.Parser()
        parser.feed(b'x'*2000+b'\n')
        self.assertLess(len(parser.buffer), 768)

    def test_impossible_conservation(self):
        f = list(p.decode(self.lines[0]))
        f[9] += 1
        with self.assertRaises(ValueError):
            p.decode(p.encode(f))

    def test_session_rollback_and_freshness(self):
        a,b = map(p.decode,self.lines)
        s=p.Session('live')
        s.accept(a, 1.0)
        self.assertFalse(s.fresh(1.0, True))
        s.accept(b, 1.1)
        self.assertTrue(s.fresh(1.1, True))
        s.accept(b, 1.9)
        self.assertFalse(s.fresh(2.2, True))
        s.accept(b, 2.3)
        self.assertTrue(s.blocked)
        s=p.Session('live')
        s.accept(b,1)
        s.accept(a,1.1)
        self.assertTrue(s.blocked)

    def test_replay_and_private_export(self):
        with tempfile.TemporaryDirectory() as directory:
            src=Path(directory)/'input'
            src.write_bytes(self.data)
            session,parser=q.replay(src)
            self.assertEqual(len(session.rows),2)
            target=Path(directory)/'out.csv'
            q.export(target,session,parser,False)
            self.assertIn('jayis1',target.read_text())
            self.assertIn('replay',target.read_text())
            self.assertEqual(target.stat().st_mode & 0o777,0o600)
            with self.assertRaises(FileExistsError):
                q.export(target,session,parser,False)
            fifo=Path(directory)/'fifo'
            os.mkfifo(fifo)
            with self.assertRaises(ValueError):
                q.replay(fifo)
            src.write_bytes(self.data+b'RQ1,')
            with self.assertRaises(ValueError):
                q.replay(src)

    def test_gradual_backlog(self):
        frame=list(p.decode(self.lines[0]))
        s=p.Session('live')
        for i in range(100):
            f=frame.copy()
            f[0]+=i*90
            s.accept(tuple(f),i*.1)
        self.assertTrue(s.blocked)

    def test_restore_on_constructor_failure(self):
        import termios
        master,slave=pty.openpty()
        try:
            old=termios.tcgetattr(slave)
            with patch.object(q.termios,'tcflush',side_effect=OSError('synthetic failure')):
                with self.assertRaises(OSError):
                    q.Serial(os.ttyname(slave))
            self.assertEqual(termios.tcgetattr(slave),old)
        finally:
            os.close(master)
            os.close(slave)

    def test_gui_replay_export_actions(self):
        root=tk.Tk()
        app=q.App(root)
        try:
            with tempfile.TemporaryDirectory() as directory:
                src=Path(directory)/'fixture'
                src.write_bytes(self.data)
                out=Path(directory)/'export.csv'
                with patch.object(q.filedialog,'askopenfilename',return_value=str(src)):
                    app.load()
                root.update()
                self.assertEqual(app.session.source,'replay')
                self.assertEqual(len(app.history.get_children()),2)
                with patch.object(q.filedialog,'asksaveasfilename',return_value=str(out)):
                    app.save()
                self.assertIn('jayis1',out.read_text())
                self.assertEqual(len(app.tabs.tabs()),3)
        finally:
            app.close()

    def test_gui_serial_and_disconnect(self):
        master,slave=pty.openpty()
        root=tk.Tk()
        app=q.App(root)
        try:
            app.port.set(os.ttyname(slave))
            app.connect()
            self.assertIsNotNone(app.serial)
            original=app.serial
            app.connect()
            self.assertIs(app.serial,original)
            os.write(master,self.lines[0])
            for _ in range(10): root.update(); time.sleep(.01)
            os.write(master,self.lines[1])
            for _ in range(10): root.update(); time.sleep(.01)
            self.assertEqual(len(app.table.get_children()),4)
            self.assertEqual(len(app.history.get_children()),2)
            self.assertIn('LIVE',app.status.get())
            os.close(master)
            master=-1
            for _ in range(10): root.update(); time.sleep(.01)
            self.assertIsNone(app.serial)
            self.assertIn('DISCONNECTED',app.status.get())
            self.assertEqual(len(app.session.rows),2)
        finally:
            app.close()
            if master>=0: os.close(master)
            os.close(slave)

if __name__=='__main__': unittest.main()
