# Author: jayis1. SPDX-License-Identifier: MIT
"""Exercise real Tk, production serial transport and real C encoder fixture."""
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
ROOT=Path(__file__).resolve().parents[1]

class UITests(unittest.TestCase):
    def test_workflow(self):
        root=tk.Tk(); app=Workbench(root)
        master,slave=pty.openpty()
        try:
            app.connect(os.ttyname(slave))
            with self.assertRaises(ValueError): app.connect(os.ttyname(slave))
            lines=(ROOT/'firmware/build/demo.rw1').read_bytes().splitlines(keepends=True)
            begin=time.monotonic()
            for line in lines:
                stamp=int(line.split(b',')[2])
                while time.monotonic()-begin < stamp/1000:
                    root.update(); time.sleep(.005)
                os.write(master,line)
                until=time.monotonic()+.035
                while time.monotonic()<until:
                    root.update(); time.sleep(.002)
            self.assertEqual(app.session.rows[-1]['good'],2)
            self.assertIn('48/96',app.summary.get())
            self.assertEqual(len(app.table.get_children()),len(app.session.rows))
            for tab in app.tabs.tabs():
                app.tabs.select(tab); root.update()
            self.assertTrue(app.chart.find_all())
            with tempfile.TemporaryDirectory() as tmp:
                app.session.export(Path(tmp)/'live.csv',time.monotonic())
                text=(Path(tmp)/'live.csv').read_text()
                self.assertIn('jayis1,live,True,ADVANCING',text)
                os.close(master); master=None
                until=time.monotonic()+.15
                while time.monotonic()<until:
                    root.update(); time.sleep(.005)
                self.assertFalse(app.session.connected)
                self.assertEqual(app.session.rows[-1]['good'],2)
                app.session.export(Path(tmp)/'disconnected.csv',time.monotonic())
                self.assertIn('live,False,DISCONNECTED',(Path(tmp)/'disconnected.csv').read_text())
                app.replay(ROOT/'firmware/build/demo.rw1')
                self.assertIn('REPLAY - NOT LIVE',app.status.get())
                self.assertEqual(app.session.rows[-1]['last'],48)
                old=app.session
                with self.assertRaises((OSError,ValueError)): app.replay(Path(tmp)/'missing')
                self.assertIs(old,app.session)
        finally:
            app.close()
            if master is not None: os.close(master)
            os.close(slave)

if __name__=='__main__': unittest.main()
