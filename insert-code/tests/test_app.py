# Author: jayis1. SPDX-License-Identifier: MIT
import pathlib, sys, unittest, binascii
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/'app'))
from model import Session, validate, FIELDS
from transport import Decoder, load_replay
ROOT=pathlib.Path(__file__).resolve().parents[1]
class ModelTests(unittest.TestCase):
    def setUp(self):
        self.data=(ROOT/'firmware/build/demo.ic1').read_bytes()
        self.rows=Decoder().feed(self.data,strict=True)
    def test_real_encoder(self):
        s=load_replay(ROOT/'firmware/build/demo.ic1')
        r=s.rows[-1]
        self.assertEqual((r['starts'],r['completed'],r['changed'],r['invalid']),(3,3,1,2))
        self.assertEqual((r['seats0'],r['seats5'],r['unidentified']),(1,1,1))
    def test_all_sparse_pairs(self):
        for i,a in enumerate(self.rows):
            for b in self.rows[i+1:]:
                s=Session('replay'); s.accept(a); s.accept(b)
    def test_framing(self):
        d=Decoder(); rows=[]
        for byte in self.data: rows+=d.feed(bytes([byte]),strict=True)
        self.assertEqual(rows,self.rows); d.finish()
        d=Decoder(); d.feed(self.data[:-1]); self.assertRaises(ValueError,d.finish)
        d=Decoder(); self.assertEqual(len(d.feed(b'IC1,broken'+self.data)),len(self.rows))
        self.assertGreater(d.errors,0)
    def test_crc_valid_impossible(self):
        r=self.rows[-1].copy(); r['completed']+=1
        body=('IC1,'+','.join(str(r[k]) for k in FIELDS)).encode()
        frame=body+f'*{binascii.crc_hqx(body,65535):04X}\n'.encode()
        self.assertRaises(ValueError,Decoder().feed,frame,True)
    def test_identity_migration(self):
        active=[r for r in self.rows if r['state']==2 and r['identity']==1]
        a,b=active[0],active[-1].copy(); b['identity']=2; b['seats1']=1; b['seats0']=0
        self.assertRaises(ValueError,SessionPair,a,b)
    def test_required_taints(self):
        active=next(r for r in self.rows if r['state']==2 and r['identity']==1)
        for word in (1,5):
            r=active.copy(); r['qualified']=word
            body=('IC1,'+','.join(str(r[k]) for k in FIELDS)).encode()
            frame=body+f'*{binascii.crc_hqx(body,65535):04X}\n'.encode()
            self.assertRaises(ValueError,Decoder().feed,frame,True)
    def test_freshness(self):
        s=Session('live'); s.accept(self.rows[0],0); s.accept(self.rows[1],.001)
        self.assertTrue(s.fresh(.1)); s.accept(self.rows[1],.9)
        self.assertFalse(s.fresh(1.1)); s.connected=False; self.assertFalse(s.fresh(.1))
        s=Session('live'); s.accept(self.rows[0],0)
        self.assertRaises(ValueError,s.accept,self.rows[1],2)
    def test_replay_bounds(self):
        import tempfile, os
        with tempfile.TemporaryDirectory() as temp:
            p=pathlib.Path(temp)/'fifo'; os.mkfifo(p)
            self.assertRaises(ValueError,load_replay,p)
            p=pathlib.Path(temp)/'empty'; p.write_bytes(b'')
            self.assertRaises(ValueError,load_replay,p)
def SessionPair(a,b):
    s=Session('replay'); s.accept(a); s.accept(b)
if __name__=='__main__': unittest.main()
