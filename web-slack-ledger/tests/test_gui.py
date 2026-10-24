# Author: jayis1. SPDX-License-Identifier: MIT
"""Exercise actual Tk screens and serial reader through a Linux PTY."""
import csv
import os
from pathlib import Path
import pty
import tempfile
import time
import tkinter as tk
import unittest
from test_app import ledger, frame
class GuiTests(unittest.TestCase):
    def test_serial_replay_export_settings(self):
        root=tk.Tk()
        app=ledger.App(root)
        master, slave=pty.openpty()
        def pump(seconds=0.15):
            deadline=time.monotonic()+seconds
            while time.monotonic()<deadline:
                root.update()
                time.sleep(.01)
        try:
            app.port.set(os.ttyname(slave))
            app.connect()
            self.assertIsNotNone(app.fd)
            os.write(master,frame(0,100)+frame(1,200,8,3))
            pump()
            self.assertEqual(len(app.model.rows),2)
            self.assertIn('1.250 mm',app.summary.get())
            for index in range(3):
                app.tabs.select(index)
                pump(.05)
            app.scale_up.set('500'); app.calibrate()
            self.assertEqual(app.up_scale,500)
            app.scale_up.set('nan'); app.calibrate()
            self.assertEqual(app.up_scale,500)
            self.assertIn('finite',app.error)
            with tempfile.TemporaryDirectory() as directory:
                export=Path(directory)/'capture.csv'
                app.model.export(export,app.source,True,app.up_scale,app.down_scale)
                self.assertEqual(export.stat().st_mode & 0o777,0o600)
                with export.open() as handle:
                    rows=list(csv.DictReader(handle))
                self.assertEqual(len(rows),2)
                self.assertEqual(rows[-1]['author'],'jayis1')
                with self.assertRaises(FileExistsError):
                    app.model.export(export,app.source,True,250,250)
                replay=Path(directory)/'synthetic.txt'
                replay.write_bytes(frame(0,100)+frame(1,200))
                app.load_replay(replay)
                self.assertIsNone(app.fd)
                self.assertIn('NOT LIVE',app.source)
                self.assertEqual(len(app.model.rows),2)
            app.connect()
            self.assertEqual(app.summary.get(),'No samples')
            os.write(master,frame(0,100))
            pump()
            pump(1.1)
            self.assertIn('STALE',app.status.get())
            os.close(master); master=None
            pump()
            self.assertIsNone(app.fd)
            self.assertIn('disconnected',app.error)
            app.port.set('/nonexistent/port')
            app.connect()
            self.assertIsNone(app.fd)
        finally:
            if master is not None: os.close(master)
            os.close(slave)
            app.close()
if __name__=='__main__': unittest.main()
