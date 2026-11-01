# Author: jayis1. SPDX-License-Identifier: MIT
import binascii
import os
from pathlib import Path
import sys
import tempfile
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'app'))
from model import FIELDS, Session, validate
from transport import Decoder, Serial, load_replay

def row(**kw):
    r=dict.fromkeys(FIELDS,0); r.update(state=1,valid=15); r.update(kw); return r

def frame(r):
    body=('DW1,'+','.join(str(r[k]) for k in FIELDS)).encode()
    return body+('*%04X\n'%binascii.crc_hqx(body,65535)).encode()

class Tests(unittest.TestCase):
    def test_codec_and_fragment_recovery(self):
        d=Decoder(); r=row(); f=frame(r)
        self.assertEqual(d.feed(f[:8]),[])
        self.assertEqual(d.feed(f[8:]),[r]); d.finish()
        self.assertEqual(d.feed(f[:10]+f),[r]); self.assertEqual(d.errors,1)
        self.assertEqual(d.feed(f.replace(b'DW1,0',b'DW1,9')),[])
        self.assertEqual(d.errors,2)
    def test_impossible_snapshots(self):
        for bad in [row(starts=1),row(faults=1),row(faces=3,strokes=1),
                    row(state=2,starts=1),row(starts=1,good=1,last_result=1,last_faces=1,last_strokes=2)]:
            with self.assertRaises(ValueError): validate(bad)
            with self.assertRaises(ValueError): Decoder().feed(frame(bad),strict=True)
    def test_progress(self):
        s=Session('live'); s.accept(row(),host=0); self.assertFalse(s.fresh(0))
        s.accept(row(now=100),host=.1); self.assertTrue(s.fresh(.1))
        s.accept(row(now=100),host=1); self.assertFalse(s.fresh(3))
        with self.assertRaises(ValueError): s.accept(row(now=200),host=4)
        self.assertTrue(s.warning)
    def test_session_contradictions(self):
        for second in [row(now=0,levels=1),row(now=0xffffffff),row(now=100,last_age=1)]:
            s=Session('replay'); s.accept(row())
            with self.assertRaises(ValueError): s.accept(second)
    def test_live_drift(self):
        s=Session('live'); s.accept(row(),host=0)
        with self.assertRaises(ValueError):
            for i in range(1,40): s.accept(row(now=i*10),host=i*.1)
    def test_export_and_replay(self):
        with tempfile.TemporaryDirectory() as tmp:
            p=Path(tmp)/'capture'; p.write_bytes(frame(row()))
            s=load_replay(p); self.assertFalse(s.fresh())
            out=Path(tmp)/'out.csv'; s.export(out,'=1+1')
            self.assertEqual(out.stat().st_mode&0o777,0o600)
            self.assertIn('jayis1',out.read_text())
            self.assertIn("'=1+1",out.read_text())
            with self.assertRaises(FileExistsError): s.export(out,'x')
            p.write_bytes(frame(row())+b'DW1,')
            with self.assertRaises(ValueError): load_replay(p)
            fifo=Path(tmp)/'fifo'; os.mkfifo(fifo)
            with self.assertRaises(ValueError): load_replay(fifo)
    def test_serial(self):
        import pty, termios
        master,slave=pty.openpty(); saved=termios.tcgetattr(slave)
        try:
            serial=Serial(os.ttyname(slave)); os.write(master,frame(row()))
            import select
            select.select([serial.fd],[],[],1)
            self.assertEqual(Decoder().feed(serial.read()),[row()])
            serial.close(); self.assertEqual(saved,termios.tcgetattr(slave))
        finally: os.close(master); os.close(slave)
        with self.assertRaises((OSError,termios.error)): Serial('/dev/null')

if __name__=='__main__': unittest.main()
