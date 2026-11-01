# Author: jayis1. SPDX-License-Identifier: MIT
import binascii
import os
from pathlib import Path
import sys
import tempfile
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'app'))
from model import Session, FIELDS, validate, describe
from transport import Decoder, Serial, load_replay

def frame(row):
    body=('TC1,'+','.join(str(row[k]) for k in FIELDS)).encode()
    return body+b'*'+f'{binascii.crc_hqx(body,65535):04X}\n'.encode()

def blank():
    r=dict.fromkeys(FIELDS,0); r.update(state=1,valid=15); return r

class ModelTests(unittest.TestCase):
    def test_real_c_recording(self):
        path=Path(__file__).parent/'../firmware/build/demo.tc1'
        s=load_replay(path)
        r=s.rows[-1]
        self.assertEqual(r['preferences'],3); self.assertEqual(r['cycle'],1)
        self.assertIn('CYCLE',describe(r)); self.assertFalse(s.fresh())
        with tempfile.TemporaryDirectory() as d:
            dest=Path(d)/'evidence.csv'; s.export(dest,'=example')
            self.assertEqual(dest.stat().st_mode&0o777,0o600)
            self.assertIn('jayis1',dest.read_text())
            with self.assertRaises(FileExistsError): s.export(dest,'again')
    def test_invalid_crc_and_tail(self):
        d=Decoder(); self.assertEqual(d.feed(b'TC1,0*0000\n'),[])
        self.assertEqual(d.errors,1)
        self.assertEqual(len(d.feed(b'TC1,garbage'+frame(blank()))),1)
        d.feed(b'TC1,'); self.assertRaises(ValueError,d.finish)
    def test_semantic_validation(self):
        for updates in [dict(graph=1,preferences=1,references=1,starts=2),
                        dict(cycle=1),dict(anchored=1),dict(state=2,outcome=12,starts=1),
                        dict(preferences=1,references=1,starts=2)]:
            r=blank(); r.update(updates)
            with self.assertRaises(ValueError): validate(r)
    def test_lost_and_unfunded_edges(self):
        r=blank(); r.update(graph=2,preferences=1,references=1,starts=2)
        for updates in [dict(graph=4),dict(graph=6),dict(graph=0)]:
            s=Session('replay'); s.accept(r)
            new=dict(r,now=100,**updates)
            with self.assertRaises(ValueError): s.accept(new)
    def test_wrong_active_completion(self):
        r=blank(); r.update(state=2,starts=2,references=1,anchor=1,anchored=1,selected=0,outcome=8)
        s=Session('replay'); s.accept(r)
        bad=dict(r,now=10,state=1,preferences=1,graph=4,anchored=0)
        with self.assertRaises(ValueError): s.accept(bad)
    def test_freshness_and_pause(self):
        r=blank(); r.update(references=2,starts=2,anchored=1,anchor=1)
        s=Session('replay'); s.accept(r)
        with self.assertRaises(ValueError):
            s.accept(dict(r,now=100,preferences=1,starts=3,graph=2))
        s=Session('live'); r=blank(); s.accept(r,0)
        s.accept(dict(r,now=100),0.1); self.assertTrue(s.fresh(0.2))
        s.accept(dict(r,now=100),1); self.assertFalse(s.fresh(3))
        with self.assertRaises(ValueError): s.accept(dict(r,now=200),3)
    def test_serial_pty(self):
        import pty,termios,time
        master,slave=pty.openpty(); saved=termios.tcgetattr(slave)
        serial=Serial(os.ttyname(slave))
        os.write(master,frame(blank())); time.sleep(.01)
        self.assertEqual(len(Decoder().feed(serial.read())),1)
        serial.close(); self.assertEqual(termios.tcgetattr(slave),saved)
        os.close(master); os.close(slave)
    def test_replay_boundaries(self):
        with tempfile.TemporaryDirectory() as d:
            fifo=Path(d)/'fifo'; os.mkfifo(fifo)
            self.assertRaises(ValueError,load_replay,fifo)
            empty=Path(d)/'empty'; empty.write_bytes(b'')
            self.assertRaises(ValueError,load_replay,empty)
            empty.write_bytes(frame(blank())+b'TC1,')
            self.assertRaises(ValueError,load_replay,empty)

if __name__=='__main__': unittest.main()
