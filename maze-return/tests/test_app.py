# Author: jayis1. SPDX-License-Identifier: MIT
import sys
import binascii
import unittest
import subprocess
import tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'app'))
from protocol import Decoder, Session, validate, load_replay, FIELDS

class ProtocolTests(unittest.TestCase):
    def setUp(self):
        self.data=subprocess.check_output([str(ROOT/'firmware/build/test-engine'),'--fixture'])
        self.rows=Decoder().feed(self.data,strict=True)
    def test_real_encoder(self):
        self.assertEqual(len(self.rows),2)
        self.assertEqual(self.rows[-1]['visits'],6)
        self.assertEqual(self.rows[-1]['loops'],1)
        self.assertEqual(self.rows[-1]['depth'],3)
    def test_bad_evidence(self):
        for key,value in [('visits',0),('depth',4),('route',0),('erased',9),('pending',9)]:
            row=dict(self.rows[0]); row[key]=value
            with self.assertRaises(ValueError): validate(row)
    def test_terminal_encoder_and_valid_crc_negatives(self):
        data=subprocess.check_output([str(ROOT/'firmware/build/test-engine'),'--fault-fixtures'])
        rows=Decoder().feed(data,strict=True)
        self.assertEqual([r['fault'] for r in rows],[16,8,32,4])
        old=dict(self.rows[0],visits=2,added=2,loops=0,repeats=0,erased=0,depth=2,route=4)
        new=dict(old,now=old['now']+200,visits=3,added=3,depth=3,route=28)
        def encode(row):
            body=('MR1,'+','.join(str(row[k]) for k in FIELDS)).encode()
            return body+f'*{binascii.crc_hqx(body,65535):04X}\n'.encode()
        decoded=Decoder().feed(encode(old)+encode(new),strict=True)
        session=Session('replay'); session.accept(decoded[0])
        with self.assertRaises(ValueError): session.accept(decoded[1])
    def test_framing(self):
        decoder=Decoder(); self.assertEqual(len(decoder.feed(b'junk'+self.data)),2)
        self.assertGreater(decoder.errors,0)
        self.assertEqual(len(Decoder().feed(self.data[:20]+self.data)),2)
        decoder=Decoder(); decoder.feed(self.data[:-2])
        with self.assertRaises(ValueError): decoder.finish()
        bad=self.data.replace(b'MR1,',b'MR1,9',1)
        with self.assertRaises(ValueError): Decoder().feed(bad,strict=True)
    def test_session(self):
        s=Session('live'); s.connected=True
        s.accept(self.rows[0],10); s.accept(self.rows[1],10.1)
        self.assertTrue(s.fresh(10.2)); self.assertFalse(s.fresh(12))
        s.connected=False; self.assertFalse(s.fresh(10.2))
        r=dict(self.rows[1]); r['tx_errors']=1
        with self.assertRaises(ValueError): s.accept(r,10.2)
    def test_impossible_transition(self):
        old=dict(self.rows[0],visits=2,added=2,loops=0,repeats=0,erased=0,depth=2,route=4)
        new=dict(old,now=old['now']+200,visits=3,added=3,depth=3,route=27)
        validate(old); validate(new)
        session=Session('replay'); session.accept(old)
        with self.assertRaises(ValueError): session.accept(new)
        # Same root is not sufficient: A B -> A D B is not one append.
        new['route']=28
        session=Session('replay'); session.accept(old)
        with self.assertRaises(ValueError): session.accept(new)
    def test_replay_export(self):
        with tempfile.TemporaryDirectory() as tmp:
            source=Path(tmp)/'input'; source.write_bytes(self.data)
            session=load_replay(source)
            self.assertFalse(session.fresh())
            dest=Path(tmp)/'out'; session.export(dest,'synthetic fixture')
            self.assertIn('jayis1',dest.read_text())
            self.assertEqual(dest.stat().st_mode & 0o777,0o600)
            with self.assertRaises(FileExistsError): session.export(dest)
            source.write_bytes(b'')
            with self.assertRaises(ValueError): load_replay(source)
if __name__=='__main__': unittest.main()
