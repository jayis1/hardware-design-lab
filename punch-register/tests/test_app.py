# Author: jayis1. SPDX-License-Identifier: MIT
import binascii
import os
from pathlib import Path
import pty
import subprocess
import sys
import tempfile
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'app'))
from protocol import Decoder, Session, Serial, load_replay
ROOT = Path(__file__).resolve().parents[1]
def fixture():
    return subprocess.check_output([str(ROOT/'firmware/build/test-engine'), '--fixture'])
def frame(values):
    body = ('PR1,' + ','.join(map(str, values))).encode()
    return body+b'*'+f'{binascii.crc_hqx(body,65535):04X}'.encode()+b'\n'
class Tests(unittest.TestCase):
    def test_encoder_and_partial(self):
        d=Decoder(); rows=[]
        for b in fixture(): rows += d.feed(bytes([b]))
        d.finish(); self.assertEqual(len(rows),2)
        self.assertEqual(rows[-1]['good'],1)
    def test_crc_and_resync(self):
        d=Decoder(); rows=d.feed(b'PR1,broken'+fixture())
        self.assertEqual(len(rows),2); self.assertEqual(d.errors,1)
        with self.assertRaises(ValueError): Decoder().feed(fixture().replace(b'PR1,',b'PR1,9',1), strict=True)
    def test_tail(self):
        d=Decoder(); d.feed(fixture()+b'PR1,1')
        with self.assertRaises(ValueError): d.finish()
    def test_impossible(self):
        row=fixture().splitlines()[1].split(b'*')[0].split(b',')[1:]
        v=list(map(int,row)); v[7]=99
        with self.assertRaises(ValueError): Decoder().feed(frame(v),strict=True)
    def test_time_and_counters(self):
        rows=Decoder().feed(fixture()); s=Session('replay')
        s.accept(rows[0],0); s.accept(rows[1],.08)
        self.assertEqual(len(s.rows),2)
        s.accept(rows[1],.09); self.assertEqual(len(s.rows),2)
        with self.assertRaises(ValueError): s.accept(rows[0],.1)
        self.assertTrue(s.blocked)
    def test_stale_and_backlog(self):
        row=Decoder().feed(fixture())[0]; s=Session('live'); s.connected=True
        s.accept(row,0); self.assertFalse(s.fresh(.01))
        n=dict(row,now=row['now']+100); s.accept(n,.1)
        self.assertTrue(s.fresh(.2)); self.assertFalse(s.fresh(2))
        with self.assertRaises(ValueError): s.accept(dict(n,now=n['now']+100),5)
    def test_replay_bounds_and_export(self):
        with tempfile.TemporaryDirectory() as directory:
            p=Path(directory)/'input'; p.write_bytes(fixture())
            s=load_replay(p); out=Path(directory)/'export.json'
            s.export(out,'fixture, not hardware')
            with self.assertRaises(FileExistsError): s.export(out,'again')
            self.assertIn('jayis1',out.read_text())
            p.write_bytes(b''); self.assertRaises(ValueError,load_replay,p)
            p.unlink(); os.mkfifo(p)
            self.assertRaises(ValueError,load_replay,p)
    def test_serial_pty(self):
        import termios
        master,slave=pty.openpty(); path=os.ttyname(slave)
        before=termios.tcgetattr(slave)
        port=Serial(path); os.write(master,fixture())
        import time; time.sleep(.02)
        self.assertEqual(len(Decoder().feed(port.read())),2)
        port.close(); self.assertEqual(termios.tcgetattr(slave),before)
        os.close(master); os.close(slave)
    def test_serial_setup_failure_restores(self):
        import termios
        from unittest.mock import patch
        master,slave=pty.openpty()
        try:
            before=termios.tcgetattr(slave)
            with patch('protocol.termios.tcflush',side_effect=OSError('injected setup failure')):
                with self.assertRaises(OSError): Serial(os.ttyname(slave))
            self.assertEqual(termios.tcgetattr(slave),before)
        finally:
            os.close(master); os.close(slave)
    def test_duplicate_payload_rejected(self):
        row=Decoder().feed(fixture())[0]; s=Session('replay'); s.accept(row)
        with self.assertRaises(ValueError): s.accept(dict(row,raw=0))
        self.assertTrue(s.blocked)
if __name__=='__main__': unittest.main()
