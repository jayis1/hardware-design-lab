# Author: jayis1. SPDX-License-Identifier: MIT
import sys, unittest, tempfile, os, binascii
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'app'))
from model import FIELDS, Session, validate, bracket
from transport import Decoder, load_replay

def row(**kw):
    r=dict.fromkeys(FIELDS,0); r.update(state=1,valid=15); r.update(kw); return r

def frame(r):
    body=('SB1,'+','.join(str(r[k]) for k in FIELDS)).encode()
    return body+b'*'+f'{binascii.crc_hqx(body,65535):04X}\n'.encode()

class Contract(unittest.TestCase):
    def test_impossible_active_and_completion(self):
        for changes in ({'age':10000}, {'outcome':12}):
            r=row(state=2,starts=1,outcome=4,**{'levels':4})
            r.update(changes)
            with self.assertRaises(ValueError): validate(r)
        for tainted, masks in ((1,{'pass':1}), (0,{'refuse':8})):
            s=Session('replay')
            s.accept(row(state=2,starts=1,outcome=4,levels=4,tainted=tainted))
            with self.assertRaises(ValueError):
                s.accept(row(now=100,starts=1,accepted=1,outcome=4,**masks))
    def test_all_masks(self):
        for p in range(16):
            for n in range(16):
                conflict=any(p&(1<<i) and n&(1<<j) for i in range(4) for j in range(i+1))
                count=p.bit_count()+n.bit_count()
                r=row(starts=count,accepted=count,**{'pass':p,'refuse':n,'conflict':int(conflict)})
                validate(r)
                self.assertEqual('CONTRADICTORY' in bracket(r),bool(conflict))
                r['conflict']=1-r['conflict']
                with self.assertRaises(ValueError): validate(r)
    def test_rollback_and_no_new_mask_without_credit(self):
        s=Session('replay'); s.accept(row())
        with self.assertRaises(ValueError): s.accept(row(now=100,**{'pass':1}))
        s=Session('replay'); s.accept(row(starts=1,accepted=1,**{'pass':1}))
        with self.assertRaises(ValueError): s.accept(row(now=100,starts=1,accepted=1,**{'pass':2}))
    def test_freshness(self):
        s=Session('live'); r=row(); s.accept(r,0); self.assertFalse(s.fresh(0))
        s.accept(row(now=100),.1); self.assertTrue(s.fresh(.1))
        s.accept(row(now=100),1); self.assertFalse(s.fresh(3))
        with self.assertRaises(ValueError): s.accept(row(now=200),4)
    def test_decoder(self):
        data=frame(row()); d=Decoder()
        self.assertEqual(d.feed(data[:7]),[])
        self.assertEqual(d.feed(data[7:]),[row()]); d.finish()
        d=Decoder(); self.assertEqual(d.feed(data[:9]+data),[row()]); self.assertEqual(d.errors,1)
        d=Decoder()
        with self.assertRaises(ValueError): d.feed(data.replace(b'SB1,0',b'SB1,1'),strict=True)
        d=Decoder(); d.feed(data[:-1])
        with self.assertRaises(ValueError): d.finish()
    def test_private_export_replay(self):
        with tempfile.TemporaryDirectory() as t:
            p=Path(t)/'record'; p.write_bytes(frame(row()))
            s=load_replay(p); self.assertFalse(s.fresh())
            out=Path(t)/'output.csv'; s.export(out,'=unsafe')
            self.assertEqual(out.stat().st_mode&0o777,0o600)
            self.assertIn('jayis1',out.read_text())
            with self.assertRaises(FileExistsError): s.export(out,'again')
            p.write_bytes(frame(row())+b'SB1,')
            with self.assertRaises(ValueError): load_replay(p)
            fifo=Path(t)/'fifo'; os.mkfifo(fifo)
            with self.assertRaises(ValueError): load_replay(fifo)
if __name__=='__main__': unittest.main()
