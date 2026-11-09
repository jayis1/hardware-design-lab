# Author: jayis1. SPDX-License-Identifier: MIT
import csv
import os
import pathlib
import pty
import sys
import tempfile
import time
import tkinter as tk
import unittest
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / 'app'))
from workbench import Workbench
ROOT = pathlib.Path(__file__).resolve().parents[1]

class GUI(unittest.TestCase):
    def setUp(self):
        self.root = tk.Tk()
        self.ui = Workbench(self.root)
    def tearDown(self):
        self.ui.close()
    def pump(self):
        for _ in range(8):
            self.root.update(); time.sleep(.015)
    def test_replay_screens_and_private_export(self):
        self.ui.replay(str(ROOT/'firmware/build/demo.lv1'))
        self.pump()
        self.assertEqual(self.ui.matrix.get(), 'True A:  A 1   B 1\nTrue B:  A 1   B 1')
        self.assertIn('5 presentations', self.ui.summary.get())
        self.assertEqual(len(self.ui.history.get_children()), 168)
        for i in range(3):
            self.ui.tabs.select(i); self.pump()
        with tempfile.TemporaryDirectory() as tmp:
            p = pathlib.Path(tmp)/'results.csv'
            self.ui.export(str(p))
            self.assertEqual(p.stat().st_mode & 0o777, 0o600)
            with p.open() as handle:
                rows = list(csv.reader(handle))
            self.assertIn(['author','jayis1'], rows)
            self.assertIn(['origin','replay'], rows)
            with self.assertRaises(FileExistsError): self.ui.export(str(p))
    def test_live_pty_double_connect_and_hangup(self):
        master, slave = pty.openpty()
        try:
            path = os.ttyname(slave)
            self.ui.connect(path)
            with self.assertRaises(ValueError): self.ui.connect(path)
            data = (ROOT/'firmware/build/demo.lv1').read_bytes()
            # Feed bounded batches to avoid filling the PTY before GUI drains.
            lines = data.splitlines(keepends=True)
            os.write(master, lines[0][10:])
            self.pump()  # Attaching mid-frame is normal, not ongoing corruption.
            self.assertTrue(self.ui.session.connected)
            for start in range(0, len(lines), 20):
                os.write(master, b''.join(lines[start:start+20])); self.pump()
            self.assertEqual(self.ui.session.rows[-1]['completed'], 4)
            self.assertEqual(self.ui.session.rows[-1]['rejected'], 1)
            self.assertTrue(self.ui.session.fresh())
            os.close(master); master = None
            self.pump()
            self.assertFalse(self.ui.session.connected)
            self.assertEqual(self.ui.session.origin, 'live')
            self.assertIn('disconnected', self.ui.warning.get())
        finally:
            if master is not None: os.close(master)
            os.close(slave)
    def test_failed_connection_preserves_replay(self):
        self.ui.replay(str(ROOT/'firmware/build/demo.lv1'))
        retained = self.ui.session
        with self.assertRaises(Exception): self.ui.connect('/dev/null')
        self.assertIs(self.ui.session, retained)

if __name__ == '__main__': unittest.main()
