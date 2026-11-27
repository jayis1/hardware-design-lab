# Author: jayis1. SPDX-License-Identifier: MIT
import binascii
import os
from pathlib import Path
import pty
import subprocess
import sys
import tempfile
import time
import unittest
from unittest.mock import patch
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'app'))
from model import FIELDS, SPLITS, missing, parents, complete, missing_total, suggest, Session, transition, validate
from transport import Decoder, Serial, load_replay
FIXTURE=Path(__file__).resolve().parents[1]/'firmware/build/demo.mg1'

def encode(row):
    body=('MG1,'+','.join(str(row[f]) for f in FIELDS)).encode()
    return body+f'*{binascii.crc_hqx(body,65535):04X}\n'.encode()

def summaries(row):
    row.update(coverage=row['settings'],complete=complete(row['settings']),
               missing_total=missing_total(row['settings']),
               suggestion=suggest(row['settings'],row['raw']))
    return row

class ModelTests(unittest.TestCase):
    def test_all_histories(self):
        # Pair-splice oracle, different from host's projection-membership model.
        for h in range(65536):
            words=[w for w in range(16) if h&(1<<w)]
            expected=[]
            for s in SPLITS:
                product={(a&s)|(b&(15^s)) for a in words for b in words}
                holes=product-set(words)
                mask=sum(1<<w for w in holes)
                self.assertEqual(missing(h,s),mask)
                expected.append(mask)
                if holes:
                    w=min(holes); pair=parents(h,s,w)
                    self.assertIsNotNone(pair)
                    a,b=pair
                    self.assertIn(a,words); self.assertIn(b,words)
                    self.assertEqual((a&s)|(b&(15^s)),w)
            self.assertEqual(complete(h),sum(1<<i for i,m in enumerate(expected) if h and not m))
            self.assertEqual(missing_total(h),sum(m.bit_count() for m in expected))
    def test_nonmonotonic_completeness(self):
        h=1
        self.assertEqual(complete(0),0)
        self.assertEqual(complete(h),127)
        h|=1<<15
        self.assertEqual(missing(h,3),(1<<3)|(1<<12))
        self.assertEqual(complete(h),0)
        h|=(1<<3)|(1<<12)
        self.assertEqual(missing(h,3),0)
        self.assertTrue(complete(h)&2)
        h|=1<<1
        self.assertEqual(missing(h,3),1<<13)
        self.assertEqual(parents(h,3,13),(1,12))
        h|=1<<13
        self.assertEqual(missing(h,3),0)
        self.assertIsNone(parents(h,3,13))
        self.assertEqual(missing(65535,3),0)
        self.assertEqual(suggest(65535,0),16)
    def test_real_encoder(self):
        session=load_replay(FIXTURE)
        self.assertEqual(session.total,48)
        states={r['visits']:r for r in session.rows}
        self.assertEqual(missing(states[2]['settings'],3),(1<<3)|(1<<12))
        self.assertEqual(missing(states[4]['settings'],3),0)
        self.assertEqual(missing(states[5]['settings'],3),1<<13)
        self.assertEqual(missing(states[6]['settings'],3),0)
        data=subprocess.check_output([str(FIXTURE.parent/'fixture'),'--dense'])
        decoder=Decoder(); dense=Session('synthetic dense')
        for offset in range(0,len(data),4096):
            for r in decoder.feed(data[offset:offset+4096],strict=True): dense.accept(r)
        decoder.finish()
        self.assertEqual(dense.total,960)
        self.assertEqual(dense.rows[-1],session.rows[-1])
        for i,a in enumerate(session.rows):
            for b in session.rows[i+1:]: transition(a,b)
    def test_bad_frames(self):
        row=load_replay(FIXTURE).rows[-1]
        for key,value in [('coverage',0),('suggestion',16),('raw',16),('hold',101),('visits',0),('complete',128),('missing_total',999)]:
            wrong=dict(row); wrong[key]=value
            with self.subTest(key=key), self.assertRaises(ValueError): Decoder().feed(encode(wrong),strict=True)
        raw=encode(row); decoder=Decoder()
        self.assertEqual(len(decoder.feed(raw[:10]+raw)),1)
        self.assertEqual(decoder.errors,1)
        self.assertEqual(decoder.feed(raw.replace(b'MG1,',b'MG1,9',1)),[])
        decoder.feed(raw[:8])
        with self.assertRaises(ValueError): decoder.finish()
    def test_exact_visit(self):
        a=dict(load_replay(FIXTURE).rows[5])
        a.update(now=460,samples=460,raw=1,hold=99,credited=0,visits=2,
                 settings=3,unknown_ms=300,qualified_ms=159)
        summaries(a); validate(a)
        b=dict(a); b.update(now=461,samples=461,hold=100,credited=1,visits=3,unknown_ms=301)
        summaries(b); validate(b); transition(a,b)
        b['settings']=7; summaries(b); validate(b)
        with self.assertRaises(ValueError): transition(a,b)
    def test_replay_bounds(self):
        with tempfile.TemporaryDirectory() as tmp:
            path=Path(tmp)/'fifo'; os.mkfifo(path)
            with self.assertRaises(ValueError): load_replay(path)
            path=Path(tmp)/'file'
            for data in (b'',FIXTURE.read_bytes()+b'MG1',b'X'*(2*1024*1024+1)):
                path.write_bytes(data)
                with self.assertRaises(ValueError): load_replay(path)
    def test_serial_restore(self):
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
    def test_replay_screens_export(self):
        self.app.replay(FIXTURE); self.root.update()
        self.assertIn('split-specific gaps',self.app.summary.get())
        rows=[self.app.table.item(i)['values'] for i in self.app.table.get_children()]
        self.assertEqual(len(rows),7)
        self.assertIn('complete in retained set',str(rows[1]))
        self.assertIn('left from observed',self.app.details.get('1.0','end'))
        with tempfile.TemporaryDirectory() as tmp:
            path=Path(tmp)/'evidence.csv'; self.app.export(path)
            self.assertEqual(path.stat().st_mode&0o777,0o600)
            self.assertIn('author,jayis1',path.read_text())
            self.assertIn('replay',path.read_text())
            self.assertIn('complete,missing_total',path.read_text())
            with self.assertRaises(FileExistsError): self.app.export(path)
    def test_empty_and_unobserved(self):
        self.app.render(); self.assertEqual(self.app.summary.get(),'No acquired settings')
        row=load_replay(FIXTURE).rows[0]
        self.app.session=Session('synthetic'); self.app.session.accept(row); self.app.render()
        rows=[self.app.table.item(i)['values'] for i in self.app.table.get_children()]
        self.assertTrue(all('UNOBSERVED' in str(r) for r in rows))
    def test_live_reconnect_hangup_stale(self):
        master,slave=pty.openpty()
        try:
            self.app.replay(FIXTURE); self.app.connect(os.ttyname(slave))
            self.assertEqual(self.app.summary.get(),'No acquired settings')
            with self.assertRaises(ValueError): self.app.connect(os.ttyname(slave))
            data=FIXTURE.read_bytes().splitlines(keepends=True)
            os.write(master,b''.join(data[:16])); self.app.poll(); self.root.update()
            self.assertEqual(len(self.app.session.rows),16)
            self.assertIn('3, 12',str(self.app.table.item(self.app.table.get_children()[1])['values']))
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
