# Author: jayis1. SPDX-License-Identifier: MIT
import binascii
from pathlib import Path
import os
import pty
import sys
import tempfile
import termios
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'app'))
from model import Session, FIELDS, validate
from transport import Decoder, Serial, load_replay
ROOT=Path(__file__).resolve().parents[1]
def encoded(row):
    body=('AR1,'+','.join(str(row[x]) for x in FIELDS)).encode()
    return body+b'*'+('%04X'%binascii.crc_hqx(body,65535)).encode()+b'\n'
class ModelTest(unittest.TestCase):
    def rows(self):
        return Decoder().feed((ROOT/'firmware/build/demo.ar1').read_bytes(),True)
    def test_real_encoder(self):
        rows=self.rows(); self.assertEqual(len(rows),10)
        session=load_replay(ROOT/'firmware/build/demo.ar1')
        self.assertEqual(len(session.rows),10)
        last=rows[-1]
        self.assertEqual(last['insertions'],6)
        self.assertEqual(last['removals'],6)
        self.assertEqual(last['ambiguous'],2)
        # C and D depart together: both ranks use the pre-group inventory.
        self.assertEqual([last['rank'+str(i)] for i in range(4)],[2,2,0,0])
        self.assertFalse(session.fresh())
    def test_crc_and_fragment_recovery(self):
        data=encoded(self.rows()[0]); decoder=Decoder()
        self.assertEqual(len(decoder.feed(data[:20]+data)),1)
        self.assertEqual(decoder.errors,1)
        with self.assertRaises(ValueError): Decoder().feed(data.replace(b'AR1,',b'AR1,9',1),True)
        decoder=Decoder(); decoder.feed(data[:-1])
        with self.assertRaises(ValueError): decoder.finish()
    def test_impossible_frames(self):
        row=self.rows()[-1]
        for key,value in [('removals',7),('high0',4),('stamp0',1),('last_mask',0),('state',2)]:
            bad=dict(row); bad[key]=value
            with self.assertRaises(ValueError): Decoder().feed(encoded(bad),True)
    def test_transition_guards(self):
        rows=self.rows()
        session=Session('replay'); session.accept(rows[2])
        bad=dict(rows[2]); bad['now']+=1; bad['sequence']+=1
        bad['stamp0']=2
        validate(bad)
        with self.assertRaises(ValueError): session.accept(bad)
        session=Session('replay'); session.accept(rows[-1])
        bad=dict(rows[-1]); bad['now']+=1; bad['sequence']+=1
        bad['rank0']-=1; bad['rank1']+=1
        validate(bad)
        with self.assertRaises(ValueError): session.accept(bad)
    def test_freshness_export(self):
        rows=self.rows(); s=Session('live'); s.connected=True
        s.accept(rows[0],10); s.accept(rows[1],10.01)
        self.assertTrue(s.fresh(10.02)); self.assertFalse(s.fresh(12))
        s.accept(rows[1],10.9); self.assertFalse(s.fresh(11.1))
        with self.assertRaises(ValueError): s.accept(rows[2],20)
        with tempfile.TemporaryDirectory() as temp:
            p=Path(temp)/'x.csv'; s.export(p)
            self.assertIn('jayis1',p.read_text()); self.assertEqual(p.stat().st_mode&0o777,0o600)
            with self.assertRaises(FileExistsError): s.export(p)
    def test_single_departure_histogram_attribution(self):
        rows=self.rows()
        prior=dict(rows[5]); after=dict(rows[6])
        # Add one historical unique rank-zero removal to both endpoints.
        # A CRC-valid after-frame can now hide a misattributed new removal
        # behind that preexisting rank-zero budget unless deltas are checked.
        for row in (prior,after):
            row['insertions']+=1; row['removals']+=1; row['rank0']+=1
            validate(row)
        s=Session('replay'); s.accept(prior); s.accept(after)
        bad=dict(after); bad['rank0']-=1; bad['rank1']+=1
        validate(bad)
        s=Session('replay'); s.accept(prior)
        with self.assertRaises(ValueError):
            s.accept(Decoder().feed(encoded(bad),True)[0])
    def test_replay_bounds(self):
        with tempfile.TemporaryDirectory() as temp:
            p=Path(temp)/'x'; p.write_bytes(b'')
            with self.assertRaises(ValueError): load_replay(p)
            p.write_bytes(encoded(self.rows()[0])+b'AR')
            with self.assertRaises(ValueError): load_replay(p)
            p.unlink(); os.mkfifo(p)
            with self.assertRaises(ValueError): load_replay(p)
    def test_serial_restore(self):
        master,slave=pty.openpty()
        try:
            original=termios.tcgetattr(slave)
            port=Serial(os.ttyname(slave)); port.close()
            self.assertEqual(termios.tcgetattr(slave),original)
        finally: os.close(master); os.close(slave)
        with self.assertRaises((OSError,termios.error)): Serial('/dev/null')
if __name__=='__main__': unittest.main()
