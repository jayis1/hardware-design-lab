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
from model import FIELDS, components, bridges, labels, shortest, Session, transition, validate
from transport import Decoder, Serial, load_replay
FIXTURE=Path(__file__).resolve().parents[1]/'firmware/build/demo.si1'

def encode(row):
    body=('SI1,'+','.join(str(row[f]) for f in FIELDS)).encode()
    return body+f'*{binascii.crc_hqx(body,65535):04X}\n'.encode()

class ModelTests(unittest.TestCase):
    def test_all_sets(self):
        # Union-find is independent of the flood fill used in the app.
        for history in range(65536):
            roots=list(range(16))
            def root(a):
                while roots[a]!=a: a=roots[a]
                return a
            words=[w for w in range(16) if history&(1<<w)]
            for a in words:
                for b in words:
                    if (a^b).bit_count()==1: roots[root(a)]=root(b)
            expected=len({root(a) for a in words})
            self.assertEqual(components(history),expected)
            expected_bridges=sum(1<<w for w in range(16) if w not in words and
                len({root(a) for a in words if (a^w).bit_count()==1})>=2)
            self.assertEqual(bridges(history),expected_bridges)
    def test_paths(self):
        self.assertEqual(shortest(0,0,0),[])
        self.assertEqual(shortest(11,0,3),[0,1,3])
        self.assertEqual(shortest(9,0,3),[])
        self.assertEqual(shortest(65535,0,15),[0,1,3,7,15])
        self.assertEqual(shortest(1,0,0),[0])
        with self.assertRaises(ValueError): shortest(1,-1,0)
        self.assertEqual(labels(9),{0:0,3:3})
        self.assertEqual(bridges(9),6)
        self.assertEqual(bridges(11),0)
    def test_real_encoder(self):
        session=load_replay(FIXTURE)
        self.assertEqual(session.total,48)
        data=subprocess.check_output([str(FIXTURE.parent/'fixture'),'--dense'])
        decoder=Decoder(); dense=Session('synthetic dense')
        for offset in range(0,len(data),4096):
            for row in decoder.feed(data[offset:offset+4096],strict=True): dense.accept(row)
        decoder.finish()
        self.assertEqual(dense.total,960)
        self.assertEqual(dense.rows[-1],session.rows[-1])
        for i,a in enumerate(session.rows):
            for b in session.rows[i+1:]: transition(a,b)
    def test_crc_valid_false_summary(self):
        row=load_replay(FIXTURE).rows[-1]
        for key,value in [('components',99),('bridges',65535),('coverage',0),('raw',16),('hold',101)]:
            wrong=dict(row); wrong[key]=value
            with self.subTest(key=key),self.assertRaises(ValueError):
                Decoder().feed(encode(wrong),strict=True)
        data=encode(row); decoder=Decoder()
        self.assertEqual(len(decoder.feed(data[:10]+data)),1)
        self.assertEqual(decoder.errors,1)
        decoder.feed(data[:8])
        with self.assertRaises(ValueError): decoder.finish()
    def test_sparse_freeze(self):
        a=load_replay(FIXTURE).rows[-1]; b=dict(a); b['now']+=100
        with self.assertRaises(ValueError): transition(a,b)
        b['fault']=4; transition(a,b)
        b['settings']^=1
        with self.assertRaises(ValueError): transition(a,b)
    def test_replay_bounds(self):
        with tempfile.TemporaryDirectory() as tmp:
            path=Path(tmp)/'fifo'; os.mkfifo(path)
            with self.assertRaises(ValueError): load_replay(path)
            path=Path(tmp)/'file'
            for data in (b'',FIXTURE.read_bytes()+b'SI1',b'x'*(2*1024*1024+1)):
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
    def test_screens_path_export(self):
        self.app.replay(FIXTURE); self.root.update()
        self.assertIn('islands',self.app.summary.get())
        self.assertEqual(len(self.app.table.get_children()),16)
        self.app.start.set('0'); self.app.end.set('3'); self.app.show_path()
        self.assertIn('0 -> 1 -> 3',self.app.path_result.get())
        self.app.end.set('2'); self.app.show_path()
        self.assertIn('No retained',self.app.path_result.get())
        self.assertIn('not witnessed motion',self.app.details.get('1.0','end'))
        with tempfile.TemporaryDirectory() as tmp:
            path=Path(tmp)/'evidence.csv'; self.app.export(path)
            self.assertEqual(path.stat().st_mode&0o777,0o600)
            self.assertIn('author,jayis1',path.read_text())
            self.assertIn('components,bridges',path.read_text())
            with self.assertRaises(FileExistsError): self.app.export(path)
    def test_live_reconnect_hangup_stale(self):
        master,slave=pty.openpty()
        try:
            self.app.replay(FIXTURE); self.app.connect(os.ttyname(slave))
            self.assertEqual(self.app.summary.get(),'No acquired settings')
            with self.assertRaises(ValueError): self.app.connect(os.ttyname(slave))
            data=FIXTURE.read_bytes().splitlines(keepends=True)
            os.write(master,b''.join(data[:16])); self.app.poll(); self.root.update()
            self.assertEqual(len(self.app.session.rows),16)
            self.assertIn('2 islands',self.app.summary.get())
            self.app.last_progress=time.monotonic()-3; self.app.poll()
            self.assertIsNone(self.app.serial); self.assertIn('stale',self.app.warning)
            self.app.connect(os.ttyname(slave)); self.assertFalse(self.app.session.rows)
            os.close(master); master=None; self.app.poll()
            self.assertIsNone(self.app.serial); self.assertIn('disconnect',self.app.warning)
        finally:
            if master is not None: os.close(master)
            os.close(slave)
    def test_failed_open_preserves(self):
        import termios
        self.app.replay(FIXTURE); old=self.app.session
        with self.assertRaises((OSError,termios.error)): self.app.connect('/dev/null')
        self.assertIs(self.app.session,old)
    def test_disconnect_accounts_partial_tail_once(self):
        master,slave=pty.openpty()
        try:
            self.app.connect(os.ttyname(slave))
            os.write(master,b'SI1,123')
            self.app.poll()
            self.app.disconnect()
            self.assertEqual(self.app.decoder.errors,1)
            self.assertIn('partial',self.app.warning)
            self.app.disconnect()
            self.assertEqual(self.app.decoder.errors,1)
        finally:
            os.close(master); os.close(slave)

if __name__=='__main__': unittest.main()
