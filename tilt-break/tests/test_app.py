#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
import importlib.util
import pathlib
import unittest
import tempfile
import os
import pty
import time
import tkinter as tk
import binascii
import termios
ROOT = pathlib.Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('tilt', ROOT/'app/tilt.py')
assert spec is not None and spec.loader is not None
a = importlib.util.module_from_spec(spec)
spec.loader.exec_module(a)

class TestApp(unittest.TestCase):
    def setUp(self):
        self.line = (ROOT/'firmware/build/fixture.txt').read_bytes()
    def test_actual_encoder(self):
        row = a.decode(self.line)
        self.assertEqual(row['state'], 5)
        self.assertEqual(row['transit_ms'], 200)
        with self.assertRaises(ValueError):
            a.decode(self.line.replace(b'TB1', b'TB2'))
    def test_crc_and_partial(self):
        with self.assertRaises(ValueError): a.decode(self.line[:-1])
        with self.assertRaises(ValueError): a.decode(self.line.replace(b',200,',b',201,'))
    def test_duplicates_and_stale(self):
        m=a.Model(); m.source='live'; m.accept(self.line,1)
        m.accept(self.line,2)
        self.assertEqual(len(m.rows),1)
        self.assertEqual(m.last_host,1)
        m.check_age(4)
        self.assertTrue(m.blocked)
    def frame(self,**changes):
        row=a.decode(self.line); row.update(changes)
        body=('TB1,'+','.join(str(row[k]) for k in a.FIELDS)).encode()
        return body+f',{binascii.crc_hqx(body,0xffff):04X}\n'.encode()
    def test_valid_crc_impossible_records(self):
        for changes in ({'lo':1000,'hi':0},{'release':0},{'faults':1},
                        {'transit_ms':10001},{'adc0':4096},{'state':7}):
            with self.assertRaises(ValueError): a.decode(self.frame(**changes))
    def test_mutation_and_rollback(self):
        for changes in ({'transit_ms':201},{'baseline':123}, {'seq':1},
                        {'ms':1},{'state':3},{'lo':1}):
            m=a.Model(); m.source='replay'; m.accept(self.line)
            with self.assertRaises(ValueError): m.accept(self.frame(**changes))
            self.assertTrue(m.blocked)
    def test_empty_export(self):
        with tempfile.TemporaryDirectory() as d:
            path=pathlib.Path(d)/'empty.csv'
            a.Model().export(path,'no run',1,1)
            self.assertIn('no run',path.read_text())
    def test_failed_connect_keeps_evidence(self):
        root=tk.Tk(); app=a.App(root)
        try:
            app.model.source='replay'; app.model.accept(self.line)
            app.port.set('/this/path/does/not/exist'); app.connect()
            self.assertEqual(len(app.model.rows),1)
            self.assertEqual(app.model.source,'replay')
        finally: app.close()
    def test_export_and_replay(self):
        with tempfile.TemporaryDirectory() as d:
            path=pathlib.Path(d)/'record.txt'; path.write_bytes(self.line)
            m=a.Model(); m.replay(path)
            out=pathlib.Path(d)/'out.csv'; m.export(out,'test',2222,100)
            self.assertIn('jayis1',out.read_text())
            self.assertEqual(out.stat().st_mode & 0o777,0o600)
            with self.assertRaises(FileExistsError): m.export(out,'',2222,100)
            path.write_bytes(self.line+b'TB1,')
            with self.assertRaises(ValueError): a.Model().replay(path)
            fifo=pathlib.Path(d)/'fifo'; os.mkfifo(fifo)
            with self.assertRaises(ValueError): a.Model().replay(fifo)
    def test_gui_serial_disconnect(self):
        root=tk.Tk(); app=a.App(root)
        master,slave=pty.openpty()
        try:
            app.port.set(os.ttyname(slave)); app.connect()
            os.write(master,self.line)
            for _ in range(8): root.update(); time.sleep(.03)
            self.assertEqual(len(app.model.rows),1)
            self.assertIn('DONE',app.status.get())
            for i in range(3): app.tabs.select(i); root.update()
            os.close(master); master=None
            for _ in range(8): root.update(); time.sleep(.03)
            self.assertIsNone(app.serial)
            self.assertEqual(app.model.source,'live')
        finally:
            app.close(); os.close(slave)
            if master is not None: os.close(master)
    def test_partial_first_frame_and_double_connect(self):
        root=tk.Tk(); app=a.App(root)
        master,slave=pty.openpty()
        original=termios.tcgetattr(slave)
        try:
            app.port.set(os.ttyname(slave)); app.connect()
            first=app.serial
            configured=termios.tcgetattr(slave)
            app.connect()
            self.assertIs(app.serial,first)
            self.assertEqual(termios.tcgetattr(slave),configured)
            os.write(master,self.line[10:]+self.line)
            for _ in range(8): root.update(); time.sleep(.03)
            self.assertEqual(len(app.model.rows),1)
            self.assertFalse(app.model.blocked)
            self.assertEqual(app.model.errors,1)
            app.disconnect()
            self.assertEqual(termios.tcgetattr(slave),original)
        finally:
            app.close(); os.close(slave); os.close(master)

if __name__=='__main__': unittest.main()
