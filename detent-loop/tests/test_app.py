# Author: jayis1. SPDX-License-Identifier: MIT
import importlib.util
from pathlib import Path
import os
import pty
import tempfile
import time
import tkinter as tk
import unittest
from unittest.mock import patch
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('detent',ROOT/'app/detent.py')
assert spec is not None and spec.loader is not None
app=importlib.util.module_from_spec(spec)
spec.loader.exec_module(app)

class Tests(unittest.TestCase):
    def setUp(self):
        self.frame=(ROOT/'firmware/build/fixture.txt').read_bytes().splitlines(keepends=True)[-1]
    def test_encoder_parser(self):
        row=app.parse(self.frame)
        self.assertEqual(row['state'],5)
        self.assertGreater(row['out_uj'],0)
        self.assertGreater(row['back_uj'],0)
        for bad in [self.frame[:-1],b'garbage\n',self.frame.replace(b'D1',b'D2')]:
            with self.assertRaises(ValueError): app.parse(bad)
    def test_regular_replay(self):
        rows=app.load_replay(ROOT/'firmware/build/fixture.txt')
        self.assertEqual(rows[-1]['state'],5)
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'bad';p.write_bytes(self.frame+b'D1,')
            with self.assertRaises(ValueError):app.load_replay(p)
            fifo=Path(d)/'fifo';os.mkfifo(fifo)
            with self.assertRaises(ValueError):app.load_replay(fifo)
    def test_gui_serial_export(self):
        root=tk.Tk(); a=app.App(root)
        master,slave=pty.openpty()
        try:
            a.port.set(os.ttyname(slave)); a.connect()
            os.write(master,self.frame)
            for _ in range(10): root.update();time.sleep(.04)
            self.assertEqual(len(a.rows),1)
            self.assertIn('DONE',a.live.get())
            self.assertIn('mJ',a.result.get())
            self.assertEqual(len(a.tabs.tabs()),3)
            self.assertGreater(len(a.canvas.find_all()),0)
            with tempfile.TemporaryDirectory() as d:
                p=Path(d)/'record.csv'; a.export_to(p)
                self.assertIn('jayis1',p.read_text())
                self.assertEqual(p.stat().st_mode&0o777,0o600)
                with self.assertRaises(FileExistsError):a.export_to(p)
            # Identical duplicates must not refresh freshness.
            age=a.last_host;a.accept(app.parse(self.frame))
            self.assertEqual(a.last_host,age)
            changed=app.parse(self.frame);changed['x_um']+=1
            with self.assertRaises(ValueError):a.accept(changed)
            os.close(master);master=None
            for _ in range(5):root.update();time.sleep(.04)
            self.assertIsNone(a.fd)
            self.assertEqual(a.provenance,'live')
        finally:
            a.close()
            if master is not None:os.close(master)
            os.close(slave)
    def test_failed_connect_retains(self):
        root=tk.Tk();a=app.App(root)
        try:
            a.provenance='replay';a.accept(app.parse(self.frame))
            a.port.set('/this/path/does/not/exist');a.connect()
            self.assertEqual(len(a.rows),1)
            self.assertEqual(a.provenance,'replay')
        finally:a.close()

    def test_full_c_recording_through_gui(self):
        root=tk.Tk();a=app.App(root)
        try:
            a.provenance='replay'
            rows=app.load_replay(ROOT/'firmware/build/fixture.txt')
            for r in rows:a.accept(r)
            self.assertEqual(len(a.rows),len(rows))
            self.assertEqual(a.rows[-1]['state'],5)
            self.assertGreater(len(a.canvas.find_all()),20)
            old=a.rows[-1];bad=dict(old,sequence=old['sequence']+1,now=old['now']+200,x_um=old['x_um']+1)
            with self.assertRaises(ValueError):a.accept(bad)
        finally:a.close()

    def test_semantic_invalid_crc_valid(self):
        r=app.parse(self.frame)
        for change in [dict(state=0),dict(faults=1),dict(span_um=0),dict(x_um=1000),dict(peak_back_mn=1)]:
            bad=dict(r,**change)
            body=('D1,'+','.join(str(bad[k]) for k in app.FIELDS)).encode()
            packet=body+f',{app.binascii.crc_hqx(body,65535):04X}\n'.encode()
            with self.assertRaises(ValueError):app.parse(packet)

    def test_long_pause_rejects_queued_progress(self):
        root=tk.Tk();a=app.App(root)
        try:
            a.provenance='live';a.accept(app.parse(self.frame))
            a.last_host-=3
            old=a.rows[-1]
            with self.assertRaises(ValueError):
                a.accept(dict(old,sequence=old['sequence']+1,now=old['now']+200))
        finally:a.close()

    def test_empty_export_and_setup_failure_restore(self):
        root=tk.Tk();a=app.App(root)
        master,slave=pty.openpty()
        try:
            original=app.termios.tcgetattr(slave)
            a.port.set(os.ttyname(slave))
            with patch.object(app.termios,'tcflush',side_effect=OSError('injected flush failure')):
                a.connect()
            self.assertIsNone(a.fd)
            self.assertEqual(app.termios.tcgetattr(slave),original)
            with tempfile.TemporaryDirectory() as d:
                p=Path(d)/'empty.csv';a.export_to(p)
                self.assertIn('metadata',p.read_text())
                self.assertIn('none',p.read_text())
        finally:a.close();os.close(master);os.close(slave)

if __name__=='__main__':unittest.main()
