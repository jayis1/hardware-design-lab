# Author: jayis1. SPDX-License-Identifier: MIT
import binascii
import copy
import os
from pathlib import Path
import sys
import tempfile
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'app'))
from model import Session, validate, FIELDS
from transport import Decoder, load_replay, Serial
ROOT=Path(__file__).resolve().parents[1]
def encode(r):
    body=('AY1,'+','.join(str(r[k]) for k in FIELDS)).encode()
    return body+b'*'+f'{binascii.crc_hqx(body,65535):04X}\n'.encode()
class ModelTests(unittest.TestCase):
    def setUp(self):
        self.data=(ROOT/'firmware/build/demo.ay1').read_bytes()
        self.rows=Decoder().feed(self.data,strict=True)
    def test_fixture(self):
        s=load_replay(ROOT/'firmware/build/demo.ay1')
        self.assertEqual(len(s.rows),10)
        self.assertEqual(s.rows[-1]['phase'],3)
        self.assertTrue(all(s.rows[-1][f'bin{i}'] for i in range(9)))
        self.assertFalse(s.fresh(True))
    def test_dense_real_encoder_and_sparse_subsequences(self):
        import subprocess
        data=subprocess.check_output([str(ROOT/'firmware/build/fixture'),'--stress'])
        d=Decoder(); rows=[]
        for offset in range(0,len(data),4096):
            rows.extend(d.feed(data[offset:offset+4096],strict=True))
        d.finish(); self.assertGreater(len(rows),2000)
        for stride in (1,7,37):
            s=Session('replay')
            for row in rows[::stride]: s.accept(row)
    def test_crc_shape_and_conservation(self):
        for key,value in [('bin0',999),('unknown',0),('maximum',99999),('valid',4),('phase',2)]:
            r=dict(self.rows[0]); r[key]=value
            with self.assertRaises(ValueError): Decoder().feed(encode(r),strict=True)
        with self.assertRaises(ValueError): Decoder().feed(self.data.replace(b'AY1,100',b'AY1,101',1),strict=True)
    def test_prefix_and_partial_tail(self):
        d=Decoder(); rows=d.feed(b'garbage\n'+self.data)
        self.assertEqual(len(rows),10); self.assertGreater(d.errors,0)
        d=Decoder(); rows=d.feed(self.data[:20]+self.data)
        self.assertEqual(len(rows),10); self.assertEqual(d.errors,1)
        d=Decoder(); d.feed(self.data[:-1])
        with self.assertRaises(ValueError): d.finish()
    def test_monotonic_and_halt(self):
        for key in ['bin0','sequence','unknown']:
            s=Session('replay'); s.accept(self.rows[0])
            bad=dict(self.rows[1]); bad[key]=0
            with self.assertRaises(ValueError): s.accept(bad)
        s=Session('replay'); s.accept(self.rows[-1]); bad=dict(self.rows[-1])
        bad['now']+=100; bad['sequence']+=1; bad['intervals']+=1; bad['unknown']+=1
        with self.assertRaises(ValueError): s.accept(bad)
    def test_no_acquisition_cannot_add_evidence(self):
        s=Session('replay'); old=self.rows[-2]; s.accept(old)
        bad=dict(self.rows[-1]); bad['episodes']+=1
        validate(bad)
        with self.assertRaises(ValueError): s.accept(bad)
    def test_maximum_growth(self):
        # Valid endpoint, impossible latest run given one new interval.
        old=dict(self.rows[-2]); old['run']=1; old['maximum']=100
        old['episodes']=10; validate(old)
        new=dict(old); new['now']+=1; new['sequence']+=1
        new['intervals']+=1; new['bin8']+=1; new['run']=20
        validate(new); s=Session('replay'); s.accept(old)
        with self.assertRaises(ValueError): s.accept(new)
    def test_missing_acquisition_requires_halt(self):
        old=self.rows[0]; new=dict(old)
        new['now']+=100; new['sequence']+=1; new['intervals']+=1; new['bin0']+=1
        validate(new); s=Session('replay'); s.accept(old)
        with self.assertRaises(ValueError): s.accept(new)
    def test_run_cannot_consume_unknown(self):
        old=dict(self.rows[-2]); old['run']=1
        new=dict(old); new['now']+=10; new['sequence']+=10
        new['intervals']+=10; new['unknown']+=10; new['run']=11
        validate(new); s=Session('replay'); s.accept(old)
        with self.assertRaises(ValueError): s.accept(new)
    def test_adjacent_cell_attribution(self):
        old=dict(self.rows[4]); new=dict(old)
        new['now']+=1; new['sequence']+=1; new['intervals']+=1
        new['bin8']+=1; new['run']+=1; new['maximum']+=1
        validate(new); s=Session('replay'); s.accept(old)
        with self.assertRaises(ValueError): s.accept(new)
        new['bin8']-=1; new['bin4']+=1
        s=Session('replay'); s.accept(old); s.accept(new)
    def test_startup_credit(self):
        r={k:0 for k in FIELDS}
        r.update(sequence=2,now=2,intervals=1,bin0=1)
        with self.assertRaises(ValueError): validate(r)
    def test_freshness_duplicates_drift(self):
        s=Session('live'); s.accept(self.rows[0],0); s.accept(self.rows[1],.1)
        self.assertTrue(s.fresh(True,.2)); self.assertFalse(s.fresh(False,.2))
        s.accept(self.rows[1],.9); self.assertFalse(s.fresh(True,1.2))
        with self.assertRaises(ValueError): s.accept(self.rows[2],5)
        self.assertTrue(s.blocked)
    def test_export_and_files(self):
        with tempfile.TemporaryDirectory() as temp:
            path=Path(temp)/'result.csv'; s=Session('replay'); s.errors=2
            s.accept(self.rows[0]); s.export(path,False)
            self.assertIn('jayis1,replay,False,False,2',path.read_text())
            self.assertEqual(path.stat().st_mode&0o777,0o600)
            with self.assertRaises(FileExistsError): s.export(path,False)
            fifo=Path(temp)/'fifo'; os.mkfifo(fifo)
            with self.assertRaises(ValueError): load_replay(fifo)
            empty=Path(temp)/'empty'; empty.write_bytes(b'')
            with self.assertRaises(ValueError): load_replay(empty)
    def test_serial_restore_and_hangup(self):
        import pty, termios
        master,slave=pty.openpty(); before=termios.tcgetattr(slave)
        serial=Serial(os.ttyname(slave)); serial.close()
        self.assertEqual(termios.tcgetattr(slave),before)
        serial=Serial(os.ttyname(slave)); os.close(master)
        with self.assertRaises(OSError): serial.read()
        serial.close(); os.close(slave)
if __name__=='__main__': unittest.main()
