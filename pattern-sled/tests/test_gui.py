# Author: jayis1. SPDX-License-Identifier: MIT
"""Real Tk loop + Linux PTY, fed SYNTHETIC production-C encoder output."""
import os
from pathlib import Path
import pty
import subprocess
import sys
import tempfile
import time
import tkinter as tk
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'app'))
from workbench import Workbench
ROOT=Path(__file__).resolve().parents[1]

class GuiTest(unittest.TestCase):
    def test_workflow(self):
        with tempfile.TemporaryDirectory() as d:
            executable=Path(d)/'fixture'
            subprocess.run(['gcc','-std=c11','-Wall','-Wextra','-Werror','-I'+str(ROOT/'firmware'),
                str(ROOT/'tests/fixture.c'),str(ROOT/'firmware/engine.c'),str(ROOT/'firmware/wire.c'),'-o',str(executable)],check=True)
            data=subprocess.check_output([executable]); path=Path(d)/'synthetic.ps1'; path.write_bytes(data)
            root=tk.Tk(); app=Workbench(root)
            def pump(seconds):
                end=time.monotonic()+seconds
                while time.monotonic()<end: root.update(); time.sleep(.005)
            try:
                app.replay(path); root.update()
                self.assertEqual(app.session.rows[-1]['words'],48)
                self.assertEqual(app.session.rows[-1]['bad'],16)
                self.assertEqual(len(app.grid.get_children()),16)
                self.assertEqual(len(app.history.get_children()),len(data.splitlines()))
                self.assertIn('REPLAY',app.status.get())
                for tab in app.tabs.tabs(): app.tabs.select(tab); root.update()
                export=Path(d)/'evidence.csv'; app.session.export(export)
                self.assertIn('jayis1',export.read_text())
                app.connect('/dev/null'); self.assertEqual(app.session.source,'replay')
                m,s=pty.openpty()
                try:
                    app.connect(os.ttyname(s)); root.update()
                    self.assertEqual(len(app.grid.get_children()),0)
                    owned=app.port; app.connect(os.ttyname(s)); self.assertIs(app.port,owned)
                    # Emulate loss of the first partial frame during late attachment.
                    os.write(m,b'fragment\n')
                    for frame in data.splitlines(keepends=True):
                        os.write(m,frame); pump(.1)
                    self.assertEqual(app.session.rows[-1]['words'],48)
                    self.assertEqual(app.session.errors,1)
                    self.assertTrue(app.session.fresh())
                    os.close(m); m=None; pump(.1)
                    self.assertFalse(app.session.connected)
                    self.assertIsNone(app.port)
                    self.assertEqual(app.session.rows[-1]['bad'],16)
                    self.assertFalse(app.session.fresh())
                finally:
                    if m is not None: os.close(m)
                    os.close(s)
                app.replay(path); root.update()
                self.assertEqual(app.session.source,'replay')
            finally: app.close()

if __name__=='__main__': unittest.main()
