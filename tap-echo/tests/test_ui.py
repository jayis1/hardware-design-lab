# Author: jayis1. SPDX-License-Identifier: MIT
import os
from pathlib import Path
import pty
import sys
import unittest
from unittest.mock import patch
import tempfile
import json
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'app'))
from workbench import Workbench
from model import FIELDS
from test_app import frame
import tkinter as tk

class ScreenTests(unittest.TestCase):
    def test_screens_serial_stale_and_disconnect(self):
        root = tk.Tk(); root.withdraw(); app = Workbench(root)
        master, slave = pty.openpty()
        try:
            app.path.set(os.ttyname(slave)); app.connect()
            row = dict.fromkeys(FIELDS, 0)
            os.write(master, frame(row)); app.poll(); root.update()
            self.assertIn('LIVE', app.status.get())
            self.assertEqual(len(app.session.rows), 1)
            self.assertEqual(len(app.tabs.tabs()), 3)
            app.last_seen -= 3; os.write(master, frame(row)); app.poll()
            self.assertIn('STALE', app.status.get())
            app.disconnect(); self.assertIsNone(app.serial)
            app.path.set('/dev/null'); app.connect()
            self.assertIn('ERROR', app.status.get())
            app.replay(Path(__file__).resolve().parents[1] / 'firmware/build/demo.te1')
            self.assertIn('REPLAY', app.status.get())
            self.assertIn('match', app.summary.get())
            with tempfile.TemporaryDirectory() as directory:
                path = Path(directory) / 'evidence.json'
                with patch('workbench.filedialog.asksaveasfilename', return_value=str(path)):
                    app.export()
                data = json.loads(path.read_text())
                self.assertFalse(data['metadata']['connected'])
                self.assertIn('REPLAY', data['metadata']['status'])
                self.assertIsNone(data['metadata']['seconds_since_progress'])
        finally:
            app.close(); os.close(master); os.close(slave)
if __name__ == '__main__': unittest.main()
