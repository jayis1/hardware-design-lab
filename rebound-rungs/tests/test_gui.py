# Author: jayis1. SPDX-License-Identifier: MIT
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
from test_app import frame, idle

class GuiTests(unittest.TestCase):
    def pump(self,root,duration=.15):
        end=time.monotonic()+duration
        while time.monotonic()<end:
            root.update(); time.sleep(.005)
    def test_live_replay_export_and_disconnect(self):
        root=tk.Tk(); app=Workbench(root)
        master,slave=pty.openpty()
        try:
            app.port.set(os.ttyname(slave)); app.connect()
            for i in range(4):
                os.write(master,frame(idle(100+i*100)))
                self.pump(root,.1)
            self.assertEqual(len(app.session.rows),4)
            self.assertTrue(app.session.fresh())
            self.assertIn('waiting',app.result.get())
            old_serial=app.serial
            app.connect(); self.assertIs(app.serial,old_serial)
            for index in range(3):
                app.tabs.select(index); self.pump(root,.03)
            with tempfile.TemporaryDirectory() as directory:
                output=Path(directory)/'result.csv'
                app.output.set(str(output)); app.fixture.set('PTY synthetic')
                app.export(); self.assertTrue(output.exists())
                app.export(); self.assertIn('exists',app.message.get())
                recording=Path(directory)/'capture.txt'
                recording.write_bytes(frame(idle(100))+frame(idle(200)))
                os.close(master); master=None; self.pump(root)
                self.assertIsNone(app.serial)
                self.assertFalse(app.session.connected)
                self.assertEqual(len(app.session.rows),4)
                app.replay.set(str(recording)); app.load()
                self.assertEqual(app.session.provenance,'replay')
                self.assertEqual(len(app.session.rows),2)
                app.port.set('/dev/null'); app.connect()
                self.assertEqual(app.session.provenance,'replay')
        finally:
            app.close()
            if master is not None: os.close(master)
            os.close(slave)

if __name__=='__main__': unittest.main()
