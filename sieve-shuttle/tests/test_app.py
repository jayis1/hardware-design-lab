# Author: jayis1. SPDX-License-Identifier: MIT
"""Model tests use fixtures emitted by the real C encoder, never hardware data."""
import binascii
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'app'))
from model import Session, validate, FIELDS
from transport import Decoder, load_replay, Serial
ROOT=Path(__file__).resolve().parents[1]
def fixture():
    return subprocess.check_output([str(ROOT/'firmware/build/test-engine'),'--fixture'])
def frame(row):
    body=('SS1,'+','.join(str(row[k]) for k in FIELDS)).encode()
    return body+b'*'+f'{binascii.crc_hqx(body,65535):04X}'.encode()+b'\n'
class Tests(unittest.TestCase):
    def test_c_fixture(self):
        rows=Decoder().feed(fixture(),True)
        self.assertEqual(len(rows),9)
        s=Session('replay')
        for row in rows: s.accept(row)
        self.assertEqual((rows[-1]['visits'],rows[-1]['x'],rows[-1]['y'],rows[-1]['turns']),(8,2,2,3))
        self.assertFalse(s.fresh())
    def test_framing(self):
        data=fixture(); d=Decoder(); rows=[]
        for b in data: rows+=d.feed(bytes([b]))
        d.finish(); self.assertEqual(len(rows),9)
        d=Decoder(); self.assertEqual(len(d.feed(data[:25]+data)),9)
        self.assertGreater(d.errors,0)
        with self.assertRaises(ValueError): Decoder().feed(data.replace(b'SS1,',b'SS2,',1),True)
        d=Decoder(); d.feed(data[:-1])
        with self.assertRaises(ValueError): d.finish()
    def test_impossible(self):
        r=Decoder().feed(fixture())[-1]
        for key,value in [('x',100),('m0',1),('run',100),('peak_x',0),('fault',1),('last',4)]:
            bad=dict(r); bad[key]=value
            with self.assertRaises(ValueError,msg=key): Decoder().feed(frame(bad),True)
    def test_sessions(self):
        rows=Decoder().feed(fixture())
        s=Session('live'); s.connected=True
        s.accept(rows[0],0); s.accept(rows[1],.14)
        self.assertTrue(s.fresh(.2)); self.assertFalse(s.fresh(2))
        s.accept(rows[1],.9); self.assertFalse(s.fresh(1.2))
        bad=dict(rows[1]); bad['now']+=100
        with self.assertRaises(ValueError): s.accept(bad,3)
        self.assertTrue(s.blocked)
        s=Session('replay'); s.accept(rows[2]);
        with self.assertRaises(ValueError): s.accept(rows[1])
    def test_single_transition(self):
        rows=Decoder().feed(fixture()); s=Session('replay'); s.accept(rows[1])
        bad=dict(rows[2]); bad['last']=0
        with self.assertRaises(ValueError): s.accept(bad)
    def test_pending_commit_guard(self):
        rows=Decoder().feed(fixture())
        for pending,clear in [(0,100),(1,0)]:
            prev=dict(rows[1],state=2,pending=pending,clear_ms=clear,now=rows[2]['now']-1)
            s=Session('replay'); s.accept(prev)
            with self.assertRaises(ValueError): s.accept(rows[2])
    def test_qualification_cannot_disappear(self):
        r=Decoder().feed(fixture())[0]
        bad=dict(r,qualified=0)
        with self.assertRaises(ValueError): Decoder().feed(frame(bad),True)
        s=Session('replay'); s.accept(r)
        bad=dict(r,qualified=0,state=0,now=r['now']+100)
        with self.assertRaises(ValueError): s.accept(bad)
    def test_sparse_peak_bound(self):
        r=Decoder().feed(fixture())[-1]
        bad=dict(r,now=r['now']+300,visits=10,x=3,turns=4,last=1,axis=0,run=1,peak_x=3)
        bad['m12']+=1; bad['m1']+=1
        validate(bad)
        s=Session('replay'); s.accept(r)
        with self.assertRaises(ValueError): s.accept(bad)
    def test_exports_and_replay(self):
        with tempfile.TemporaryDirectory() as tmp:
            p=Path(tmp)/'frames'; p.write_bytes(fixture())
            s=load_replay(p); target=Path(tmp)/'export.json'; s.export(target,'test fixture')
            self.assertEqual(json.loads(target.read_text())['author'],'jayis1')
            self.assertEqual(target.stat().st_mode&0o777,0o600)
            with self.assertRaises(FileExistsError): s.export(target)
            p.write_bytes(b'')
            with self.assertRaises(ValueError): load_replay(p)
            fifo=Path(tmp)/'fifo'; os.mkfifo(fifo)
            with self.assertRaises(ValueError): load_replay(fifo)
    def test_serial(self):
        import pty,termios
        master,slave=pty.openpty(); before=termios.tcgetattr(slave)
        serial=Serial(os.ttyname(slave)); os.write(master,b'example\n')
        import time; time.sleep(.02)
        self.assertEqual(serial.read(),b'example\n')
        serial.close(); self.assertEqual(termios.tcgetattr(slave),before)
        os.close(master); os.close(slave)
if __name__=='__main__': unittest.main()
