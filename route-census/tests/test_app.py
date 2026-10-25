# Author: jayis1. SPDX-License-Identifier: MIT
import importlib.util
from pathlib import Path
import unittest
import os
import pty
import tempfile
import time
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('census', ROOT/'app/census.py')
assert spec is not None and spec.loader is not None
m=importlib.util.module_from_spec(spec); spec.loader.exec_module(m)
FRAME=(ROOT/'firmware/build/fixture.txt').read_bytes()
class Tests(unittest.TestCase):
    def test_incomplete_replay(self):
        model=m.Model(); model.feed(FRAME+FRAME[:30]); model.finish()
        self.assertEqual(model.errors,1)
    def test_encoder_roundtrip(self):
        row=m.decode(FRAME.strip())
        self.assertEqual(row['accepted'],1)
        self.assertEqual(row['routes'][0]['count'],1)
        self.assertEqual(row['last_ms'],40)
    def test_crc_and_invariant(self):
        with self.assertRaises(ValueError): m.decode(FRAME.replace(b'RTC1',b'RTC2').strip())
        body=FRAME.split(b'*')[0].split(b','); body[6]=b'9'
        body=b','.join(body)
        with self.assertRaises(ValueError): m.decode(body+b'*'+f'{m.crc(body):04X}'.encode())
    def test_resync(self):
        model=m.Model(); model.feed(FRAME[:30]+FRAME, 1.0)
        self.assertEqual(len(model.rows),1)
        self.assertGreater(model.errors,0)
    def test_duplicate_does_not_refresh(self):
        model=m.Model(); model.feed(FRAME, 1.0); model.feed(FRAME,10.0)
        self.assertEqual(model.received,1.0)
        self.assertTrue(model.stale(10))
    def test_counter_rollback(self):
        model=m.Model(); model.feed(FRAME,1)
        values=FRAME.split(b'*')[0].split(b','); values[1]=b'3'; values[2]=b'200'
        values[5]=values[6]=b'0'; values[11]=values[12]=b'0'
        for i in range(16,26): values[i]=b'0'
        body=b','.join(values)
        model.feed(body+b'*'+f'{m.crc(body):04X}\n'.encode(),2)
        self.assertTrue(model.frozen)
        self.assertEqual(len(model.rows),1)
    def test_serial_and_hangup(self):
        master, slave=pty.openpty()
        link=m.Serial(os.ttyname(slave))
        os.write(master,FRAME)
        time.sleep(.02)
        self.assertEqual(link.read(),FRAME)
        os.close(master)
        with self.assertRaises(OSError): link.read()
        link.close(); os.close(slave)
    def test_replay_rejects_fifo(self):
        with tempfile.TemporaryDirectory() as tmp:
            path=Path(tmp)/'pipe'; os.mkfifo(path)
            with self.assertRaises(ValueError): m.read_replay(path)
    def test_gui_live_export_disconnect(self):
        import tkinter as tk
        with tempfile.TemporaryDirectory() as tmp:
            root=tk.Tk(); ui=m.App(root)
            master, slave=pty.openpty()
            ui.path.set(os.ttyname(slave)); ui.connect()
            os.write(master,FRAME)
            for _ in range(15): root.update(); time.sleep(.02)
            self.assertIn('Accepted: 1',ui.summary.get())
            self.assertEqual(len(ui.history.get_children()),1)
            target=Path(tmp)/'result.json'; ui.save(target)
            self.assertIn('jayis1',target.read_text())
            with self.assertRaises(FileExistsError): ui.save(target)
            os.close(master)
            for _ in range(10): root.update(); time.sleep(.02)
            self.assertIsNone(ui.link)
            self.assertEqual(ui.model.source,'live')
            ui.close(); os.close(slave)
if __name__=='__main__': unittest.main()
