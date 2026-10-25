# Author: jayis1. SPDX-License-Identifier: MIT
import csv
import importlib.util
import os
from pathlib import Path
import pty
import tempfile
import termios
import time
import tkinter as tk
import unittest
from unittest.mock import patch
ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('pedal_app', ROOT/'app/app.py')
assert spec is not None and spec.loader is not None
app = importlib.util.module_from_spec(spec)
spec.loader.exec_module(app)


class GuiTests(unittest.TestCase):
    def test_live_replay_disconnect_and_export(self):
        root = tk.Tk()
        ui = app.App(root)
        master, slave = pty.openpty()
        try:
            ui.path.set(os.ttyname(slave))
            ui.connect()
            self.assertIsNotNone(ui.serial)
            frames = (ROOT/'firmware/build/fixture.txt').read_bytes()
            os.write(master, frames)
            deadline = time.monotonic() + 0.2
            while time.monotonic() < deadline:
                root.update()
                time.sleep(0.005)
            self.assertEqual(len(ui.table.get_children()), 2)
            self.assertIn('Completed 2', ui.metrics.get())
            for index in range(3):
                ui.tabs.select(index)
                root.update()
            ui.model.last_seen = time.monotonic() - 2
            ui.render()
            self.assertIn('STALE', ui.status.get())
            os.close(master)
            master = None
            deadline = time.monotonic() + 0.1
            while time.monotonic() < deadline:
                root.update()
                time.sleep(0.005)
            self.assertIsNone(ui.serial)
            self.assertIn('Port lost', ui.message)
            with tempfile.TemporaryDirectory() as tmp:
                dest = Path(tmp)/'lesson.csv'
                ui.model.export(dest, False, time.monotonic())
                with dest.open() as stream:
                    rows = list(csv.reader(stream))
                self.assertEqual(len(rows), 4)
                self.assertIn('live serial:', rows[0][3])
                self.assertEqual(dest.stat().st_mode & 0o777, 0o600)
                with self.assertRaises(FileExistsError):
                    ui.model.export(dest, False, time.monotonic())
            ui.replay(ROOT/'firmware/build/fixture.txt')
            self.assertEqual(len(ui.table.get_children()), 2)
            self.assertIn('Replay only', ui.status.get())
            # A new empty connection must clear all numerical UI summaries.
            new_master, new_slave = pty.openpty()
            try:
                ui.path.set(os.ttyname(new_slave))
                ui.connect()
                self.assertEqual(ui.metrics.get(), '')
                self.assertEqual(ui.table.get_children(), ())
            finally:
                ui.disconnect()
                os.close(new_master)
                os.close(new_slave)
        finally:
            if master is not None:
                os.close(master)
            os.close(slave)
            ui.close()

    def test_failed_serial_setup_restores_terminal(self):
        master, slave = pty.openpty()
        try:
            saved = termios.tcgetattr(slave)
            with patch.object(app.termios, 'tcflush', side_effect=OSError('injected')):
                with self.assertRaises(OSError):
                    app.Serial(os.ttyname(slave))
            self.assertEqual(termios.tcgetattr(slave), saved)
        finally:
            os.close(master)
            os.close(slave)


if __name__ == '__main__':
    unittest.main()
