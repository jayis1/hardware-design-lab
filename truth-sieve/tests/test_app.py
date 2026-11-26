# Author: jayis1. SPDX-License-Identifier: MIT
import binascii
import os
from pathlib import Path
import pty
import sys
import tempfile
import time
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'app'))
from model import project, candidates, row_outputs, suggest, validate, Session, transition
from transport import Decoder, Serial, load_replay
FIXTURE=Path(__file__).resolve().parents[1]/'firmware/build/demo.ts1'

def encode(row):
    from model import FIELDS
    body=('TS1,'+','.join(str(row[f]) for f in FIELDS)).encode()
    return body+f'*{binascii.crc_hqx(body,65535):04X}\n'.encode()

class ModelTests(unittest.TestCase):
    def test_exhaustive_relations(self):
        for settings in range(65536):
            coverage=project(settings)
            count=1
            for i in range(4):
                expected=[o for o in range(4) if settings & (1<<(i+4*o))]
                self.assertEqual(row_outputs(coverage,i),expected)
                if not expected: count*=4
                elif len(expected)>1: count=0
            self.assertEqual(candidates(coverage),count)
    def test_real_encoder(self):
        session=load_replay(FIXTURE)
        self.assertEqual(session.total,55)
        final=session.rows[-1]
        self.assertEqual(candidates(final['coverage']),0)
        self.assertEqual(row_outputs(final['coverage'],0),[0,1])
        self.assertEqual(suggest(final['coverage'],final['raw']),4)
        deterministic=next(r for r in session.rows if r['visits']==4)
        self.assertEqual(candidates(deterministic['coverage']),1)
    def test_crc_fragment_and_tail(self):
        raw=FIXTURE.read_bytes().splitlines(keepends=True)[0]
        d=Decoder(); self.assertEqual(len(d.feed(raw[:13]+raw)),1)
        self.assertEqual(d.errors,1)
        self.assertEqual(d.feed(raw.replace(b'TS1,',b'TS1,9',1)),[])
        d.feed(raw[:8])
        with self.assertRaises(ValueError): d.finish()
    def test_impossible_valid_crc(self):
        row=load_replay(FIXTURE).rows[-1]
        for key,value in [('coverage',0),('suggestion',0),('raw',16),('hold',101),('visits',0)]:
            wrong=dict(row); wrong[key]=value
            with self.assertRaises(ValueError): Decoder().feed(encode(wrong),strict=True)
    def test_rollback_and_disappearing_evidence(self):
        rows=load_replay(FIXTURE).rows
        a=dict(rows[-1]); b=dict(a); b['now']+=1; b['samples']+=1; b['qualified_ms']+=1
        transition(a,b)
        b['settings']=0; b['coverage']=0
        with self.assertRaises(ValueError): transition(a,b)
        b=dict(a); b['now']-=1
        with self.assertRaises(ValueError): transition(a,b)
    def test_replay_bounds_and_special_files(self):
        with tempfile.TemporaryDirectory() as tmp:
            fifo=Path(tmp)/'fifo'; os.mkfifo(fifo)
            with self.assertRaises(ValueError): load_replay(fifo)
            empty=Path(tmp)/'empty'; empty.write_bytes(b'')
            with self.assertRaises(ValueError): load_replay(empty)
            empty.write_bytes(FIXTURE.read_bytes()+b'TS1')
            with self.assertRaises(ValueError): load_replay(empty)
    def test_serial_restore(self):
        import termios
        master,slave=pty.openpty()
        try:
            old=termios.tcgetattr(slave)
            port=Serial(os.ttyname(slave)); os.write(master,b'hello')
            time.sleep(.02); self.assertEqual(port.read(),b'hello')
            port.close(); self.assertEqual(termios.tcgetattr(slave),old)
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
        self.assertIn('0 deterministic',self.app.summary.get())
        rows=[self.app.table.item(i)['values'] for i in self.app.table.get_children()]
        self.assertEqual(len(rows),4)
        self.assertIn('CONFLICT',str(rows[0]))
        with tempfile.TemporaryDirectory() as tmp:
            path=Path(tmp)/'evidence.csv'; self.app.export(path)
            self.assertEqual(path.stat().st_mode & 0o777,0o600)
            self.assertIn('author,jayis1',path.read_text())
            self.assertIn('replay',path.read_text())
            with self.assertRaises(FileExistsError): self.app.export(path)
    def test_live_and_reconnect(self):
        master,slave=pty.openpty()
        try:
            self.app.replay(FIXTURE)
            self.app.connect(os.ttyname(slave))
            self.assertEqual(self.app.summary.get(),'No acquired settings')
            with self.assertRaises(ValueError): self.app.connect(os.ttyname(slave))
            data=FIXTURE.read_bytes().splitlines(keepends=True)
            os.write(master,b''.join(data[:5])); self.app.poll()
            self.assertEqual(len(self.app.session.rows),5)
            self.app.last_progress=time.monotonic()-3
            self.app.poll(); self.assertIsNone(self.app.serial)
            self.assertIn('stale',self.app.warning)
        finally: os.close(master); os.close(slave)
    def test_failed_open_preserves_evidence(self):
        import termios
        self.app.replay(FIXTURE); old=self.app.session
        with self.assertRaises((OSError,termios.error)): self.app.connect('/dev/null')
        self.assertIs(self.app.session,old)

if __name__=='__main__': unittest.main()
