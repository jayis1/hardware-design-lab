# Author: jayis1. SPDX-License-Identifier: MIT
import importlib.util
import json
import os
from pathlib import Path
import pty
import subprocess
import tempfile
import time
import termios
import unittest
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('cadence',ROOT/'app/cadence.py')
assert spec is not None and spec.loader is not None
cadence=importlib.util.module_from_spec(spec); spec.loader.exec_module(cadence)
FIXTURE=subprocess.check_output([str(ROOT/'firmware/build/test-engine'),'fixture'])
class Tests(unittest.TestCase):
    def test_real_encoder(self):
        rows=[cadence.parse(x) for x in FIXTURE.splitlines()]
        self.assertEqual(rows[0][5],4)
        self.assertEqual(rows[0][6],3)
        m=cadence.Model(); m.feed(FIXTURE,1.0)
        self.assertEqual(len(m.rows),2)
        self.assertTrue(m.fresh(1.2))
        m.feed(FIXTURE.splitlines()[-1]+b'\n',1.9)
        self.assertFalse(m.fresh(2.2))
    def test_corruption_and_partial(self):
        with self.assertRaises(ValueError): cadence.parse(b'TC1,0*0000')
        m=cadence.Model(); m.feed(FIXTURE+b'TC1,',1)
        with self.assertRaises(ValueError): m.finish()
    def test_reset_and_contradiction(self):
        m=cadence.Model(); m.feed(FIXTURE,1)
        m.feed(FIXTURE.splitlines()[0]+b'\n',1.1)
        self.assertTrue(m.blocked)
        self.assertEqual(len(m.rows),2)
    def test_semantic_corruption(self):
        row=cadence.parse(FIXTURE.splitlines()[0]); row[7]=99
        text=('TC1,'+','.join(map(str,row))).encode()
        line=text+b'*'+f'{cadence.crc(text):04X}'.encode()
        with self.assertRaises(ValueError): cadence.parse(line)
    def test_marker_resync_and_backlog(self):
        m=cadence.Model(); m.feed(b'TC1,broken'+FIXTURE,1)
        self.assertEqual(len(m.rows),2)
        self.assertEqual(m.errors,1)
        rows=FIXTURE.splitlines()
        m=cadence.Model('live'); m.feed(rows[0]+b'\n',1)
        m.feed(rows[1]+b'\n',3)
        self.assertTrue(m.blocked)
        self.assertEqual(len(m.rows),1)
    def test_serial_restores_attributes(self):
        master,slave=pty.openpty(); before=termios.tcgetattr(slave)
        try:
            port=cadence.Serial(os.ttyname(slave)); port.close()
            self.assertEqual(termios.tcgetattr(slave),before)
        finally:
            os.close(master); os.close(slave)
    def test_export_and_bound(self):
        m=cadence.Model(); m.feed(FIXTURE,1)
        with tempfile.TemporaryDirectory() as tmp:
            path=Path(tmp)/'capture.json'; m.export(path,2)
            saved=json.loads(path.read_text()); self.assertEqual(saved['author'],'jayis1')
            self.assertFalse(saved['fresh'])
            with self.assertRaises(FileExistsError): m.export(path,2)
            fifo=Path(tmp)/'pipe'; os.mkfifo(fifo)
            with self.assertRaises(ValueError): cadence.load_replay(fifo)
            path2=Path(tmp)/'large'; path2.write_bytes(b'x'*(cadence.MAX_REPLAY+1))
            with self.assertRaises(ValueError): cadence.load_replay(path2)
    def test_gui_serial_replay_disconnect(self):
        import tkinter as tk
        master,slave=pty.openpty(); root=tk.Tk(); app=cadence.App(root)
        try:
            app.path.set(os.ttyname(slave)); app.connect()
            self.assertIsNotNone(app.serial)
            os.write(master,FIXTURE)
            for _ in range(10): root.update(); time.sleep(.03)
            self.assertEqual(len(app.model.rows),2)
            self.assertIn('4',app.summary.get())
            os.close(master); master=-1
            for _ in range(10): root.update(); time.sleep(.03)
            self.assertIsNone(app.serial)
            self.assertIn('disconnected',app.status.get())
            self.assertEqual(app.model.source,'live')
            with tempfile.TemporaryDirectory() as tmp:
                export=Path(tmp)/'evidence.json'
                app.model.export(export,time.monotonic())
                self.assertFalse(json.loads(export.read_text())['fresh'])
                p=Path(tmp)/'synthetic.txt'; p.write_bytes(FIXTURE)
                app.replay_path(p)
                self.assertEqual(app.model.source,'replay')
                self.assertEqual(len(app.model.rows),2)
        finally:
            app.close(); os.close(slave)
            if master>=0: os.close(master)
if __name__=='__main__': unittest.main()
