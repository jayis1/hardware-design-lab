# Author: jayis1. SPDX-License-Identifier: MIT
import sys, unittest, tempfile, pathlib, binascii
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/'app'))
from model import Session, validate, FIELDS
from transport import Decoder, load_replay
ROOT=pathlib.Path(__file__).resolve().parents[1]
class Tests(unittest.TestCase):
    def setUp(self):
        self.data=(ROOT/'firmware/build/demo.sv1').read_bytes()
        self.rows=Decoder().feed(self.data,True)
    def test_real_encoder(self):
        s=load_replay(ROOT/'firmware/build/demo.sv1')
        self.assertEqual(s.rows[-1]['completed'],2)
        self.assertEqual(s.rows[-1]['nested'],1)
        self.assertEqual(s.rows[-1]['inner_trips'],1)
        self.assertEqual(s.rows[-1]['aborted'],1)
    def test_all_sparse_pairs(self):
        for i,a in enumerate(self.rows):
            for b in self.rows[i+1:]:
                s=Session('replay'); s.accept(a); s.accept(b)
    def test_bad_semantics(self):
        for key,value in [('completed',999),('phase',99),('qualify',4),('nested',999),('age',60000)]:
            row=dict(self.rows[-1]); row[key]=value
            with self.assertRaises(ValueError): validate(row)
    def test_crc_and_tail(self):
        with self.assertRaises(ValueError): Decoder().feed(self.data.replace(b'SV1',b'SV2',1),True)
        d=Decoder(); d.feed(self.data+b'SV1,')
        with self.assertRaises(ValueError): d.finish()
        d=Decoder(); rows=d.feed(b'SV1,123'+self.data)
        self.assertTrue(d.errors); self.assertEqual(len(rows),len(self.rows))
    def test_reset_and_duplicates(self):
        s=Session('live'); s.accept(self.rows[0],0); s.accept(self.rows[1],.001)
        self.assertTrue(s.fresh(True,.002))
        s.accept(self.rows[1],.5)
        self.assertFalse(s.fresh(True,1.1))
        bad=dict(self.rows[1]); bad['raw']^=1
        with self.assertRaises(ValueError): s.accept(bad,.6)
        self.assertTrue(s.blocked)
    def test_pause(self):
        s=Session('live'); s.accept(self.rows[0],0)
        with self.assertRaises(ValueError): s.accept(self.rows[1],3)
    def test_latest_duration_cannot_exceed_observed_budget(self):
        a = self.rows[0]
        b = dict(self.rows[-1]); b['last_age'] = 59999
        s = Session('replay'); s.accept(a)
        with self.assertRaises(ValueError): s.accept(b)
    def test_terminal_fault_bits_cannot_disappear(self):
        a = dict(self.rows[0]); a.update(phase=8, fault=3)
        b = dict(a); b.update(now=a['now']+1, fault=1)
        s = Session('replay'); s.accept(a)
        with self.assertRaises(ValueError): s.accept(b)
    def test_no_acquisition_cannot_create_completed_cycle(self):
        a = dict(self.rows[0])
        b = dict(a); b.update(now=a['now']+1, starts=1, completed=1, last=1)
        s = Session('replay'); s.accept(a)
        with self.assertRaises(ValueError): s.accept(b)
    def test_export(self):
        with tempfile.TemporaryDirectory() as d:
            p=pathlib.Path(d)/'x.csv'; s=Session('replay'); s.export(p,False)
            self.assertIn('jayis1',p.read_text()); self.assertEqual(p.stat().st_mode & 0o777,0o600)
            with self.assertRaises(FileExistsError): s.export(p,False)
if __name__=='__main__': unittest.main()
