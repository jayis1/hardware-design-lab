# Author: jayis1. SPDX-License-Identifier: MIT
"""Exercise the real Tk event loop and production serial adapter with a PTY."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import time
import tkinter as tk
import unittest
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'app'))
from workbench import Workbench

class GuiTests(unittest.TestCase):
    def test_workflow(self):
        frames=subprocess.check_output([str(ROOT/'firmware/build/test-engine'),'--fixture']).splitlines(keepends=True)
        master,slave=os.openpty()
        window=tk.Tk(); app=Workbench(window)
        def pump(seconds=0.12):
            end=time.monotonic()+seconds
            while time.monotonic()<end:
                window.update(); time.sleep(0.005)
        try:
            self.assertEqual(len(app.tabs.tabs()),3)
            app.path.set(os.ttyname(slave)); app.connect()
            self.assertIsNotNone(app.port)
            old=app.port; app.connect(); self.assertIs(app.port,old)
            os.write(master,frames[0]); pump()
            os.write(master,frames[1]); pump()
            self.assertEqual(len(app.history.get_children()),2)
            self.assertIn('A > B > D',app.summary.get())
            self.assertTrue(app.session.fresh())
            for tab in app.tabs.tabs(): app.tabs.select(tab); window.update()
            with tempfile.TemporaryDirectory() as tmp:
                output=Path(tmp)/'evidence.json'; app.save(output)
                self.assertTrue(output.exists())
                app.save(output); self.assertIn('failed',app.status.get())
                os.close(master); master=None; pump()
                self.assertIsNone(app.port)
                self.assertFalse(app.session.fresh())
                replay=Path(tmp)/'fixture'; replay.write_bytes(b''.join(frames))
                app.replay(replay)
                self.assertEqual(app.session.source,'replay')
                self.assertEqual(len(app.history.get_children()),2)
                # Reconnect must clear retained numerical summaries before data.
                master2,slave2=os.openpty()
                try:
                    app.path.set(os.ttyname(slave2)); app.connect()
                    self.assertEqual(app.summary.get(),'No evidence')
                    app.disconnect()
                finally: os.close(master2); os.close(slave2)
        finally:
            app.close()
            if master is not None: os.close(master)
            os.close(slave)
if __name__=='__main__': unittest.main()
