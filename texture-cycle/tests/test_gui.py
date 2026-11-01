# Author: jayis1. SPDX-License-Identifier: MIT
"""Real Tk event loop with Linux PTY transport; no physical hardware claim."""
import os
from pathlib import Path
import pty
import sys
import tempfile
import time
import tkinter as tk
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'app'))
from workbench import Workbench
from transport import Decoder

class GuiTests(unittest.TestCase):
    def test_replay_live_hangup_and_export(self):
        root=tk.Tk(); ui=Workbench(root)
        fixture=Path(__file__).parent/'../firmware/build/demo.tc1'
        ui.replay_path(fixture); root.update()
        self.assertIn('CYCLE',ui.summary.get())
        self.assertEqual(len(ui.graph.find_withtag('edge')),3)
        self.assertGreater(len(ui.history.get_children()),3)
        for tab in range(4): ui.tabs.select(tab); root.update()
        with tempfile.TemporaryDirectory() as d:
            ui.export_to(Path(d)/'study.csv')
            self.assertIn('replay',(Path(d)/'study.csv').read_text())
        master,slave=pty.openpty()
        try:
            ui.path.set(os.ttyname(slave)); ui.connect(); root.update()
            self.assertEqual(ui.summary.get(),'No samples')
            self.assertEqual(len(ui.graph.find_all()),0)
            fd=ui.port.fd; ui.connect(); self.assertEqual(ui.port.fd,fd)
            # Sample every 20 device milliseconds; deliver paced in wall time.
            rows=fixture.read_bytes().splitlines(keepends=True)
            for line in rows[::20]:
                os.write(master,line)
                until=time.monotonic()+.020
                while time.monotonic()<until: root.update(); time.sleep(.002)
            until=time.monotonic()+.08
            while time.monotonic()<until: root.update(); time.sleep(.002)
            self.assertTrue(ui.session.fresh())
            self.assertIn('CYCLE',ui.summary.get())
            self.assertEqual(len(ui.graph.find_withtag('edge')),3)
            os.close(master); master=None
            until=time.monotonic()+.1
            while time.monotonic()<until: root.update(); time.sleep(.002)
            self.assertIsNone(ui.port); self.assertFalse(ui.session.fresh())
            self.assertIn('CYCLE',ui.summary.get())
            ui.path.set('/dev/null'); ui.connect()
            self.assertIn('Connection failed',ui.status.get())
            self.assertIn('CYCLE',ui.summary.get())
            ui.replay_path(fixture); self.assertFalse(ui.session.fresh())
        finally:
            if master is not None: os.close(master)
            os.close(slave); ui.close()

if __name__=='__main__': unittest.main()
