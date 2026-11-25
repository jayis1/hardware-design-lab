# Author: jayis1. SPDX-License-Identifier: MIT
import sys, unittest, binascii
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'app'))
from model import FIELDS, validate, Session
from transport import Decoder, load_replay

def frame(row):
    data=('SM1,'+','.join(str(row[x]) for x in FIELDS)).encode()
    return data+f'*{binascii.crc_hqx(data,65535):04X}\n'.encode()

class Tests(unittest.TestCase):
    def test_encoder_fixture(self):
        path=Path(__file__).resolve().parents[1]/'firmware/build/demo.sm1'
        session=load_replay(path)
        self.assertEqual(session.rows[-1]['completed'],1)
        self.assertEqual(session.rows[-1]['mismatch_x'],0x5555)
        self.assertEqual(session.rows[-1]['mismatch_y'],0xaaaa)
    def test_crc_and_tail(self):
        row=dict.fromkeys(FIELDS,0)
        d=Decoder()
        self.assertEqual(d.feed(frame(row),True),[row])
        with self.assertRaises(ValueError): d.feed(frame(row).replace(b'SM1,0',b'SM1,1',1),True)
        d=Decoder(); d.feed(frame(row)[:-1])
        with self.assertRaises(ValueError): d.finish()
    def test_impossible(self):
        row=dict.fromkeys(FIELDS,0)
        row['mismatch_x']=1
        with self.assertRaises(ValueError): validate(row)
    def test_duplicate_conflict(self):
        row=dict.fromkeys(FIELDS,0)
        s=Session('replay'); s.accept(row)
        other=row|{'now':1}
        with self.assertRaises(ValueError): s.accept(other)
    def test_fragment_recovery(self):
        row=dict.fromkeys(FIELDS,0); d=Decoder()
        self.assertEqual(d.feed(b'badSM1,0,'+frame(row)),[row])
        self.assertGreater(d.errors,0)
    def test_counter_reset(self):
        row=dict.fromkeys(FIELDS,0); row.update(now=10,samples=10,state=1,valid=1)
        s=Session('live'); s.accept(row)
        with self.assertRaises(ValueError): s.accept(row|{'now':20,'samples':1})
if __name__=='__main__': unittest.main()
