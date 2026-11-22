# Author: jayis1. SPDX-License-Identifier: MIT
import csv
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
FIXTURE=Path(__file__).resolve().parents[1]/'firmware/build/demo.fr1'
class UI(unittest.TestCase):
    def test_real_tk_replay_live_export(self):
        root=tk.Tk(); app=Workbench(root)
        master,slave=pty.openpty()
        try:
            app.replay(FIXTURE); root.update()
            self.assertEqual(len(app.table.get_children()),16)
            self.assertIn('diagonal=2',app.summary.get())
            self.assertIn('offline',app.status.get())
            with tempfile.TemporaryDirectory() as tmp:
                path=Path(tmp)/'evidence.csv'; app.export(path)
                self.assertIn('jayis1',path.read_text())
                self.assertEqual(path.stat().st_mode&0o777,0o600)
                with self.assertRaises(FileExistsError): app.export(path)
            app.path.set(os.ttyname(slave)); app.connect(); root.update()
            self.assertEqual(app.summary.get(),'No samples')
            port=app.port; app.connect(); self.assertIs(app.port,port)
            lines=FIXTURE.read_bytes().splitlines(keepends=True)
            for index in (99,199,299):
                os.write(master,lines[index]); end=time.monotonic()+.10
                while time.monotonic()<end:
                    root.update(); time.sleep(.005)
            self.assertIn('LIVE acquisition',app.status.get())
            self.assertEqual(len(app.table.get_children()),16)
            os.close(master); master=None
            for _ in range(15): root.update(); time.sleep(.01)
            self.assertIsNone(app.port)
            self.assertIn('DISCONNECTED',app.status.get())
            self.assertEqual(app.session.source,'live')
        finally:
            app.close()
            if master is not None: os.close(master)
            os.close(slave)
if __name__=='__main__': unittest.main()
