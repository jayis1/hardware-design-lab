# Author: jayis1. SPDX-License-Identifier: MIT
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
ROOT = Path(__file__).resolve().parents[1]

class GuiTests(unittest.TestCase):
    def test_replay_live_export_and_hangup(self):
        root = tk.Tk(); app = Workbench(root)
        def pump(seconds=.08):
            end = time.monotonic() + seconds
            while time.monotonic() < end:
                root.update(); time.sleep(.002)
        try:
            app.replay(ROOT / 'firmware/build/demo.ke1'); pump()
            self.assertIn('HALT', app.summary.get())
            self.assertIn('Cycles 1', app.summary.get())
            self.assertEqual(len(app.trace.get_children()), len(app.session.rows))
            for tab in app.tabs.tabs(): app.tabs.select(tab); pump(.01)
            with tempfile.TemporaryDirectory() as d:
                app.export(Path(d) / 'evidence.csv')
                self.assertIn('replay', (Path(d) / 'evidence.csv').read_text())
            master, slave = pty.openpty()
            try:
                app.connect(os.ttyname(slave)); pump(.02)
                self.assertNotIn('Cycles 1', app.summary.get())
                with self.assertRaises(ValueError): app.connect(os.ttyname(slave))
                data = (ROOT / 'firmware/build/demo.ke1').read_bytes().splitlines(keepends=True)
                # Transport is a PTY with synthetic C frames, not a physical board.
                os.write(master, b'fragment' + b''.join(data[:60])); pump()
                self.assertIn('READY', app.summary.get())
                self.assertEqual(app.session.errors, 1)
                self.assertTrue(app.session.fresh())
                os.close(master); master = None; pump()
                self.assertFalse(app.session.connected)
                self.assertFalse(app.session.fresh())
                self.assertEqual(app.session.provenance, 'live')
            finally:
                if master is not None: os.close(master)
                os.close(slave)
        finally:
            app.close()

if __name__ == '__main__': unittest.main()
