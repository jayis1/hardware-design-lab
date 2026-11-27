# Author: jayis1. SPDX-License-Identifier: MIT
import binascii
import os
from pathlib import Path
import pty
import sys
import tempfile
import time
import unittest
from unittest.mock import patch
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'app'))
from model import FIELDS, closure, support, minimal_for, witness, supported, rules, suggest, Session, transition
from transport import Decoder, Serial, load_replay
FIXTURE=Path(__file__).resolve().parents[1]/'firmware/build/demo.cb1'

def encode(row):
    body=('CB1,'+','.join(str(row[f]) for f in FIELDS)).encode()
    return body+f'*{binascii.crc_hqx(body,65535):04X}\n'.encode()

class ModelTests(unittest.TestCase):
    def test_all_contexts(self):
        for settings in range(65536):
            words=[w for w in range(16) if settings&(1<<w)]
            closures=[]
            for a in range(16):
                matching=[w for w in words if all(not(a&(1<<b)) or w&(1<<b) for b in range(4))]
                expected=sum(1<<b for b in range(4) if all(w&(1<<b) for w in matching)) if matching else 16
                self.assertEqual(closure(settings,a),expected)
                self.assertEqual(support(settings,a),len(matching))
                closures.append(expected)
                for bit in range(4):
                    bad=[w for w in matching if not w&(1<<bit)]
                    self.assertEqual(witness(settings,a,bit),bad[0] if bad else None)
            self.assertEqual(supported(settings),sum(1<<a for a,c in enumerate(closures) if c!=16))
            total=0
            for bit in range(4):
                candidates=[a for a in range(16) if not a&(1<<bit) and closures[a]!=16 and closures[a]&(1<<bit)]
                minimal=[a for a in candidates if not any(b!=a and b&a==b for b in candidates)]
                self.assertEqual(minimal_for(settings,bit),sum(1<<a for a in minimal))
                total+=len(minimal)
            self.assertEqual(rules(settings),total)
    def test_encoder(self):
        session=load_replay(FIXTURE)
        self.assertEqual(session.total,48)
        before=next(r for r in session.rows if r['visits']==5)
        after=session.rows[-1]
        self.assertEqual(closure(before['settings'],1),3)
        self.assertTrue(minimal_for(before['settings'],1)&(1<<1))
        self.assertEqual(closure(after['settings'],1),1)
        self.assertEqual(witness(after['settings'],1,1),1)
        self.assertEqual(closure(after['settings'],8),11)
        self.assertEqual(suggest(65535,0),16)
    def test_bad_frames(self):
        row=load_replay(FIXTURE).rows[-1]
        for key,value in [('coverage',0),('suggestion',16),('raw',16),('hold',101),('visits',0),('supported',0),('rules',99)]:
            wrong=dict(row); wrong[key]=value
            with self.subTest(key=key), self.assertRaises(ValueError): Decoder().feed(encode(wrong),strict=True)
        raw=encode(row); d=Decoder()
        self.assertEqual(len(d.feed(raw[:10]+raw)),1)
        self.assertEqual(d.errors,1)
        self.assertEqual(d.feed(raw.replace(b'CB1,',b'CB1,9',1)),[])
        d.feed(raw[:8])
        with self.assertRaises(ValueError): d.finish()
    def test_dense_real_encoder(self):
        import subprocess
        data=subprocess.check_output([str(FIXTURE.parent/'fixture'),'--dense'])
        decoder=Decoder(); session=Session('synthetic dense')
        for start in range(0,len(data),4096):
            for row in decoder.feed(data[start:start+4096],strict=True): session.accept(row)
        decoder.finish()
        self.assertEqual(session.total,960)
        self.assertEqual(session.rows[-1],load_replay(FIXTURE).rows[-1])
    def test_transitions(self):
        rows=load_replay(FIXTURE).rows
        for i,a in enumerate(rows):
            for b in rows[i+1:]: transition(a,b)
        a=rows[-1]; b=dict(a); b['now']+=1; b['samples']+=1; b['qualified_ms']+=1
        transition(a,b)
        b['settings']=0; b['coverage']=0
        with self.assertRaises(ValueError): transition(a,b)
        b=dict(a); b['now']-=1
        with self.assertRaises(ValueError): transition(a,b)
    def test_replay_bounds(self):
        with tempfile.TemporaryDirectory() as tmp:
            path=Path(tmp)/'fifo'; os.mkfifo(path)
            with self.assertRaises(ValueError): load_replay(path)
            path=Path(tmp)/'file'
            for data in (b'',FIXTURE.read_bytes()+b'CB1',b'X'*(2*1024*1024+1)):
                path.write_bytes(data)
                with self.assertRaises(ValueError): load_replay(path)
    def test_single_visit_cannot_credit_another_word(self):
        # A fully consistent prequalification endpoint with earlier word zero.
        a=dict(load_replay(FIXTURE).rows[5])
        a.update(now=260,samples=260,raw=1,hold=99,credited=0,visits=1,
                 settings=1,coverage=1,unknown_ms=200,qualified_ms=59,
                 supported=1,rules=0,suggestion=1)
        from model import validate
        validate(a)
        b=dict(a)
        b.update(now=261,samples=261,hold=100,credited=1,visits=2,
                 settings=3,coverage=3,unknown_ms=201,supported=3,suggestion=3)
        validate(b); transition(a,b)
        # Both old and current words were already retained; spend the new visit
        # on an unrelated third word while leaving the current raw word valid.
        a.update(settings=3,coverage=3,visits=2,unknown_ms=300,supported=3,
                 suggestion=3,samples=460,now=460,qualified_ms=159)
        b=dict(a)
        b.update(now=461,samples=461,hold=100,credited=1,visits=3,
                 settings=7,coverage=7,unknown_ms=301,supported=7,suggestion=3)
        validate(a); validate(b)
        with self.assertRaises(ValueError): transition(a,b)
    def test_serial_restore_and_failure(self):
        import termios
        master,slave=pty.openpty()
        try:
            old=termios.tcgetattr(slave)
            with patch('transport.termios.tcflush',side_effect=termios.error('failure')):
                with self.assertRaises(termios.error): Serial(os.ttyname(slave))
            self.assertEqual(termios.tcgetattr(slave),old)
            port=Serial(os.ttyname(slave)); os.write(master,b'hello'); time.sleep(.02)
            self.assertEqual(port.read(),b'hello'); port.close()
            self.assertEqual(termios.tcgetattr(slave),old)
        finally: os.close(master); os.close(slave)

class GuiTests(unittest.TestCase):
    def setUp(self):
        import tkinter as tk
        from main import App
        self.root=tk.Tk(); self.root.withdraw(); self.app=App(self.root)
    def tearDown(self):
        self.app.close(); self.root.destroy()
    def test_replay_screens_and_export(self):
        self.app.replay(FIXTURE); self.root.update()
        self.assertIn('provisional',self.app.summary.get())
        rows=[self.app.table.item(i)['values'] for i in self.app.table.get_children()]
        self.assertEqual(len(rows),16)
        self.assertIn('B:1',str(rows[1]))
        self.assertIn('supported',self.app.details.get('1.0','end'))
        with tempfile.TemporaryDirectory() as tmp:
            path=Path(tmp)/'evidence.csv'; self.app.export(path)
            self.assertEqual(path.stat().st_mode&0o777,0o600)
            self.assertIn('author,jayis1',path.read_text())
            self.assertIn('replay',path.read_text())
            self.assertIn('supported,rules',path.read_text())
            with self.assertRaises(FileExistsError): self.app.export(path)
    def test_unsupported_and_empty(self):
        self.app.render(); self.assertEqual(self.app.summary.get(),'No acquired settings')
        row=next(r for r in load_replay(FIXTURE).rows if r['visits']==1)
        self.app.session=Session('synthetic'); self.app.session.accept(row); self.app.render()
        rows=[self.app.table.item(i)['values'] for i in self.app.table.get_children()]
        self.assertIn('UNOBSERVED',str(rows[1]))
    def test_live_reconnect_hangup_stale(self):
        master,slave=pty.openpty()
        try:
            self.app.replay(FIXTURE); self.app.connect(os.ttyname(slave))
            self.assertEqual(self.app.summary.get(),'No acquired settings')
            with self.assertRaises(ValueError): self.app.connect(os.ttyname(slave))
            data=FIXTURE.read_bytes().splitlines(keepends=True)
            os.write(master,b''.join(data[:5])); self.app.poll()
            self.assertEqual(len(self.app.session.rows),5)
            self.app.last_progress=time.monotonic()-3; self.app.poll()
            self.assertIsNone(self.app.serial); self.assertIn('stale',self.app.warning)
            self.app.connect(os.ttyname(slave)); self.assertFalse(self.app.session.rows)
            os.close(master); master=None; self.app.poll()
            self.assertIsNone(self.app.serial); self.assertIn('disconnect',self.app.warning)
        finally:
            if master is not None: os.close(master)
            os.close(slave)
    def test_failed_open(self):
        import termios
        self.app.replay(FIXTURE); old=self.app.session
        with self.assertRaises((OSError,termios.error)): self.app.connect('/dev/null')
        self.assertIs(self.app.session,old)
        with patch('transport.os.open',side_effect=PermissionError('denied')):
            with self.assertRaises(PermissionError): self.app.connect('/dev/ttyACM0')
        self.assertIs(self.app.session,old)

if __name__=='__main__': unittest.main()
