# Author: jayis1. SPDX-License-Identifier: MIT
import os
from pathlib import Path
import pty
import subprocess
import sys
import tempfile
import termios
import unittest
from unittest.mock import patch
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'app'))
from protocol import decode, encode, Model, Stream, read_replay
from relay import Serial, Application
import tkinter as tk
ROOT=Path(__file__).resolve().parents[1]
class Tests(unittest.TestCase):
    def setUp(self):
        self.line=subprocess.check_output([str(ROOT/'firmware/build/test-engine'),'fixture']).decode()
        self.row=decode(self.line)
    def test_real_encoder(self):
        self.assertEqual(self.row['overlap_ms'],30)
        self.assertEqual(self.row['completed'],1)
        self.assertEqual(encode(self.row),self.line)
    def test_impossible(self):
        for key,value in [('completed',2),('gap_ms',10001),('max_gap_ms',1),('grips',0),('qualify',6),('faults',1)]:
            row=self.row.copy(); row[key]=value
            with self.assertRaises(ValueError): decode(encode(row))
        with self.assertRaises(ValueError): decode(self.line.replace('*','*F'))
    def test_stream(self):
        s=Stream(); rows=s.feed(b'partial'+self.line[:20].encode()+self.line.encode())
        self.assertEqual(len(rows),1)
        self.assertGreater(s.errors,0)
        s.feed(b'CR1,5')
        with self.assertRaises(ValueError): s.finish()
    def test_freshness(self):
        m=Model(); m.accept(self.row,0)
        self.assertFalse(m.fresh(0))
        row=self.row.copy(); row['now']+=100; m.accept(row,.1)
        self.assertTrue(m.fresh(.1))
        m.accept(row,.5); self.assertFalse(m.fresh(1.2))
        row=row.copy(); row['now']+=100
        with self.assertRaises(ValueError): m.accept(row,3)
        self.assertFalse(m.fresh(3))
    def test_counter_rollback(self):
        m=Model(); m.accept(self.row,0)
        row=self.row.copy(); row['now']+=100; row['tx_errors']=1; m.accept(row,.1)
        row=row.copy(); row['now']+=100; row['tx_errors']=0
        with self.assertRaises(ValueError): m.accept(row,.2)
    def test_gradual_backlog_and_duplicate(self):
        m=Model(); m.accept(self.row,0)
        with self.assertRaises(ValueError):
            for i in range(1,100):
                row=self.row.copy(); row['now']+=i*10
                m.accept(row,i*.1)
        self.assertTrue(m.blocked)
        m=Model(); m.accept(self.row,0)
        row=self.row.copy(); row['tx_errors']=1
        with self.assertRaises(ValueError): m.accept(row,.1)
    def test_serial_restoration(self):
        master,slave=pty.openpty(); path=os.ttyname(slave)
        original=termios.tcgetattr(slave)
        try:
            port=Serial(path); port.close()
            self.assertEqual(termios.tcgetattr(slave),original)
            with patch('relay.termios.tcflush',side_effect=OSError('injected flush failure')):
                with self.assertRaises(OSError): Serial(path)
            self.assertEqual(termios.tcgetattr(slave),original)
        finally:
            os.close(master); os.close(slave)
    def test_replay_bounds(self):
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'record'; p.write_text(self.line)
            self.assertEqual(len(read_replay(p)),1)
            p.write_text(self.line+'CR1,')
            with self.assertRaises(ValueError): read_replay(p)
            p.unlink(); os.mkfifo(p)
            with self.assertRaises(ValueError): read_replay(p)
    def test_gui_serial_export(self):
        master,slave=pty.openpty(); path=os.ttyname(slave)
        root=tk.Tk(); app=Application(root)
        try:
            app.connect(path)
            os.write(master,self.line.encode()); app.poll(); root.update()
            row=self.row.copy(); row['now']+=100
            os.write(master,encode(row).encode()); app.poll(); root.update()
            self.assertEqual(len(app.model.rows),2)
            self.assertIn('completed=1',app.summary.get())
            for i in range(3): app.tabs.select(i); root.update()
            with tempfile.TemporaryDirectory() as d:
                p=Path(d)/'capture.csv'; app.export(p)
                data=p.read_text(); self.assertIn('jayis1',data); self.assertIn('live',data)
                with self.assertRaises(FileExistsError): app.export(p)
            with self.assertRaises(RuntimeError): app.connect(path)
            os.close(master); master=-1; app.poll(); root.update()
            self.assertIsNone(app.serial)
            self.assertEqual(app.provenance,'live')
            with tempfile.TemporaryDirectory() as d:
                p=Path(d)/'disconnected.csv'; app.export(p)
                self.assertIn('connected,False,fresh,False',p.read_text())
                self.assertIn('host_age_seconds',p.read_text())
        finally:
            app.close(); root.destroy(); os.close(slave)
            if master>=0: os.close(master)
if __name__=='__main__': unittest.main()
