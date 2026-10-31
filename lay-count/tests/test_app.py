# Author: jayis1. SPDX-License-Identifier: MIT
import binascii
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
    return subprocess.check_output([str(ROOT/'firmware/build/test-engine'),'--fixture'])
def encode(row):
    body=('LC1,'+','.join(str(row[k]) for k in FIELDS)).encode()
    return body+f'*{binascii.crc_hqx(body,65535):04X}\n'.encode()
class ModelTests(unittest.TestCase):
    def test_real_encoder(self):
        rows=Decoder().feed(fixture(),True)
        self.assertEqual([r['last_pulses'] for r in rows],[1,2,3,4])
        s=Session('replay')
        for r in rows:s.accept(r)
        self.assertEqual(s.rows[-1]['completed'],4)
    def test_corruption_resync_and_tail(self):
        data=fixture(); d=Decoder()
        self.assertEqual(len(d.feed(b'LC1,99'+data)),4)
        self.assertEqual(d.errors,1)
        with self.assertRaises(ValueError):Decoder().feed(data.replace(b'LC1',b'LC2',1),True)
        d=Decoder();d.feed(data+b'LC1,')
        with self.assertRaises(ValueError):d.finish()
    def test_impossible_valid_crc(self):
        row=Decoder().feed(fixture())[0]
        for key,value in [('last_pulses',2),('starts',9),('bad_total',2),('last_end',0)]:
            bad=dict(row);bad[key]=value
            with self.subTest(key=key),self.assertRaises(ValueError):Decoder().feed(encode(bad),True)
    def test_duplicate_reset_and_stale(self):
        rows=Decoder().feed(fixture());s=Session('live');s.connected=True
        s.accept(rows[0],10);self.assertFalse(s.fresh(10))
        s.accept(rows[1],10.1);self.assertTrue(s.fresh(10.1))
        s.accept(rows[1],10.9);self.assertFalse(s.fresh(11.2))
        bad=dict(rows[1]);bad['time']+=100;bad['tx']=1
        s.accept(bad,10.2)
        bad=dict(bad);bad['time']+=100;bad['tx']=0
        with self.assertRaises(ValueError):s.accept(bad,10.3)
        self.assertTrue(s.blocked)
    def test_replay_and_export(self):
        with tempfile.TemporaryDirectory() as td:
            p=Path(td)/'capture';p.write_bytes(fixture());s=load_replay(p)
            out=Path(td)/'export.csv';s.export(out)
            self.assertIn('jayis1',out.read_text());self.assertIn('replay',out.read_text())
            self.assertEqual(out.stat().st_mode&0o777,0o600)
            with self.assertRaises(FileExistsError):s.export(out)
            p.write_bytes(fixture()+b'LC1,')
            with self.assertRaises(ValueError):load_replay(p)
            fifo=Path(td)/'fifo';os.mkfifo(fifo)
            with self.assertRaises(ValueError):load_replay(fifo)
    def test_aggregate_changes_need_outcomes(self):
        row=Decoder().feed(fixture())[1]
        for bucket in ('completed_pulses','abandoned'):
            s=Session('replay');s.accept(row)
            bad=dict(row);bad['time']+=100;bad['pulses']+=1;bad[bucket]+=1
            validate(bad)
            with self.assertRaises(ValueError):s.accept(bad)
    def test_host_pause_and_sustained_backlog(self):
        row=Decoder().feed(fixture())[0]
        s=Session('live');s.accept(row,0)
        bad=dict(row);bad['time']+=100
        with self.assertRaises(ValueError):s.accept(bad,2)
        s=Session('live');s.accept(row,0)
        with self.assertRaises(ValueError):
            for i in range(1,100):
                bad=dict(row);bad['time']+=i*100;s.accept(bad,i*.12)
    def test_serial_restore_and_hangup(self):
        import termios
        master,slave=os.openpty(); before=termios.tcgetattr(slave)
        port=Serial(os.ttyname(slave));os.write(master,b'hello')
        import time;time.sleep(.02)
        self.assertEqual(port.read(),b'hello');port.close()
        self.assertEqual(before,termios.tcgetattr(slave))
        port=Serial(os.ttyname(slave));os.close(master)
        with self.assertRaises(OSError):port.read()
        port.close();os.close(slave)
if __name__=='__main__':unittest.main()
