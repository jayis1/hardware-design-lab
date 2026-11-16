# Author: jayis1. SPDX-License-Identifier: MIT
import binascii
import os
from pathlib import Path
import pty
import sys
import tempfile
import termios
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'app'))
from model import Session, FIELDS, validate
from transport import Decoder, Serial, load_replay
ROOT=Path(__file__).resolve().parents[1]
def frame(row):
    body=('CD1,'+','.join(str(row[x]) for x in FIELDS)).encode()
    return body+('*%04X\n'%binascii.crc_hqx(body,65535)).encode()
class Tests(unittest.TestCase):
    def setUp(self):
        self.data=(ROOT/'firmware/build/demo.cd1').read_bytes()
        self.rows=Decoder().feed(self.data,strict=True)
    def test_real_fixture(self):
        s=load_replay(ROOT/'firmware/build/demo.cd1')
        self.assertEqual(s.rows[-1]['completed'],3)
        done=[r for r in s.rows if r['completed']==1]
        self.assertEqual(done[0]['last_returned'],1)
        self.assertEqual(s.rows[-1]['phase'],3)
    def test_all_chunks(self):
        d=Decoder(); result=[]
        for byte in self.data: result+=d.feed(bytes([byte]),strict=True)
        d.finish(); self.assertEqual(result,self.rows)
    def test_crc_and_resync(self):
        d=Decoder(); good=frame(self.rows[0]); bad=bytearray(good); bad[5]^=1
        self.assertFalse(d.feed(bytes(bad)))
        self.assertEqual(d.feed(good[:10]+good),[self.rows[0]])
        self.assertGreaterEqual(d.errors,2)
    def test_impossible_last(self):
        r=next(r.copy() for r in self.rows if r['completed']==1)
        r['last_returned']=2
        r['last_touched']=1
        with self.assertRaises(ValueError): Decoder().feed(frame(r),strict=True)
    def test_frozen_record(self):
        r=next(r.copy() for r in self.rows if r['completed']==1)
        s=Session('replay'); s.accept(r)
        bad=r.copy(); bad['now']+=1; bad['sequence']+=1
        bad['last_age']+=1
        with self.assertRaises(ValueError): s.accept(bad)
    def test_rollback_and_conflicting_duplicate(self):
        r=self.rows[2]
        for key in ['now','sequence','starts']:
            if not r[key]: continue
            s=Session('replay'); s.accept(r)
            bad=r.copy(); bad[key]-=1
            with self.assertRaises(ValueError): s.accept(bad)
    def test_replay_limits(self):
        with tempfile.TemporaryDirectory() as t:
            p=Path(t)/'bad'
            for content in [b'',self.data+b'CD1,12']:
                p.write_bytes(content)
                with self.assertRaises(ValueError): load_replay(p)
            p.unlink(); os.mkfifo(p)
            with self.assertRaises(ValueError): load_replay(p)
    def test_serial_restore_hangup(self):
        master,slave=pty.openpty()
        old=termios.tcgetattr(slave)
        serial=Serial(os.ttyname(slave)); serial.close()
        self.assertEqual(termios.tcgetattr(slave),old)
        serial=Serial(os.ttyname(slave)); os.close(master)
        with self.assertRaises(OSError): serial.read()
        serial.close(); os.close(slave)
    def test_serial_wrong_path(self):
        with self.assertRaises((OSError,termios.error)): Serial('/dev/null')
    def test_freshness_and_export(self):
        s=Session('live'); r=self.rows[0].copy(); s.accept(r,10)
        self.assertFalse(s.fresh(10))
        r=self.rows[1].copy(); s.accept(r,10.1)
        self.assertTrue(s.fresh(10.1))
        self.assertFalse(s.fresh(13))
        self.assertTrue(s.blocked)
        with tempfile.TemporaryDirectory() as t:
            p=Path(t)/'e.csv'; s.export(p,13)
            text=p.read_text(); self.assertIn('jayis1',text); self.assertIn('live',text)
            self.assertEqual(p.stat().st_mode&0o777,0o600)
            with self.assertRaises(FileExistsError): s.export(p,13)
    def test_stopped_sequence_is_not_live(self):
        s=Session('live')
        first=self.rows[0].copy(); s.accept(first,10)
        row=self.rows[1].copy(); s.accept(row,10.1)
        self.assertTrue(s.fresh(10.1))
        # Valid CRC/schema alone must not let timestamp-only traffic renew
        # acquisition freshness after the initial sequence advance.
        for step in range(1,22):
            frozen=row.copy(); frozen['now']+=step*100
            s.accept(frozen,10.1+step/10)
        self.assertFalse(s.fresh(12.2))
        self.assertTrue(s.blocked)
if __name__=='__main__': unittest.main()
