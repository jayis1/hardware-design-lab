# Author: jayis1. SPDX-License-Identifier: MIT
import binascii
import os
from pathlib import Path
import pty
import sys
import tempfile
import time
import tkinter as tk
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'app'))
from model import FIELDS, Session, conflict, validate
from transport import Decoder, Serial, load_replay
from workbench import Workbench
ROOT=Path(__file__).resolve().parents[1]

def frame(r):
    body=('BO1,'+','.join(str(r[k]) for k in FIELDS)).encode()
    return body+f'*{binascii.crc_hqx(body,65535):04X}\n'.encode()

class Tests(unittest.TestCase):
    def setUp(self):
        self.data=(ROOT/'firmware/build/demo.bo1').read_bytes()
        self.rows=Decoder().feed(self.data,strict=True)
    def test_oracle(self):
        for mask in range(512):
            possible=False
            for a in range(7):
                for b in range(7):
                    for c in range(7):
                        ds=(a-b,a-c,b-c)
                        if all(not mask&(1<<(3*p+r)) or
                               (abs(ds[p])<=2 if r==0 else ds[p]>2 if r==1 else ds[p]<-2)
                               for p in range(3) for r in range(3)):
                            possible=True
            self.assertEqual(conflict(mask),not possible,mask)
    def test_fixture_and_replay(self):
        s=load_replay(ROOT/'firmware/build/demo.bo1')
        self.assertEqual(s.rows[-1]['accepted'],4)
        self.assertEqual(s.rows[-1]['rejected'],1)
        self.assertEqual(s.rows[-1]['conflict'],1)
        self.assertTrue(any(r['accepted']==3 and not r['conflict'] for r in s.rows))
    def test_corrupt_and_truncated(self):
        d=Decoder()
        self.assertTrue(d.feed(b'BO1,fragment'+self.data))
        self.assertGreater(d.errors,0)
        with self.assertRaises(ValueError): Decoder().feed(self.data.replace(b'BO1,',b'BO1,9',1),strict=True)
        d=Decoder(); d.feed(self.data[:-1])
        with self.assertRaises(ValueError): d.finish()
    def test_invalid_evidence(self):
        r=dict(self.rows[-1]); r['c0']+=1
        with self.assertRaises(ValueError): Decoder().feed(frame(r),strict=True)
        r=dict(self.rows[-1]); r['conflict']=0
        with self.assertRaises(ValueError): validate(r)
    def test_session(self):
        s=Session('live')
        for r in self.rows: s.accept(r,host=r['now']/1000)
        self.assertTrue(s.fresh(self.rows[-1]['now']/1000))
        self.assertFalse(s.fresh(999))
        before=s.progress; s.accept(self.rows[-1],host=999)
        self.assertEqual(before,s.progress)
        r=dict(self.rows[-1]); r['now']+=100
        with self.assertRaises(ValueError): s.accept(r,host=999)
    def test_migration(self):
        old=dict(self.rows[-1]); new=dict(old); new['now']+=100
        new['c0']-=1; new['c3']+=1
        new['evidence']=sum(1<<i for i in range(9) if new['c'+str(i)])
        new['conflict']=conflict(new['evidence'])
        s=Session('replay'); s.accept(old)
        with self.assertRaises(ValueError): s.accept(new)
    def test_settlement_cannot_change_outcome(self):
        old={k:0 for k in FIELDS}
        old.update(now=1000,state=2,levels=1,valid=15,starts=1,selected=1,age=499,dwell=499)
        new=dict(old)
        new.update(now=1001,state=3,levels=5,accepted=1,c1=1,evidence=2,outcome=1,age=500,dwell=500)
        s=Session('replay'); s.accept(old)
        with self.assertRaises(ValueError): s.accept(new)
        new=dict(old); new.update(now=1001,levels=5,outcome=1,age=500,dwell=499)
        s=Session('replay'); s.accept(old)
        with self.assertRaises(ValueError): s.accept(new)
    def test_abort_requires_halt(self):
        row={k:0 for k in FIELDS}
        row.update(state=3,starts=1,aborted=1)
        with self.assertRaises(ValueError): validate(row)
    def test_export_and_file_guards(self):
        s=load_replay(ROOT/'firmware/build/demo.bo1')
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'out.csv'; s.export(p,'=formula')
            self.assertEqual(p.stat().st_mode & 0o777,0o600)
            self.assertIn('jayis1',p.read_text())
            with self.assertRaises(FileExistsError): s.export(p,'again')
            fifo=Path(d)/'fifo'; os.mkfifo(fifo)
            with self.assertRaises(ValueError): load_replay(fifo)
        with self.assertRaises(Exception): Serial('/dev/null')
    def test_gui_pty(self):
        root=tk.Tk(); app=Workbench(root)
        master,slave=pty.openpty()
        try:
            app.path.set(os.ttyname(slave)); app.connect()
            original=app.port; app.connect(); self.assertIs(app.port,original)
            # Exercise the real event loop, using synthetic C-encoder packets.
            start=time.monotonic()
            for i,r in enumerate(self.rows):
                target=start+i*0.1
                while time.monotonic()<target: root.update(); time.sleep(.005)
                os.write(master,frame(r))
                until=time.monotonic()+.04
                while time.monotonic()<until: root.update(); time.sleep(.005)
            self.assertEqual(app.session.rows[-1]['accepted'],4)
            self.assertIn('INCOMPATIBLE',app.summary.get())
            self.assertEqual(len(app.table.get_children()),3)
            app.disconnect(); self.assertFalse(app.session.fresh())
            with tempfile.TemporaryDirectory() as d:
                app.export_to(Path(d)/'ui.csv')
                self.assertIn('jayis1',(Path(d)/'ui.csv').read_text())
            app.replay_path(ROOT/'firmware/build/demo.bo1')
            self.assertEqual(app.session.provenance,'replay')
            app.connect(); self.assertEqual(app.summary.get(),'No samples')
            os.close(master); master=None
            until=time.monotonic()+.1
            while time.monotonic()<until: root.update(); time.sleep(.005)
            self.assertIsNone(app.port)
        finally:
            if master is not None: os.close(master)
            os.close(slave); app.close()

if __name__=='__main__': unittest.main()
