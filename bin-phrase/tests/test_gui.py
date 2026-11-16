# Author: jayis1. SPDX-License-Identifier: MIT
"""Actual Tk event-loop workflows with synthetic C-encoder input and Linux PTYs."""
import unittest, pathlib, sys, tkinter as tk, tempfile, os, pty, time
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/'app'))
from workbench import Workbench
ROOT=pathlib.Path(__file__).resolve().parents[1]
class GUI(unittest.TestCase):
    def test_replay_connection_export_and_hangup(self):
        root=tk.Tk(); app=Workbench(root)
        master,slave=pty.openpty()
        try:
            app.replay(ROOT/'firmware/build/demo.bq1'); root.update()
            self.assertIn('{B+C}',app.retained.get())
            self.assertEqual(len(app.table.get_children()),len(app.session.rows))
            with tempfile.TemporaryDirectory() as directory:
                path=pathlib.Path(directory)/'evidence.csv'; app.session.export(path,False)
                self.assertIn('replay',path.read_text())
            app.port.set('/dev/null'); app.connect(); self.assertEqual(app.session.source,'replay')
            app.port.set(os.ttyname(slave)); app.connect(); root.update()
            self.assertEqual(app.retained.get(),'No completed session')
            original=app.serial; app.connect(); self.assertIs(app.serial,original)
            data=(ROOT/'firmware/build/demo.bq1').read_bytes()
            for line in data.splitlines(keepends=True):
                os.write(master,line); root.update(); time.sleep(.002)
            for _ in range(5): root.update(); time.sleep(.06)
            self.assertEqual(app.session.rows[-1]['completed'],1)
            self.assertIn('{B+C}',app.retained.get())
            self.assertEqual(len(app.table.get_children()),len(app.session.rows))
            os.close(master); master=None
            for _ in range(3): root.update(); time.sleep(.06)
            self.assertIsNone(app.serial)
            self.assertFalse(app.session.fresh(False))
            self.assertEqual(app.session.source,'live')
        finally:
            app.close()
            if master is not None: os.close(master)
            os.close(slave)
if __name__=='__main__': unittest.main()
