# Author: jayis1. SPDX-License-Identifier: MIT
"""Real Tk/PTY integration tests; no physical device claim."""
import csv
import os
from pathlib import Path
import pty
import sys
import tempfile
import termios
import time
import tkinter as tk
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'app'))
from workbench import Workbench, Serial
from test_app import fixture
class GuiTests(unittest.TestCase):
    def setUp(self):
        self.root = tk.Tk()
        self.app = Workbench(self.root)
    def tearDown(self):
        self.app.close()
    def pump(self, seconds=.12):
        until = time.monotonic() + seconds
        while time.monotonic() < until:
            self.root.update()
            time.sleep(.005)
    def test_live_and_export(self):
        master, slave = pty.openpty()
        original = termios.tcgetattr(slave)
        try:
            self.app.connect(os.ttyname(slave))
            with self.assertRaises(ValueError): self.app.connect(os.ttyname(slave))
            for line in fixture().splitlines(keepends=True):
                os.write(master, line)
                self.pump()
            self.assertEqual(len(self.app.session.rows), 4)
            self.assertIn('Full A: 3', self.app.live.get())
            self.assertEqual(len(self.app.history.get_children()), 4)
            for tab in self.app.tabs.tabs():
                self.app.tabs.select(tab)
                self.pump(.02)
            self.app.disconnect()
            self.assertEqual(termios.tcgetattr(slave), original)
            with tempfile.TemporaryDirectory() as folder:
                path = Path(folder)/'out.csv'
                self.app.export(path)
                self.assertEqual(path.stat().st_mode & 0o777, 0o600)
                text = path.read_text()
                self.assertIn('jayis1', text)
                self.assertIn('live', text)
                self.assertIn('False', text)
                with self.assertRaises(FileExistsError): self.app.export(path)
        finally:
            os.close(master)
            os.close(slave)
    def test_hangup_and_replay(self):
        master, slave = pty.openpty()
        try:
            self.app.connect(os.ttyname(slave))
            os.close(master)
            master = None
            self.pump()
            self.assertIsNone(self.app.serial)
            with tempfile.TemporaryDirectory() as folder:
                path = Path(folder)/'log'
                path.write_bytes(fixture())
                self.app.load(path)
                self.assertEqual(self.app.session.provenance, 'replay')
                self.assertFalse(self.app.session.fresh(time.monotonic()))
                path.write_bytes(b'bad')
                with self.assertRaises(ValueError): self.app.load(path)
                self.assertEqual(len(self.app.session.rows), 4)
        finally:
            if master is not None: os.close(master)
            os.close(slave)
    def test_serial_constructor_restoration(self):
        from unittest.mock import patch
        master, slave = pty.openpty()
        original = termios.tcgetattr(slave)
        try:
            with patch('workbench.termios.tcflush', side_effect=OSError('injected')):
                with self.assertRaises(OSError): Serial(os.ttyname(slave))
            self.assertEqual(termios.tcgetattr(slave), original)
        finally:
            os.close(master)
            os.close(slave)
    def test_partial_disconnect_and_error_accounting(self):
        master, slave = pty.openpty()
        try:
            self.app.connect(os.ttyname(slave))
            lines = fixture().splitlines(keepends=True)
            os.write(master, lines[0])
            self.pump()
            os.write(master, b'garbage' + lines[0][:80])
            self.pump()
            self.app.disconnect()
            self.assertEqual(self.app.session.errors, 2)
            self.app.connect(os.ttyname(slave))
            os.write(master, lines[-1])
            self.pump()
            os.write(master, b'garbage' + lines[0])
            self.pump()
            self.assertIsNone(self.app.serial)
            self.assertEqual(self.app.session.errors, 1)
        finally:
            os.close(master)
            os.close(slave)
if __name__ == '__main__': unittest.main()
