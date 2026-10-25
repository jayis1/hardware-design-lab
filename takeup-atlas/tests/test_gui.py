# Author: jayis1. SPDX-License-Identifier: MIT
"""Real Tk event loop and synthetic USB serial via Linux PTY; no hardware claim."""
import csv
import os
from pathlib import Path
import pty
import tempfile
import time
import tkinter as tk
import unittest
from unittest.mock import patch
from test_app import atlas, frame
class GuiTests(unittest.TestCase):
    def test_workflow(self):
        root=tk.Tk();app=atlas.App(root)
        master,slave=pty.openpty()
        def pump(seconds=.2):
            end=time.monotonic()+seconds
            while time.monotonic()<end:
                root.update();time.sleep(.01)
        try:
            app.port.set(os.ttyname(slave));app.connect()
            self.assertIsNotNone(app.fd)
            os.write(master,frame()+frame(1,200,events=2,reversals=2,direction=1))
            pump()
            self.assertEqual(len(app.model.rows),2)
            self.assertEqual(len(app.table.get_children()),2)
            self.assertIn('2.700 degrees',app.summary.get())
            for i in range(3): app.tabs.select(i);pump(.03)
            app.scale.set('800');app.calibrate();self.assertEqual(app.edges,800)
            app.scale.set('nan');app.calibrate();self.assertEqual(app.edges,800)
            self.assertIn('finite',app.error)
            pump(1.1);self.assertIn('STALE',app.status.get())
            with tempfile.TemporaryDirectory() as d:
                path=Path(d)/'export.csv'
                with patch('tkinter.filedialog.asksaveasfilename',return_value=str(path)):
                    app.export_dialog()
                with path.open() as f: rows=list(csv.DictReader(f))
                self.assertEqual(len(rows),2)
                self.assertEqual(rows[0]['source'],'LIVE USB')
                replay=Path(d)/'synthetic.txt';replay.write_bytes(frame()+frame(1,200))
                with patch('tkinter.filedialog.askopenfilename',return_value=str(replay)):
                    app.replay_dialog()
                self.assertIsNone(app.fd)
                self.assertIn('NOT LIVE',app.source)
                fifo=Path(d)/'fifo';os.mkfifo(fifo)
                with patch('tkinter.filedialog.askopenfilename',return_value=str(fifo)):
                    app.replay_dialog()
                self.assertIn('regular',app.error)
            app.connect();self.assertEqual(app.summary.get(),'No samples')
            os.write(master,frame());pump()
            os.write(master,frame(1,200,first=4));pump()
            self.assertIsNone(app.fd)
            self.assertIn('changed without',app.error)
            app.connect();os.close(master);master=None;pump()
            self.assertIsNone(app.fd)
            self.assertIn('disconnected',app.error)
        finally:
            if master is not None: os.close(master)
            os.close(slave);app.close()
    def test_setup_failure_restores_terminal(self):
        root=tk.Tk();app=atlas.App(root)
        master,slave=pty.openpty()
        try:
            before=atlas.termios.tcgetattr(slave)
            app.port.set(os.ttyname(slave))
            with patch.object(atlas.termios,'tcflush',side_effect=OSError('injected')):
                app.connect()
            self.assertIsNone(app.fd)
            self.assertEqual(atlas.termios.tcgetattr(slave),before)
        finally:
            os.close(master);os.close(slave);app.close()
if __name__=='__main__':unittest.main()
