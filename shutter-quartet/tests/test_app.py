# Author: jayis1. SPDX-License-Identifier: MIT
import binascii
import csv
import io
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'app'))
from model import FIELDS, Session, validate
from transport import Decoder, Serial, load_replay
ROOT=Path(__file__).resolve().parents[1]
def fixture():
    return subprocess.check_output([ROOT/'firmware/build/test-engine','--fixture'])
def encode(row):
    body=('SQ1,'+','.join(str(row[k]) for k in FIELDS)).encode()
    return body+b'*'+f'{binascii.crc_hqx(body,65535):04X}\n'.encode()
class TestApp(unittest.TestCase):
    def test_actual_encoder(self):
        rows=Decoder().feed(fixture(),True)
        self.assertEqual(len(rows),4)
        s=Session('replay')
        for row in rows: s.accept(row)
        self.assertEqual(s.decision(0),'All four points: width spread 0 ms; no qualified re-openings')
    def test_crc_and_impossible(self):
        row=Decoder().feed(fixture(),True)[0]
        for field,value in [('lit0',41),('seen',0),('starts',0),('qualify',9),('pulses0',2),('first0',1)]:
            r=dict(row); r[field]=value
            with self.assertRaises(ValueError): Decoder().feed(encode(r),True)
        data=bytearray(encode(row)); data[10]^=1
        with self.assertRaises(ValueError): Decoder().feed(bytes(data),True)
    def test_fragments_and_tail(self):
        data=fixture(); d=Decoder()
        self.assertEqual(len(d.feed(b'SQ1,123,'+data)),4)
        self.assertEqual(d.errors,1)
        d.feed(b'SQ1,')
        with self.assertRaises(ValueError): d.finish()
    def test_session_guards(self):
        row=Decoder().feed(fixture(),True)[0]
        s=Session('live'); s.connected=True; s.accept(row,10)
        self.assertEqual(s.freshness(10),'WAITING')
        r=dict(row); r['time']+=100; s.accept(r,10.1)
        self.assertEqual(s.freshness(10.1),'LIVE')
        s.accept(r,10.8)
        self.assertEqual(s.freshness(11.2),'STALE')
        r=dict(r); r['time']+=100
        with self.assertRaises(ValueError): s.accept(r,11.3)
        self.assertTrue(s.blocked)
    def test_same_time_mutation_and_rewind(self):
        row=Decoder().feed(fixture(),True)[0]
        for key,value in [('raw',1),('time',0),('completed',0)]:
            s=Session('replay'); s.accept(row)
            r=dict(row); r[key]=value
            with self.assertRaises(ValueError): s.accept(r)
    def test_completed_evidence_frozen(self):
        row=Decoder().feed(fixture(),True)[0]
        s=Session('replay'); s.accept(row)
        r=dict(row); r['time']+=100; r['last0']=39; r['lit0']=39
        validate(r)
        with self.assertRaises(ValueError): s.accept(r)
    def test_replay_bounds_and_export(self):
        with tempfile.TemporaryDirectory() as temp:
            p=Path(temp)/'record'; p.write_bytes(fixture())
            s=load_replay(p); s.errors=2
            out=Path(temp)/'evidence.csv'; s.export(out,0)
            rows=list(csv.reader(io.StringIO(out.read_text())))
            self.assertIn('jayis1',rows[0]); self.assertIn('replay',rows[0])
            self.assertEqual(out.stat().st_mode & 0o777,0o600)
            with self.assertRaises(FileExistsError): s.export(out,0)
            p.write_bytes(fixture()+b'SQ1,')
            with self.assertRaises(ValueError): load_replay(p)
            p.write_bytes(b'')
            with self.assertRaises(ValueError): load_replay(p)
            fifo=Path(temp)/'fifo'; os.mkfifo(fifo)
            with self.assertRaises(ValueError): load_replay(fifo)
    def test_serial_pty_and_restore(self):
        import pty,termios
        master,slave=pty.openpty(); path=os.ttyname(slave)
        before=termios.tcgetattr(slave)
        s=Serial(path)
        os.write(master,fixture())
        import time
        time.sleep(.03)
        self.assertEqual(len(Decoder().feed(s.read(),True)),4)
        s.close(); self.assertEqual(termios.tcgetattr(slave),before)
        os.close(master); os.close(slave)
    def test_empty_export_preserves_error(self):
        with tempfile.TemporaryDirectory() as temp:
            s=Session('live'); s.errors=3; s.blocked='bad frame'
            p=Path(temp)/'out'; s.export(p,0)
            self.assertIn('bad frame',p.read_text())
    def test_forward_jump(self):
        row=Decoder().feed(fixture(),True)[0]
        s=Session('live'); s.connected=True; s.accept(row,10)
        r=dict(row); r['time']+=1000
        with self.assertRaises(ValueError): s.accept(r,10.1)
if __name__=='__main__': unittest.main()
