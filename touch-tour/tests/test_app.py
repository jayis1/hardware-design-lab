# Author: jayis1. SPDX-License-Identifier: MIT
import binascii
import copy
from pathlib import Path
import sys
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'app'))
from model import FIELDS, Session, validate
from transport import Decoder, load_replay

def encoded(row):
    body=('TT1,'+','.join(str(row[k]) for k in FIELDS)).encode()
    return body+b'*'+f'{binascii.crc_hqx(body,65535):04X}\n'.encode()

class Tests(unittest.TestCase):
    def test_real_encoder_and_sparse(self):
        path=Path(__file__).resolve().parents[1]/'firmware/build/demo.tt1'
        session=load_replay(path)
        self.assertGreater(session.rows[-1]['rounds'],0)
        rows=list(session.rows)
        for step in (1,2,7,23):
            sparse=Session('replay')
            for row in rows[::step]: sparse.accept(row)
    def test_crc_resync_tail(self):
        row={k:0 for k in FIELDS}
        frame=encoded(row)
        d=Decoder(); self.assertEqual(len(d.feed(frame[:8]+frame)),1)
        self.assertEqual(d.errors,1)
        d.feed(frame[:8])
        with self.assertRaises(ValueError): d.finish()
    def test_invalid_accounting(self):
        row={k:0 for k in FIELDS}; row['rounds']=1
        with self.assertRaises(ValueError): Decoder().feed(encoded(row),True)
    def test_rollback_and_latched_error(self):
        s=Session('replay'); row={k:0 for k in FIELDS}
        row.update(now=100,samples=100)
        s.accept(row)
        bad=copy.copy(row); bad['samples']=99; bad['now']=101
        with self.assertRaises(ValueError): s.accept(bad)
        with self.assertRaises(ValueError): s.accept(row)
    def test_freshness_not_receipt(self):
        s=Session('live'); s.connected=True; row={k:0 for k in FIELDS}
        row.update(now=100,samples=100)
        s.accept(row,10); self.assertFalse(s.fresh(10))
        row=dict(row,now=200,samples=200); s.accept(row,10.1)
        self.assertTrue(s.fresh(10.1))
        with self.assertRaises(ValueError): s.accept(row,11.3)
        self.assertFalse(s.fresh(11.3))
        self.assertTrue(s.blocked)
    def test_export_private_and_no_overwrite(self):
        import tempfile, os, json
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'export.json'; s=Session('replay'); s.export(p)
            self.assertEqual(os.stat(p).st_mode & 0o777,0o600)
            self.assertEqual(json.loads(p.read_text())['author'],'jayis1')
            self.assertFalse(json.loads(p.read_text())['live'])
            with self.assertRaises(FileExistsError): s.export(p)
    def test_replay_fifo_and_size(self):
        import tempfile, os
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'pipe'; os.mkfifo(p)
            with self.assertRaises(ValueError): load_replay(p)
            p=Path(d)/'large'
            with p.open('wb') as h: h.truncate(2*1024*1024+1)
            with self.assertRaises(ValueError): load_replay(p)
    def test_serial_bad_path(self):
        from transport import Serial
        import termios
        with self.assertRaises((OSError,termios.error)): Serial('/dev/null')
        with self.assertRaises(OSError): Serial('/not/a/real/serial/path')
if __name__=='__main__': unittest.main()
