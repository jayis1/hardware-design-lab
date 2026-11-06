# Author: jayis1. SPDX-License-Identifier: MIT
"""Exercise actual Tk event loop and actual serial transport using a synthetic PTY."""
import csv
import os
from pathlib import Path
import pty
import sys
import tempfile
import time
import tkinter as tk
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'app'))
from workbench import Workbench
from transport import load_replay
ROOT = Path(__file__).resolve().parents[1]

class GuiTests(unittest.TestCase):
    def test_live_replay_export_hangup(self):
        root = tk.Tk(); app = Workbench(root)
        master, slave = pty.openpty()
        try:
            app.connect(os.ttyname(slave))
            with self.assertRaises(ValueError): app.connect(os.ttyname(slave))
            data = (ROOT / 'firmware/build/demo.pc1').read_bytes()
            os.write(master, b'partial garbage' + data)
            for _ in range(8): root.update(); time.sleep(0.02)
            self.assertEqual(app.session.rows[-1]['completed'], 1)
            self.assertIn('Arrivals 5', app.summary.get())
            self.assertEqual(len(app.trace.get_children()), len(app.session.rows))
            self.assertEqual(app.session.errors, 1)
            os.close(master); master = None
            for _ in range(8): root.update(); time.sleep(0.02)
            self.assertIsNone(app.serial)
            self.assertFalse(app.session.fresh())
            self.assertEqual(app.session.provenance, 'live')
            with tempfile.TemporaryDirectory() as td:
                path = Path(td) / 'live.csv'; app.export(path)
                with path.open() as handle: rows = list(csv.reader(handle))
                self.assertEqual(rows[1][1], 'live')
                self.assertIn('serial disconnected', rows[1])
                with self.assertRaises(FileExistsError): app.export(path)
            app.replay(ROOT / 'firmware/build/demo.pc1')
            self.assertEqual(app.session.provenance, 'replay')
            self.assertEqual(len(app.tabs.tabs()), 3)
            old = app.session
            with self.assertRaises((OSError, ValueError)): app.connect('/not/a/serial/device')
            self.assertIs(app.session, old)
        finally:
            app.close()
            if master is not None: os.close(master)
            os.close(slave)

if __name__ == '__main__': unittest.main()
