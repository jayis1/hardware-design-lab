# Author: jayis1. SPDX-License-Identifier: MIT
import os
from pathlib import Path
import pty
import tempfile
import termios
import time
import tkinter as tk
import unittest
from unittest.mock import patch
from test_app import app, ROOT

class GuiTests(unittest.TestCase):
    def test_real_event_loop_serial_replay_export_hangup(self):
        master,slave=pty.openpty()
        root=tk.Tk()
        window=app.Window(root)
        def pump():
            deadline=time.monotonic()+0.25
            while time.monotonic()<deadline:
                root.update()
                time.sleep(0.005)
        try:
            self.assertEqual(len(window.tabs.tabs()),3)
            window.path.set(os.ttyname(slave)); window.connect()
            self.assertIsNotNone(window.serial)
            raw=(ROOT/'firmware/build/fixture.txt').read_bytes()
            os.write(master,raw); pump()
            self.assertEqual(window.model.latest['crossings'],16)
            self.assertEqual(len(window.table.get_children()),len(window.model.rows))
            for tab in window.tabs.tabs():
                window.tabs.select(tab); pump()
            with tempfile.TemporaryDirectory() as d:
                target=Path(d)/'export.csv'
                with patch.object(app.filedialog,'asksaveasfilename',return_value=str(target)):
                    window.export()
                self.assertIn('jayis1,live,connected',target.read_text())
                self.assertEqual(target.stat().st_mode & 0o777,0o600)
            os.close(master); master=None; pump()
            self.assertIsNone(window.serial)
            self.assertIn('disconnected',window.state.get())
            window.load_replay(ROOT/'firmware/build/fixture.txt'); pump()
            self.assertIn('REPLAY',window.state.get())
            window.model.start('live'); window.render()
            self.assertEqual(window.summary.get(),'No evidence')
            self.assertEqual(len(window.table.get_children()),0)
            window.path.set('/path/that/does/not/exist'); window.connect()
            self.assertTrue(window.error.get())
        finally:
            window.close()
            if master is not None: os.close(master)
            os.close(slave)

    def test_failed_flush_restores_terminal(self):
        master,slave=pty.openpty()
        try:
            before=termios.tcgetattr(slave)
            with patch.object(app.termios,'tcflush',side_effect=OSError('injected flush failure')):
                with self.assertRaises(OSError): app.Serial(os.ttyname(slave))
            self.assertEqual(termios.tcgetattr(slave),before)
        finally:
            os.close(master); os.close(slave)

if __name__=='__main__': unittest.main()
