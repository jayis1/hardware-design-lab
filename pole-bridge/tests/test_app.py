# Author: jayis1. SPDX-License-Identifier: MIT
import binascii
from pathlib import Path
import os
import pty
import sys
import tempfile
import termios
import unittest
from unittest.mock import patch
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'app'))
from model import Session, FIELDS, validate
from transport import Decoder, Serial, load_replay
ROOT=Path(__file__).resolve().parents[1]
def encoded(row):
    body=('PB1,'+','.join(str(row[x]) for x in FIELDS)).encode()
    return body+b'*'+('%04X'%binascii.crc_hqx(body,65535)).encode()+b'\n'
class ModelTest(unittest.TestCase):
    def rows(self):
        return Decoder().feed((ROOT/'firmware/build/demo.pb1').read_bytes(),True)
    def test_real_encoder(self):
        rows=self.rows()
        session=load_replay(ROOT/'firmware/build/demo.pb1')
        self.assertEqual(len(session.rows),len(rows))
        self.assertEqual(rows[-1]['completed'],2)
        self.assertEqual(rows[-1]['returned'],1)
        self.assertEqual(rows[-1]['last_outcome'],2)
        self.assertTrue(any(r['last_mixed'] for r in rows))
        self.assertTrue(any(r['last_bin3'] for r in rows))
        self.assertFalse(session.fresh())
    def test_sparse_real_encoder(self):
        rows=self.rows()
        for step in (2,3,7,20,50,100):
            s=Session('replay')
            for row in rows[::step]: s.accept(row)
    def test_interval_partition_oracle(self):
        # Build interval sets from adjacent raw/qualified observations, then
        # compare set cardinalities with the C engine's cumulative bins.
        rows=self.rows()
        buckets=[[set() for _ in range(5)] for _ in range(2)]
        mixed=set()
        for old,new in zip(rows,rows[1:]):
            if new['starts']>old['starts']:
                buckets=[[set() for _ in range(5)] for _ in range(2)]
                mixed=set()
            if old['state']!=2:
                continue
            tick=new['sequence']
            for pole in range(2):
                values=[(r[key]>>(pole*2))&3 for r in (old,new) for key in ('raw','qualified')]
                code=values[0] if len(set(values))==1 else 4
                buckets[pole][code].add(tick)
            if old['raw']==old['qualified']==new['raw']==new['qualified'] and new['raw'] in (6,9):
                mixed.add(tick)
            prefix='work_' if new['state']==2 else 'last_'
            for pole in range(2):
                for code in range(5):
                    key='bin'+str(pole*4+code) if code<4 else 'unknown'+str(pole)
                    self.assertEqual(new[prefix+key],len(buckets[pole][code]))
            self.assertEqual(new[prefix+'mixed'],len(mixed))
    def test_crc_fragment_and_tail(self):
        data=encoded(self.rows()[0]); decoder=Decoder()
        self.assertEqual(len(decoder.feed(data[:20]+data)),1)
        self.assertEqual(decoder.errors,1)
        with self.assertRaises(ValueError): Decoder().feed(data.replace(b'PB1,',b'PB1,9',1),True)
        decoder=Decoder(); decoder.feed(data[:-1])
        with self.assertRaises(ValueError): decoder.finish()
        decoder=Decoder(); out=[]
        for byte in data: out+=decoder.feed(bytes([byte]))
        self.assertEqual(len(out),1)
    def test_impossible_frames(self):
        row=self.rows()[-1]
        for key,value in [('completed',7),('last_age',5000),('last_mixed',5000),('last_origin',0),('state',3)]:
            bad=dict(row); bad[key]=value
            with self.assertRaises(ValueError): Decoder().feed(encoded(bad),True)
    def test_retained_coverage_migration(self):
        row=self.rows()[-1]; s=Session('replay'); s.accept(row)
        bad=dict(row); bad['now']+=1; bad['sequence']+=1
        bad['last_bin0']-=1; bad['last_unknown0']+=1
        validate(bad)
        with self.assertRaises(ValueError): s.accept(Decoder().feed(encoded(bad),True)[0])
    def test_active_time_and_completed_budget(self):
        rows=self.rows(); active=next(r for r in rows if r['state']==2 and r['work_age']>5)
        s=Session('replay'); s.accept(active)
        bad=dict(active); bad['now']+=1; bad['sequence']+=1
        validate(bad)
        with self.assertRaises(ValueError): s.accept(bad)
        done=next(r for r in rows if r['completed']==1)
        bad=dict(done); bad['now']=active['now']+1; bad['sequence']=active['sequence']+1
        validate(bad)
        s=Session('replay'); s.accept(active)
        with self.assertRaises(ValueError): s.accept(bad)
    def test_anchor_cannot_teleport(self):
        ready=next(r for r in self.rows() if r['state']==1)
        bad=dict(ready); bad['now']+=1; bad['sequence']+=1
        bad['raw']=bad['qualified']=bad['anchor']=10
        validate(bad)
        s=Session('replay'); s.accept(ready)
        with self.assertRaises(ValueError): s.accept(Decoder().feed(encoded(bad),True)[0])
    def test_fault_cannot_start_without_acquisition(self):
        ready=next(r for r in self.rows() if r['state']==1)
        bad=dict(ready); bad['now']+=1
        bad.update(state=3,fault=1,anchor=0,starts=1,aborted=1)
        validate(bad)
        s=Session('replay'); s.accept(ready)
        with self.assertRaises(ValueError): s.accept(Decoder().feed(encoded(bad),True)[0])
    def test_freshness_and_export(self):
        rows=self.rows(); s=Session('live'); s.connected=True
        s.accept(rows[0],10); s.accept(rows[1],10.001)
        self.assertTrue(s.fresh(10.02)); self.assertFalse(s.fresh(12))
        s.accept(rows[1],10.9); self.assertFalse(s.fresh(11.1))
        with self.assertRaises(ValueError): s.accept(rows[2],20)
        with tempfile.TemporaryDirectory() as temp:
            p=Path(temp)/'x.csv'; s.export(p)
            self.assertIn('jayis1',p.read_text()); self.assertEqual(p.stat().st_mode&0o777,0o600)
            with self.assertRaises(FileExistsError): s.export(p)
    def test_replay_bounds(self):
        with tempfile.TemporaryDirectory() as temp:
            p=Path(temp)/'x'; p.write_bytes(b'')
            with self.assertRaises(ValueError): load_replay(p)
            p.write_bytes(encoded(self.rows()[0])+b'PB')
            with self.assertRaises(ValueError): load_replay(p)
            p.write_bytes(b'x'*(2*1024*1024+1))
            with self.assertRaises(ValueError): load_replay(p)
            p.unlink(); os.mkfifo(p)
            with self.assertRaises(ValueError): load_replay(p)
    def test_serial_restore(self):
        master,slave=pty.openpty()
        try:
            original=termios.tcgetattr(slave)
            port=Serial(os.ttyname(slave)); port.close()
            self.assertEqual(termios.tcgetattr(slave),original)
            with patch('termios.tcflush',side_effect=OSError('injected')):
                with self.assertRaises(OSError): Serial(os.ttyname(slave))
            self.assertEqual(termios.tcgetattr(slave),original)
        finally: os.close(master); os.close(slave)
        with self.assertRaises((OSError,termios.error)): Serial('/dev/null')
if __name__=='__main__': unittest.main()
