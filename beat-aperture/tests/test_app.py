# Author: jayis1. SPDX-License-Identifier: MIT
import binascii
import csv
import os
from pathlib import Path
import pty
import sys
import tempfile
import termios
import time
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'app'))
from model import FIELDS, Session, validate
from transport import Decoder, Serial, load_replay
from workbench import Workbench
ROOT=Path(__file__).resolve().parents[1]
def frame(row):
    body=('BA1,'+','.join(str(row[k]) for k in FIELDS)).encode()
    return body+b'*'+f'{binascii.crc_hqx(body,65535):04X}\n'.encode()
class AppTests(unittest.TestCase):
    def setUp(self):
        self.data=(ROOT/'firmware/build/demo.ba1').read_bytes()
        self.rows=Decoder().feed(self.data,strict=True)
    def test_real_encoder(self):
        s=load_replay(ROOT/'firmware/build/demo.ba1')
        r=s.rows[-1]
        self.assertEqual((r['zero'],r['one'],r['many']),(1,1,1))
        self.assertEqual(r['last_hits'],2)
    def test_checksum_and_fragment(self):
        d=Decoder(); self.assertEqual(d.feed(b'BA1,8,'+self.data),self.rows)
        self.assertEqual(d.errors,1)
        with self.assertRaises(ValueError): Decoder().feed(self.data.replace(b'BA1,',b'BA1,9',1),True)
        d=Decoder(); d.feed(self.data+b'BA1,1')
        with self.assertRaises(ValueError): d.finish()
    def test_impossible_offsets_and_conservation(self):
        for changes in ({'last_first':0},{'assigned':0},{'windows':99},{'last_hits':9000},
                        {'period':30000},{'last_last':30000},{'a_phase':3}):
            row=dict(self.rows[-1],**changes)
            with self.assertRaises(ValueError): Decoder().feed(frame(row),True)
    def test_session(self):
        s=Session('replay')
        for r in self.rows: s.accept(r)
        self.assertFalse(s.accept(self.rows[-1]))
        with self.assertRaises(ValueError): s.accept(dict(self.rows[-1],levels=self.rows[-1]['levels']^1))
        s=Session('replay'); s.accept(self.rows[-1])
        with self.assertRaises(ValueError): s.accept(self.rows[0])
    def test_retained_result_no_new_window(self):
        old=self.rows[-1]; s=Session('replay'); s.accept(old)
        r=dict(old,now=old['now']+1,last_first=old['last_first']+1)
        with self.assertRaises(ValueError): s.accept(r)
    def test_sparse_impossibilities(self):
        # Individually valid endpoints still need transition checks.
        base={k:0 for k in FIELDS}
        base.update(now=100,state=1,valid=15,a_phase=2,a_age=20,
                    b_phase=2,b_age=20)
        cases=[]
        for side in ('a','b'):
            after=dict(base,now=101,a_age=21,b_age=21)
            after[side+'_phase']=1
            cases.append((base,after))
        after=dict(base,now=110,a_cycles=2,windows=2,zero=2,period=10,
                   a_phase=1,a_age=0,b_age=30)
        cases.append((base,after))
        pending=dict(base,a_age=90,b_age=5,b_cycles=2,assigned=2,
                     pending=2,first=50,last=80)
        after=dict(pending,now=101,a_age=91,b_age=6,last=81)
        cases.append((pending,after))
        for old,new in cases:
            validate(old); validate(new)
            s=Session('replay'); s.accept(old)
            with self.assertRaises(ValueError): s.accept(new)
    def test_files(self):
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'fifo'; os.mkfifo(p)
            with self.assertRaises(ValueError): load_replay(p)
            p=Path(d)/'empty'; p.write_bytes(b'')
            with self.assertRaises(ValueError): load_replay(p)
            p.write_bytes(self.data+b'BA1,')
            with self.assertRaises(ValueError): load_replay(p)
        with self.assertRaises((OSError,termios.error)): Serial('/dev/null')
    def test_gui_replay_export_serial(self):
        app=Workbench(); app.update()
        try:
            app.replay(ROOT/'firmware/build/demo.ba1'); app.update()
            self.assertIn('many',app.summary.get())
            self.assertEqual(len(app.history.get_children()),len(self.rows))
            with tempfile.TemporaryDirectory() as d:
                p=Path(d)/'export.csv'; app.export(p)
                self.assertEqual(p.stat().st_mode&0o777,0o600)
                with p.open() as handle: rows=list(csv.reader(handle))
                self.assertIn('jayis1',rows[0]); self.assertIn('replay',rows[0])
                with self.assertRaises(FileExistsError): app.export(p)
            master,slave=pty.openpty(); saved=termios.tcgetattr(slave)
            try:
                app.connect(os.ttyname(slave)); app.update()
                self.assertEqual(app.summary.get(),'No evidence')
                with self.assertRaises(ValueError): app.connect(os.ttyname(slave))
                os.write(master,self.data); time.sleep(.02); app.poll_once(); app.update()
                self.assertEqual(app.session.rows[-1]['many'],1)
                self.assertEqual(len(app.history.get_children()),len(self.rows))
                app.disconnect(); self.assertEqual(termios.tcgetattr(slave),saved)
                app.connect(os.ttyname(slave)); os.close(master); master=None
                app.poll_once(); self.assertIsNone(app.port)
            finally:
                if master is not None: os.close(master)
                os.close(slave)
        finally: app.close()
if __name__=='__main__': unittest.main()
