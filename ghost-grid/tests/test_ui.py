# Author: jayis1. SPDX-License-Identifier: MIT
"""Exercise real Tk widgets with C-encoded synthetic data and Linux PTYs."""
import csv
import os
import pathlib
import pty
import sys
import tempfile
import termios
import time
import tkinter as tk
import unittest
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / 'app'))
from workbench import Workbench
from transport import Serial, load_replay

ROOT = pathlib.Path(__file__).resolve().parents[1]


class UITests(unittest.TestCase):
    def setUp(self):
        self.root = tk.Tk()
        self.app = Workbench(self.root)
        self.root.update()

    def tearDown(self):
        self.app.close()

    def spin(self, seconds=.08):
        end = time.monotonic()+seconds
        while time.monotonic() < end:
            self.root.update()
            time.sleep(.005)

    def test_replay_grids_export_and_reconnect(self):
        self.app.replay(ROOT/'firmware/build/demo.gg1')
        self.assertEqual(len(self.app.table.get_children()), 16)
        self.assertEqual(self.app.session.rows[-1]['bins15'], 14)
        self.assertIn('CLOSED', self.app.cells[3].cget('text'))
        with tempfile.TemporaryDirectory() as tmp:
            target = pathlib.Path(tmp)/'evidence.csv'
            self.app.export(target)
            self.assertEqual(target.stat().st_mode & 0o777, 0o600)
            with target.open() as handle:
                rows = list(csv.reader(handle))
            self.assertEqual(rows[0][:4], ['author', 'jayis1', 'source', 'replay'])
            self.assertEqual(len(rows), 322)
            with self.assertRaises(FileExistsError): self.app.export(target)
            fifo = pathlib.Path(tmp)/'pipe'
            os.mkfifo(fifo)
            with self.assertRaises(ValueError): load_replay(fifo)
            oversized = pathlib.Path(tmp)/'large'
            with oversized.open('wb') as handle: handle.truncate(2*1024*1024+1)
            with self.assertRaises(ValueError): load_replay(oversized)
        # A failed connection leaves earlier evidence available.
        self.app.path.set('/dev/null'); self.app.connect()
        self.assertEqual(self.app.session.source, 'replay')
        master, slave = pty.openpty()
        saved = termios.tcgetattr(slave)
        try:
            self.app.path.set(os.ttyname(slave)); self.app.connect()
            self.assertEqual(self.app.summary.get(), 'No samples')
            first_port = self.app.port
            self.app.connect()
            self.assertIs(self.app.port, first_port)
            # Include a late-attachment fragment; application must resynchronize.
            frames = (ROOT/'firmware/build/demo.gg1').read_bytes().splitlines(keepends=True)
            os.write(master, b'late-suffix\n'+frames[139])  # mask 6
            self.spin()
            os.write(master, frames[159])  # mask 7: ghost at R1C1
            self.spin()
            self.assertIn('PREDICTED GHOST', self.app.cells[3].cget('text'))
            self.assertEqual(self.app.decoder.initial_fragments, 1)
            self.app.disconnect()
            self.assertEqual(termios.tcgetattr(slave), saved)
            self.assertEqual(self.app.session.source, 'live')
            self.app.connect()
            os.close(master); master = None
            self.spin()
            self.assertIsNone(self.app.port)
            self.assertIn('disconnected', self.app.warning)
        finally:
            if master is not None: os.close(master)
            os.close(slave)

    def test_constructor_failure_restores_tty(self):
        from unittest.mock import patch
        master, slave = pty.openpty()
        saved = termios.tcgetattr(slave)
        try:
            with patch('transport.termios.tcflush', side_effect=OSError('injected flush failure')):
                with self.assertRaises(OSError): Serial(os.ttyname(slave))
            self.assertEqual(termios.tcgetattr(slave), saved)
        finally:
            os.close(master); os.close(slave)

if __name__ == '__main__':
    unittest.main()
