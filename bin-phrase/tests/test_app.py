# Author: jayis1. SPDX-License-Identifier: MIT
import unittest, sys, pathlib, binascii, tempfile, os, pty
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/'app'))
from model import Session, validate, phrase, FIELDS
from transport import Decoder, load_replay, Serial
ROOT=pathlib.Path(__file__).resolve().parents[1]
class Tests(unittest.TestCase):
    def setUp(self):
        self.data=(ROOT/'firmware/build/demo.bq1').read_bytes()
        self.rows=Decoder().feed(self.data,strict=True)
    def test_encoder_fixture(self):
        s=load_replay(ROOT/'firmware/build/demo.bq1')
        self.assertEqual(s.rows[-1]['completed'],1)
        self.assertEqual(phrase(s.rows[-1],True),'A → {B+C} → A')
    def test_crc_and_resync(self):
        d=Decoder(); rows=d.feed(b'BQ1,broken'+self.data)
        self.assertEqual(len(rows),len(self.rows)); self.assertEqual(d.errors,1)
        d=Decoder()
        with self.assertRaises(ValueError): d.feed(b'BQ1,0*0000\n',True)
        d=Decoder(); d.feed(self.data+b'BQ1,')
        with self.assertRaises(ValueError): d.finish()
    def test_invariants(self):
        r=dict(self.rows[-1]); r['last0']=0
        with self.assertRaises(ValueError): validate(r)
        r=dict(self.rows[-1]); r['starts']+=1
        with self.assertRaises(ValueError): validate(r)
    def test_session_regression(self):
        s=Session('live'); s.accept(self.rows[-1]); r=dict(self.rows[-1]); r['sequence']-=1
        with self.assertRaises(ValueError): s.accept(r)
        with self.assertRaises(ValueError): s.accept(self.rows[-1])
    def test_frozen_result(self):
        s=Session('replay'); s.accept(self.rows[-1]); r=dict(self.rows[-1])
        r['now']+=1; r['sequence']+=1; r['last0']=2
        with self.assertRaises(ValueError): s.accept(r)
    def test_file_bounds(self):
        with tempfile.TemporaryDirectory() as directory:
            path=pathlib.Path(directory)/'pipe'; os.mkfifo(path)
            with self.assertRaises(ValueError): load_replay(path)
    def test_serial(self):
        master,slave=pty.openpty(); serial=Serial(os.ttyname(slave))
        try:
            os.write(master,self.data[:200]); self.assertTrue(serial.read())
        finally: serial.close(); os.close(master); os.close(slave)
    def test_export(self):
        s=Session('replay'); s.accept(self.rows[-1]); s.errors=2
        with tempfile.TemporaryDirectory() as directory:
            path=pathlib.Path(directory)/'report.csv'; s.export(path,False)
            text=path.read_text(); self.assertIn('jayis1',text); self.assertIn('replay',text)
            with self.assertRaises(FileExistsError): s.export(path,False)
    def test_valid_crc_impossible_record(self):
        r=dict(self.rows[-1]); r['last_count']=17
        body=('BQ1,'+','.join(str(r[k]) for k in FIELDS)).encode()
        frame=body+f'*{binascii.crc_hqx(body,65535):04X}\n'.encode()
        with self.assertRaises(ValueError): Decoder().feed(frame,True)
    def test_live_pause_and_staleness(self):
        s=Session('live'); s.accept(self.rows[0],host=10)
        s.accept(self.rows[1],host=10.001)
        self.assertTrue(s.fresh(True,10.002))
        self.assertFalse(s.fresh(True,12))
        with self.assertRaises(ValueError): s.accept(self.rows[2],host=13)
        self.assertTrue(s.blocked)
    def test_duplicate_does_not_refresh(self):
        s=Session('live'); s.accept(self.rows[0],host=10)
        s.accept(self.rows[1],host=10.001)
        s.accept(self.rows[1],host=12)
        self.assertFalse(s.fresh(True,12))
if __name__=='__main__': unittest.main()
