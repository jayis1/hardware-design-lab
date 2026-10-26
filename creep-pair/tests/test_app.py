# Author: jayis1. SPDX-License-Identifier: MIT
import binascii
import importlib.util
from pathlib import Path
import tempfile
import unittest
import os
import pty
import tkinter as tk
import time
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('creep',ROOT/'app/creep.py')
assert spec and spec.loader
m=importlib.util.module_from_spec(spec); spec.loader.exec_module(m)
FIXTURE=(ROOT/'firmware/build/fixture.txt').read_bytes()
def encode(parts):
    body=','.join(parts).encode(); return body+b','+f'{binascii.crc_hqx(body,65535):04X}'.encode()+b'\n'
class Tests(unittest.TestCase):
    def test_encoder_fixture(self):
        r=m.parse(FIXTURE); self.assertEqual(r['lanes'][0][0],4)
        self.assertEqual(r['lanes'][1][0],1)
    def test_malformed(self):
        for data in [b'',FIXTURE[:-3],FIXTURE.replace(b'C1',b'C2'),b'x'*1024]:
            with self.assertRaises(ValueError): m.parse(data)
        p=FIXTURE.decode().strip().split(',')[:-1]; p[9]='8'
        with self.assertRaises(ValueError): m.parse(encode(p))
        p=FIXTURE.decode().strip().split(',')[:-1]; p[16]='0'
        with self.assertRaises(ValueError): m.parse(encode(p))
    def test_replay_bounds(self):
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'replay'; p.write_bytes(FIXTURE)
            self.assertEqual(len(m.load_replay(p)),1)
            p.write_bytes(FIXTURE+b'C1,')
            with self.assertRaises(ValueError): m.load_replay(p)
            p.unlink(); os.mkfifo(p)
            with self.assertRaises(ValueError): m.load_replay(p)
    def test_gui_serial_export(self):
        root=tk.Tk(); app=m.App(root)
        master,slave=pty.openpty(); app.port.set(os.ttyname(slave)); app.connect()
        os.write(master,FIXTURE)
        for _ in range(10): root.update(); time.sleep(.03)
        self.assertEqual(len(app.rows),1)
        self.assertIn('DONE',app.live.get())
        for tab in app.tabs.tabs(): app.tabs.select(tab); root.update()
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'out.csv'; app.export_to(p)
            text=p.read_text(); self.assertIn('jayis1',text); self.assertIn('live',text)
            with self.assertRaises(FileExistsError): app.export_to(p)
        os.close(master)
        for _ in range(10): root.update(); time.sleep(.03)
        self.assertIsNone(app.fd)
        self.assertIn('disconnected',app.status.get())
        os.close(slave); root.destroy()
    def test_failed_connect_preserves_evidence(self):
        root=tk.Tk(); app=m.App(root); app.accept(m.parse(FIXTURE))
        app.port.set('/no/such/device'); app.connect()
        self.assertEqual(len(app.rows),1)
        root.destroy()
    def test_baseline_immutable(self):
        root=tk.Tk(); app=m.App(root); app.accept(m.parse(FIXTURE))
        p=FIXTURE.decode().strip().split(',')[:-1]
        p[1]='2'; p[2]=str(int(p[2])+200); p[11]=str(int(p[11])+100)
        with self.assertRaises(ValueError): app.accept(m.parse(encode(p)))
        root.destroy()
    def test_duplicate_and_reset(self):
        root=tk.Tk(); app=m.App(root)
        app.accept(m.parse(FIXTURE)); app.accept(m.parse(FIXTURE))
        self.assertEqual(len(app.rows),1)
        p=FIXTURE.decode().strip().split(',')[:-1]; p[2]=str(int(p[2])+1)
        with self.assertRaises(ValueError): app.accept(m.parse(encode(p)))
        root.destroy()
    def test_stale_and_backlog(self):
        root=tk.Tk(); app=m.App(root); app.provenance='live'; app.accept(m.parse(FIXTURE))
        p=FIXTURE.decode().strip().split(',')[:-1]; p[1]='2'; p[2]=str(int(p[2])+200)
        app.last_host-=3
        with self.assertRaises(ValueError): app.accept(m.parse(encode(p)))
        app.last_host=time.monotonic(); app.first_host-=4
        with self.assertRaises(ValueError): app.accept(m.parse(encode(p)))
        root.destroy()
    def test_invalid_hold_and_empty_export(self):
        p=FIXTURE.decode().strip().split(',')[:-1]; p[12]='180020'
        with self.assertRaises(ValueError): m.parse(encode(p))
        root=tk.Tk(); app=m.App(root)
        with tempfile.TemporaryDirectory() as d:
            target=Path(d)/'empty.csv'; app.export_to(target)
            self.assertIn('provenance',target.read_text())
            self.assertEqual(target.stat().st_mode & 0o777,0o600)
        root.destroy()
if __name__=='__main__': unittest.main()
