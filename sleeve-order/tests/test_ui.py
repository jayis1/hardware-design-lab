# Author: jayis1. SPDX-License-Identifier: MIT
"""Exercise the real Tk callbacks with real C frames and Linux PTYs."""
import os
import pathlib
import sys
import tempfile
import termios
import tkinter as tk
import unittest
from unittest.mock import patch
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / 'app'))
from workbench import Workbench
ROOT = pathlib.Path(__file__).resolve().parents[1]

class UserInterface(unittest.TestCase):
    def setUp(self):
        self.root = tk.Tk()
        self.ui = Workbench(self.root)
        self.root.update()
    def tearDown(self):
        self.ui.close()
    def test_replay_screens_and_export(self):
        self.ui.replay(str(ROOT / 'firmware/build/demo.sv1'))
        self.root.update()
        self.assertEqual(self.ui.session.rows[-1]['completed'], 2)
        self.assertEqual(len(self.ui.history.get_children()), 144)
        self.assertIn('REPLAY', self.ui.status.get())
        for tab in self.ui.tabs.tabs():
            self.ui.tabs.select(tab)
            self.root.update()
        with tempfile.TemporaryDirectory() as folder:
            path = str(pathlib.Path(folder) / 'evidence.csv')
            self.ui.export(path)
            self.assertIn('jayis1,replay', pathlib.Path(path).read_text())
            with patch('workbench.messagebox.showerror') as error:
                self.ui.export(path)
                error.assert_called_once()
    def test_live_reconnect_and_hangup(self):
        master, slave = os.openpty()
        original = termios.tcgetattr(slave)
        try:
            self.ui.port.set(os.ttyname(slave))
            self.ui.connect()
            with patch('workbench.messagebox.showerror') as error:
                self.ui.connect()
                error.assert_called_once()
            data = (ROOT / 'firmware/build/demo.sv1').read_bytes().splitlines(keepends=True)
            os.write(master, b'fragment' + b''.join(data[:30]))
            self.ui.poll()
            self.root.update()
            self.assertEqual(len(self.ui.session.rows), 30)
            self.assertGreater(self.ui.session.errors, 0)
            self.assertTrue(self.ui.session.fresh(True))
            self.ui.disconnect()
            self.assertEqual(termios.tcgetattr(slave), original)
            self.assertEqual(len(self.ui.session.rows), 30)
            self.ui.connect()
            self.assertEqual(len(self.ui.history.get_children()), 0)
            self.assertIn('No accepted', self.ui.summary.get())
            os.close(master)
            master = None
            self.ui.poll()
            self.assertIsNone(self.ui.serial)
        finally:
            if master is not None: os.close(master)
            os.close(slave)
    def test_failed_open_preserves_replay(self):
        self.ui.replay(str(ROOT / 'firmware/build/demo.sv1'))
        old = self.ui.session
        self.ui.port.set('/dev/null')
        with patch('workbench.messagebox.showerror') as error:
            self.ui.connect()
            error.assert_called_once()
        self.assertIs(self.ui.session, old)

if __name__ == '__main__': unittest.main()
