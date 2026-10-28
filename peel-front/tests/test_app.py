# Author: jayis1. SPDX-License-Identifier: MIT
import binascii
import importlib.util
import os
from pathlib import Path
import pty
import tempfile
import time
import unittest
import tkinter as tk
ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('peel', ROOT/'app/peel.py')
assert spec is not None and spec.loader is not None
p = importlib.util.module_from_spec(spec)
spec.loader.exec_module(p)

def frame(values):
    body = 'PF1,' + ','.join(map(str, values))
    return f'{body}*{binascii.crc_hqx(body.encode(), 65535):04X}\n'.encode()

class Tests(unittest.TestCase):
    def setUp(self):
        self.data = (ROOT/'firmware/build/fixture.txt').read_bytes()
        self.values = p.decode(self.data)
    def test_encoder_and_invariants(self):
        self.assertEqual(self.values[10:14], (0,20,50,70))
        self.assertEqual(p.metrics(self.values), (20,20,50,50))
        for index, value in [(1,5),(2,16),(6,8),(10,2),(13,19),(14,69)]:
            bad=list(self.values); bad[index]=value
            with self.assertRaises(ValueError): p.decode(frame(bad))
        with self.assertRaises(ValueError): p.decode(self.data.replace(b'PF1',b'PF2'))
    def test_stream_recovery(self):
        model=p.Model()
        model.feed(self.data[:20]+self.data, 1)
        self.assertEqual(len(model.rows),1)
        self.assertGreater(model.errors,0)
        model.feed(self.data,2)
        self.assertEqual(model.received,1)
        bad=list(self.values); bad[2]=15
        model.feed(frame(bad),3)
        self.assertTrue(model.blocked)
    def test_replay_export(self):
        with tempfile.TemporaryDirectory() as d:
            path=Path(d)/'trace'; path.write_bytes(self.data)
            m=p.Model(); m.replay(path)
            output=Path(d)/'out.csv'; m.export(output)
            self.assertIn('jayis1',output.read_text())
            self.assertIn('replay',output.read_text())
            self.assertEqual(output.stat().st_mode & 0o777,0o600)
            with self.assertRaises(FileExistsError): m.export(output)
            path.write_bytes(self.data+b'PF1,12')
            with self.assertRaises(ValueError): p.Model().replay(path)
            path.unlink(); os.mkfifo(path)
            with self.assertRaises(ValueError): p.Model().replay(path)
    def test_live_backlog_and_reset(self):
        m=p.Model(); m.source='live'
        m.feed(self.data,10)
        forward=list(self.values); forward[0]+=50
        m.feed(frame(forward),12)
        self.assertTrue(m.blocked)
        self.assertIn('Reception pause',m.warning)
        m=p.Model(); m.feed(self.data,1)
        rollback=list(self.values); rollback[0]-=1
        m.feed(frame(rollback),2)
        self.assertTrue(m.blocked)
        m=p.Model(); m.source='live'; m.feed(self.data,0)
        for i in range(1,10):
            forward=list(self.values); forward[0]+=i
            m.feed(frame(forward),i*.1)
        self.assertTrue(m.blocked)
        self.assertIn('progress mismatch',m.warning)
    def test_serial_double_connect_and_restore(self):
        import termios
        root=tk.Tk(); app=p.App(root)
        master,slave=pty.openpty()
        before=termios.tcgetattr(slave)
        try:
            app.path.set(os.ttyname(slave)); app.connect()
            port=app.serial; app.connect(); self.assertIs(app.serial,port)
            app.disconnect()
            self.assertEqual(termios.tcgetattr(slave),before)
        finally:
            app.close(); os.close(master); os.close(slave)
    def test_ui_serial_disconnect(self):
        root=tk.Tk(); app=p.App(root)
        master,slave=pty.openpty()
        try:
            app.path.set(os.ttyname(slave)); app.connect()
            os.write(master,self.data)
            for _ in range(15): root.update(); time.sleep(.01)
            self.assertEqual(len(app.model.rows),1)
            self.assertEqual(len(app.table.get_children()),1)
            self.assertIn('COMPLETE',app.summary.get())
            for page in app.tabs.tabs():
                app.tabs.select(page); root.update()
            os.close(master); master=None
            for _ in range(15): root.update(); time.sleep(.01)
            self.assertIsNone(app.serial)
            self.assertEqual(app.model.source,'live')
            self.assertEqual(len(app.model.rows),1)
        finally:
            app.close()
            if master is not None: os.close(master)
            os.close(slave)

if __name__ == '__main__': unittest.main()
