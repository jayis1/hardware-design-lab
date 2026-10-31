# Author: jayis1. SPDX-License-Identifier: MIT
import os, pathlib, pty, sys, tempfile, time, tkinter as tk, unittest
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/'app'))
from workbench import Workbench
from test_app import fixture
class GUI(unittest.TestCase):
    def test_real_tk_serial_replay_export(self):
        root=tk.Tk(); app=Workbench(root)
        master,slave=pty.openpty()
        try:
            app.path.set(os.ttyname(slave)); app.connect()
            self.assertTrue(app.session.connected)
            original=app.serial
            app.connect(); self.assertIs(app.serial,original)
            data=fixture().splitlines(keepends=True)
            for frame in data:
                os.write(master,frame)
                end=time.monotonic()+.1
                while time.monotonic()<end: root.update(); time.sleep(.005)
            self.assertEqual(len(app.session.rows),7)
            self.assertIn('compression',app.values.get())
            self.assertEqual(len(app.history.get_children()),7)
            self.assertEqual(len(app.tabs.tabs()),3)
            for tab in app.tabs.tabs(): app.tabs.select(tab); root.update()
            with tempfile.TemporaryDirectory() as tmp:
                p=pathlib.Path(tmp)/'live.csv'; app.export_to(p)
                self.assertIn('jayis1',p.read_text())
                os.close(master); master=None
                for _ in range(10): root.update(); time.sleep(.03)
                self.assertFalse(app.session.connected)
                p=pathlib.Path(tmp)/'capture'; p.write_bytes(b''.join(data))
                app.replay_from(p)
                self.assertEqual(app.session.source,'replay')
                self.assertEqual(len(app.session.rows),7)
                self.assertFalse(app.session.fresh())
                app.path.set('/nonexistent-bellows-port')
                app.connect()  # failed reopen must preserve replay evidence
                self.assertEqual(app.session.source,'replay')
                app.path.set('/dev/null')
                app.connect()  # termios.error is not OSError on every Python
                self.assertIn('Open failed',app.status.get())
                self.assertEqual(app.session.source,'replay')
        finally:
            app.close()
            if master is not None: os.close(master)
            os.close(slave)
if __name__=='__main__': unittest.main()
