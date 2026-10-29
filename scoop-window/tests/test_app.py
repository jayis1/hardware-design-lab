# Author: jayis1. SPDX-License-Identifier: MIT
import copy
import os
from pathlib import Path
import pty
import subprocess
import sys
import tempfile
import termios
import time
import tkinter as tk
import unittest
from unittest.mock import patch
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'app'))
from protocol import FIELDS, Model, Stream, decode, encode, read_replay
from scoop import Application, Serial
ROOT=Path(__file__).resolve().parents[1]

def fixture():
    result=subprocess.run([str(ROOT/'firmware/build/test-engine'),'fixture'],check=True,capture_output=True,text=True)
    return decode(result.stdout)

class Contract(unittest.TestCase):
    def test_two_thousand_real_engine_snapshots(self):
        result=subprocess.run([str(ROOT/'firmware/build/test-engine'),'trace'],check=True,capture_output=True)
        stream=Stream(); rows=stream.feed(result.stdout); stream.finish()
        self.assertEqual(stream.errors,0)
        self.assertEqual(len(rows),2000)
        model=Model()
        for i,row in enumerate(rows): model.accept(row,i*.001,live=False)
        self.assertGreater(model.last['gaps'],0)
        self.assertGreater(model.last['a_aborted'],0)
        self.assertGreater(model.last['b_flagged'],0)
    def test_real_encoder_and_framing(self):
        row=fixture()
        self.assertEqual(row['a_flagged'],1)
        packet=encode(row).encode()
        stream=Stream()
        self.assertEqual(stream.feed(b'broken'+packet[:9]),[])
        self.assertEqual(stream.feed(packet[9:]),[row])
        self.assertEqual(stream.errors,1)
        stream.feed(b'SW1,5'); self.assertRaises(ValueError,stream.finish)
        stream=Stream(); stream.feed(b'x'*10000)
        self.assertLessEqual(len(stream.buffer),511)
        self.assertRaises(ValueError,decode,packet.decode().replace('SW1','XX1'))
    def test_impossible_valid_crc(self):
        row=fixture()
        for key,value in [('a_foreign_ms',row['a_away_ms']+1),('a_completed',99),('active',1),('faults',8),('qualify',6)]:
            bad=row.copy(); bad[key]=value
            self.assertRaises(ValueError,decode,encode(bad))
    def test_model_continuity(self):
        row=fixture(); model=Model(); model.accept(row,10)
        self.assertFalse(model.fresh(10))
        later=row.copy(); later['now']+=100
        model.accept(later,10.1); self.assertTrue(model.fresh(10.2))
        model.accept(later,10.8); self.assertFalse(model.fresh(11.2))
        later['a_flagged']=0
        self.assertRaises(ValueError,model.accept,later,10.9)
        self.assertTrue(model.blocked)
        for delta,host in [(0xffffffff,10.1),(100,12),(10000,10.1)]:
            model=Model(); model.accept(row,10)
            later=row.copy(); later['now']=(row['now']+delta)&0xffffffff
            self.assertRaises(ValueError,model.accept,later,host)
    def test_durations_and_terminal(self):
        row=fixture(); model=Model(); model.accept(row,0)
        later=row.copy(); later['now']+=10; later['a_away_ms']+=11
        self.assertRaises(ValueError,model.accept,later,.01)
        row['faults']=1; row['ready']=0
        model=Model(); model.accept(row,0)
        later=row.copy(); later['now']+=100; later['a_away_ms']+=1
        self.assertRaises(ValueError,model.accept,later,.1)
    def test_replay_boundaries(self):
        with tempfile.TemporaryDirectory() as tmp:
            path=Path(tmp)/'data'; path.write_text(encode(fixture()))
            self.assertEqual(len(read_replay(path)),1)
            path.write_text(path.read_text()+'SW1,')
            self.assertRaises(ValueError,read_replay,path)
            path.write_bytes(b'')
            self.assertRaises(ValueError,read_replay,path)
            path.write_bytes(b'x'*2_000_001)
            self.assertRaises(ValueError,read_replay,path)
            fifo=Path(tmp)/'fifo'; os.mkfifo(fifo)
            self.assertRaises(ValueError,read_replay,fifo)
    def test_serial_restore_and_failure(self):
        master,slave=pty.openpty()
        original=termios.tcgetattr(slave)
        try:
            port=Serial(os.ttyname(slave)); port.close()
            self.assertEqual(termios.tcgetattr(slave),original)
            with patch('scoop.termios.tcflush',side_effect=OSError('injected')):
                self.assertRaises(OSError,Serial,os.ttyname(slave))
            self.assertEqual(termios.tcgetattr(slave),original)
        finally: os.close(master); os.close(slave)

class GUI(unittest.TestCase):
    def setUp(self):
        self.root=tk.Tk(); self.app=Application(self.root); self.root.update()
    def tearDown(self):
        self.app.close(); self.root.destroy()
    def test_terminal_state_label(self):
        row=fixture(); row['faults']=1; row['ready']=0
        self.app.model.accept(row,0,live=False)
        self.app.render()
        self.assertIn('HALTED',self.app.summary.get())
    def test_live_replay_export_disconnect(self):
        master,slave=pty.openpty()
        try:
            self.app.connect(os.ttyname(slave))
            self.assertRaises(RuntimeError,self.app.connect,os.ttyname(slave))
            row=fixture()
            os.write(master,b'tail'+encode(row).encode())
            time.sleep(.02); self.app.poll(); self.root.update()
            self.assertEqual(self.app.model.last,row)
            self.assertEqual(self.app.stream.errors,1)
            time.sleep(.1); row['now']+=100
            os.write(master,encode(row).encode())
            time.sleep(.02); self.app.poll(); self.root.update()
            self.assertIn('LIVE',self.app.status.get())
            self.assertIn('foreign',self.app.summary.get())
            self.assertEqual(len(self.app.table.get_children()),2)
            for page in range(3): self.app.tabs.select(page); self.root.update()
            with tempfile.TemporaryDirectory() as tmp:
                export=Path(tmp)/'e.csv'; self.app.export(export)
                self.assertIn('jayis1',export.read_text())
                self.assertEqual(export.stat().st_mode&0o777,0o600)
                self.assertRaises(FileExistsError,self.app.export,export)
                os.close(master); master=None
                self.app.poll(); self.root.update()
                self.assertIsNone(self.app.serial)
                self.assertIn('DISCONNECTED',self.app.status.get())
                retained=copy.deepcopy(self.app.model.last)
                self.assertRaises(OSError,self.app.connect,'/nonexistent-scoop-port')
                self.assertEqual(self.app.model.last,retained)
                replay=Path(tmp)/'r.txt'; replay.write_text(encode(row))
                self.app.replay(replay); self.root.update()
                self.assertEqual(self.app.provenance,'replay')
                self.assertEqual(len(self.app.table.get_children()),1)
                self.assertIn('replay',self.app.status.get())
        finally:
            if master is not None: os.close(master)
            os.close(slave)

if __name__=='__main__': unittest.main()
