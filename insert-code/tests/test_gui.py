# Author: jayis1. SPDX-License-Identifier: MIT
import os, pathlib, pty, sys, tempfile, termios, time, tkinter as tk, unittest
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/'app'))
from main import App
from transport import Decoder
ROOT=pathlib.Path(__file__).resolve().parents[1]
class GuiTests(unittest.TestCase):
    def setUp(self):
        self.root=tk.Tk(); self.app=App(self.root)
        self.data=(ROOT/'firmware/build/demo.ic1').read_bytes()
    def tearDown(self): self.app.close()
    def test_replay_export_and_failure(self):
        self.app.replay(str(ROOT/'firmware/build/demo.ic1')); self.root.update()
        self.assertEqual(len(self.app.table.get_children()),6)
        self.assertEqual(len(self.app.log.get_children()),100)
        self.assertIn('completed 3',self.app.info.get())
        with tempfile.TemporaryDirectory() as tmp:
            path=pathlib.Path(tmp)/'output.csv'; self.app.export(str(path))
            self.assertIn('jayis1',path.read_text()); self.assertEqual(path.stat().st_mode&0o777,0o600)
            self.app.export(str(path)); self.assertIn('exists',self.app.status.get())
        self.app.path.set('/dev/null'); self.app.connect()
        self.assertIsNone(self.app.port); self.assertEqual(len(self.app.session.rows),100)
    def test_live_pty_disconnect(self):
        master,slave=pty.openpty(); original=termios.tcgetattr(slave)
        try:
            self.app.path.set(os.ttyname(slave)); self.app.connect()
            self.app.connect(); self.assertIn('Already',self.app.status.get())
            os.write(master,self.data)
            deadline=time.monotonic()+1
            while len(self.app.session.rows)<100 and time.monotonic()<deadline:
                self.root.update(); time.sleep(.01)
            self.assertEqual(len(self.app.log.get_children()),100)
            self.assertTrue(self.app.session.fresh())
            self.app.disconnect(); self.assertEqual(termios.tcgetattr(slave),original)
            self.assertFalse(self.app.session.fresh())
            self.app.connect(); os.close(master); master=None
            deadline=time.monotonic()+1
            while self.app.port is not None and time.monotonic()<deadline:
                self.root.update(); time.sleep(.01)
            self.assertIsNone(self.app.port)
        finally:
            if master is not None: os.close(master)
            os.close(slave)
    def test_partial_batch_is_rendered_on_error(self):
        rows=Decoder().feed(self.data)
        self.app.session.accept(rows[0]); self.app.render()
        # Read first row again after valid advance to cause a session rollback.
        class FakePort:
            def read(inner): return b''
            def close(inner): pass
        self.app.port=FakePort()
        self.app.decoder.feed=lambda data: [rows[1],rows[0]]
        self.root.after_cancel(self.app.timer)
        self.app.poll()
        displayed=self.app.log.item(self.app.log.get_children()[-1])['values'][0]
        self.assertEqual(int(displayed),rows[1]['now'])
if __name__=='__main__': unittest.main()
