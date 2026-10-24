# Author: jayis1. SPDX-License-Identifier: MIT
import os
import pathlib
import pty
import subprocess
import sys
import tempfile
import time
import tkinter as tk
import unittest
ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'app'))
from witness import App
class GuiTests(unittest.TestCase):
    def test_live_replay_disconnect_export(self):
        raw = subprocess.check_output([str(ROOT / 'firmware/build/test-engine'), '--fixture'])
        master, slave = pty.openpty()
        root = tk.Tk()
        app = App(root)
        try:
            app.port.set(os.ttyname(slave))
            app.connect()
            self.assertEqual(app.mode, 'live')
            os.write(master, raw)
            end = time.monotonic() + 0.2
            while time.monotonic() < end:
                root.update(); time.sleep(0.01)
            self.assertIn('Drain band: 200 ms', app.summary.get())
            self.assertIn('LIVE', app.status.get())
            app.session.received -= 2
            app.render()
            self.assertIn('STALE', app.status.get())
            with tempfile.TemporaryDirectory() as tmp:
                stream = pathlib.Path(tmp) / 'synthetic.sw1'
                stream.write_bytes(raw)
                app.load_replay(stream)
                self.assertIsNone(app.serial)
                self.assertIn('REPLAY', app.status.get())
                self.assertEqual(len(app.tree.get_children()), 1)
                export = pathlib.Path(tmp) / 'export.csv'
                app.session.export(export, app.mode, time.monotonic())
                self.assertIn('jayis1,replay', export.read_text())
                app.disconnect()
                detached = pathlib.Path(tmp) / 'detached.csv'
                app.session.export(detached, app.mode, time.monotonic())
                self.assertIn('jayis1,replay,disconnected', detached.read_text())
                app.connect()
                self.assertEqual(app.summary.get(), 'No evidence loaded')
                self.assertEqual(len(app.tree.get_children()), 0)
            os.close(master); master = None
            end = time.monotonic() + 0.2
            while time.monotonic() < end:
                root.update(); time.sleep(0.01)
            self.assertEqual(app.mode, 'disconnected')
        finally:
            app.close()
            if master is not None:
                os.close(master)
            os.close(slave)
if __name__ == '__main__':
    unittest.main()
