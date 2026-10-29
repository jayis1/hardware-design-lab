# Author: jayis1. SPDX-License-Identifier: MIT
import csv
import os
from pathlib import Path
import pty
import subprocess
import sys
import tempfile
import tkinter as tk
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'app'))
from protocol import decode, encode, validate, Model, Stream, read_replay
from pipette import Application, Serial
ROOT=Path(__file__).resolve().parents[1]
class Tests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.frames=subprocess.check_output([str(ROOT/'firmware/build/test-engine'),'fixture']).decode().splitlines(keepends=True)
        cls.rows=[decode(s) for s in cls.frames]
    def test_encoder(self):
        self.assertEqual(self.rows[-1]['completed'],1)
        self.assertEqual({r['phase'] for r in self.rows},{1,2,3,4,5,6,7,8})
        for row in self.rows: self.assertEqual(decode(encode(row)),row)
    def test_invalid(self):
        row=self.rows[0].copy(); row.update(phase=6,levels=15,starts=1)
        with self.assertRaises(ValueError): decode(encode(row))
        for key,value in [('starts',5),('phase',9),('qualify',6),('last_reason',9),('elapsed',30000)]:
            row=self.rows[0].copy(); row[key]=value
            with self.assertRaises(ValueError): decode(encode(row))
    def test_framing(self):
        stream=Stream()
        self.assertEqual(stream.feed(b'partialPP1,5'+self.frames[0].encode()),[self.rows[0]])
        self.assertGreater(stream.errors,0)
        stream.feed(b'PP1,')
        with self.assertRaises(ValueError): stream.finish()
        stream=Stream(); stream.feed(b'x'*5000); self.assertLess(len(stream.buffer),512)
    def test_model(self):
        m=Model(); m.accept(self.rows[0],0); self.assertFalse(m.fresh(0))
        dt=(self.rows[1]['now']-self.rows[0]['now'])/1000
        m.accept(self.rows[1],dt); self.assertTrue(m.fresh(dt))
        m.accept(self.rows[1],dt+.5); self.assertFalse(m.fresh(dt+1.01))
        row=self.rows[1].copy(); row['now']+=100
        with self.assertRaises(ValueError): m.accept(row,10)
        self.assertTrue(m.blocked)
    def test_replay(self):
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'record'; p.write_text(''.join(self.frames))
            self.assertEqual(read_replay(p),self.rows)
            p.write_text(''.join(self.frames)+'PP1,')
            with self.assertRaises(ValueError): read_replay(p)
            p.unlink(); os.mkfifo(p)
            with self.assertRaises(ValueError): read_replay(p)
    def test_gui_serial_export(self):
        root=tk.Tk(); root.withdraw(); app=Application(root)
        master,slave=pty.openpty()
        try:
            app.connect(os.ttyname(slave))
            with self.assertRaises(RuntimeError): app.connect(os.ttyname(slave))
            os.write(master,''.join(self.frames).encode()); app.poll(); root.update()
            self.assertEqual(len(app.table.get_children()),len(self.rows))
            self.assertIn('completed=1',app.summary.get())
            for tab in (app.connection,app.live,app.history):
                app.tabs.select(tab); root.update()
            with tempfile.TemporaryDirectory() as d:
                target=Path(d)/'evidence.csv'; app.disconnect(); app.export(target)
                with target.open() as source: rows=list(csv.reader(source))
                self.assertIn('live',rows[0]); self.assertIn('False',rows[0])
                self.assertEqual(target.stat().st_mode&0o777,0o600)
                with self.assertRaises(FileExistsError): app.export(target)
                replay=Path(d)/'replay'; replay.write_text(''.join(self.frames))
                app.replay(replay); self.assertEqual(app.provenance,'replay')
            app.connect(os.ttyname(slave)); self.assertEqual(app.summary.get(),'No evidence')
            os.close(master); master=None; app.poll(); self.assertIsNone(app.serial)
        finally:
            app.close(); root.destroy()
            if master is not None: os.close(master)
            os.close(slave)
if __name__=='__main__': unittest.main()
