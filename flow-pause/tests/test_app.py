# Author: jayis1. SPDX-License-Identifier: MIT
import os
from pathlib import Path
import sys
import subprocess
import tempfile
import time
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'app'))
from protocol import Parser, Session, FIELDS
from workbench import Serial, replay, export, App
ROOT = Path(__file__).resolve().parents[1]

def fixture():
    return subprocess.check_output([str(ROOT/'firmware/build/test-engine'), 'fixture'])

def frame(values):
    import binascii
    body = ('FP1,' + ','.join(map(str, values))).encode()
    return body + f'*{binascii.crc_hqx(body, 65535):04X}\n'.encode()

class Tests(unittest.TestCase):
    def test_encoder_parser_and_eof(self):
        data = fixture(); p = Parser(); rows = []
        for byte in data: rows += p.feed(bytes([byte]))
        p.finish(); self.assertEqual(len(rows),1); self.assertEqual(p.errors,0)
        self.assertEqual(rows[0][FIELDS.index('assisted')],1)
        p.feed(data[:-1]); p.finish(); self.assertEqual(p.errors,1)
        p = Parser(); self.assertEqual(len(p.feed(b'old fragment'+data)),1)
        self.assertEqual(p.discarded,12)
        p = Parser(); p.feed(data.replace(b'FP1',b'FP2')); p.finish(); self.assertGreater(p.errors,0)

    def test_impossible_and_monotonic(self):
        f = Parser().feed(fixture())[0]
        bad = list(f); bad[FIELDS.index('assisted')] = 100
        p = Parser(); self.assertEqual(p.feed(frame(bad)),[]); self.assertEqual(p.errors,1)
        s = Session('live'); s.accept(f,0); self.assertFalse(s.fresh(0,True))
        later = list(f); later[0] += 100; s.accept(tuple(later),.1)
        self.assertTrue(s.fresh(.1,True)); self.assertFalse(s.fresh(.1,False))
        s.accept(tuple(later),.2); self.assertFalse(s.fresh(1.11,True))
        impossible = list(later); impossible[0] += 100; impossible[13] = 0
        s.accept(tuple(impossible),.3); self.assertTrue(s.blocked)
        s = Session('live'); s.accept(f,0); later = list(f); later[0] += 100
        s.accept(tuple(later),3); self.assertTrue(s.blocked)
        s = Session('live'); s.accept(f,0)
        for i in range(1,100):
            later = list(f); later[0] += i*10; s.accept(tuple(later),i*.1)
        self.assertTrue(s.blocked)

    def test_replay_export_bounds(self):
        with tempfile.TemporaryDirectory() as d:
            path = Path(d)/'capture'; path.write_bytes(fixture())
            s,p = replay(path); self.assertEqual(s.source,'replay')
            self.assertFalse(s.fresh(time.monotonic(),True))
            output=Path(d)/'e.csv'; export(output,s,p,False)
            self.assertIn('jayis1',output.read_text()); self.assertEqual(output.stat().st_mode & 0o777,0o600)
            with self.assertRaises(FileExistsError): export(output,s,p,False)
            path.write_bytes(fixture()+b'FP1,2')
            with self.assertRaises(ValueError): replay(path)
            path.write_bytes(b'')
            with self.assertRaises(ValueError): replay(path)
            fifo=Path(d)/'fifo'; os.mkfifo(fifo)
            with self.assertRaises(ValueError): replay(fifo)
            path.write_bytes(b'x'*1048577)
            with self.assertRaises(ValueError): replay(path)

    def test_real_gui_pty(self):
        import tkinter as tk
        import termios
        master,slave = os.openpty(); before=termios.tcgetattr(slave)
        root=tk.Tk(); app=App(root); app.port.set(os.ttyname(slave)); app.connect()
        first=app.serial; app.connect(); self.assertIs(app.serial,first)
        f=Parser().feed(fixture())[0]
        os.write(master,fixture())
        def pump():
            for _ in range(10): root.update(); time.sleep(.01)
        pump(); later=list(f); later[0]+=100; os.write(master,frame(later)); pump()
        self.assertEqual(len(app.history.get_children()),2)
        self.assertIn('LIVE',app.status.get()); self.assertEqual(len(app.table.get_children()),len(FIELDS))
        for tab in app.tabs.tabs(): app.tabs.select(tab); root.update()
        os.close(master); pump(); self.assertIsNone(app.serial)
        self.assertIn('DISCONNECTED',app.status.get()); app.close(); os.close(slave)
        master,slave=os.openpty(); before=termios.tcgetattr(slave)
        serial=Serial(os.ttyname(slave)); serial.close()
        self.assertEqual(termios.tcgetattr(slave),before)
        os.close(master); os.close(slave)

if __name__ == '__main__': unittest.main()
