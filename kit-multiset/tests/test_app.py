# Author: jayis1. SPDX-License-Identifier: MIT
import binascii
import os
from pathlib import Path
import pty
import subprocess
import sys
import tempfile
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'app'))
from model import FIELDS, Session, validate
from transport import Decoder, Serial, load_replay

def frame(row):
    body=('KM1,'+','.join(str(row[k]) for k in FIELDS)).encode()
    return body+b'*'+f'{binascii.crc_hqx(body,65535):04X}\n'.encode()

def idle(now=10):
    r=dict.fromkeys(FIELDS,0); r.update(now=now,state=1,valid=15)
    return r

class Tests(unittest.TestCase):
    def test_crc_bounds_and_resync(self):
        f=frame(idle()); d=Decoder()
        self.assertEqual(d.feed(f[:12]),[])
        self.assertEqual(d.feed(f)[0],idle())
        self.assertGreater(d.errors,0)
        with self.assertRaises(ValueError): Decoder().feed(f[:-4]+b'000\n',True)
        d=Decoder(); d.feed(f[:-1])
        with self.assertRaises(ValueError): d.finish()
    def test_invariants(self):
        for changes in [dict(starts=1),dict(state=2),dict(good=1),dict(valid=16),dict(last_result=1),dict(faults=1)]:
            r=idle(); r.update(changes)
            with self.assertRaises(ValueError): validate(r)
    def test_duplicate_and_rollback(self):
        s=Session('live'); s.accept(idle(),0); s.accept(idle(),1)
        self.assertFalse(s.fresh(1))
        s.accept(idle(110),1.1); self.assertTrue(s.fresh(1.1))
        r=idle(110); r['gaps']=1
        with self.assertRaises(ValueError): s.accept(r,1.2)
        self.assertFalse(s.fresh(1.2))
    def test_stale_pause(self):
        s=Session('live'); s.accept(idle(),0); s.accept(idle(110),.1)
        with self.assertRaises(ValueError): s.accept(idle(210),5)
        self.assertFalse(s.fresh(5))
    def test_serial_restore_and_hangup(self):
        import termios
        m,s=pty.openpty(); original=termios.tcgetattr(s)
        port=Serial(os.ttyname(s)); os.write(m,b'abc')
        import time; time.sleep(.02)
        self.assertEqual(port.read(),b'abc'); port.close()
        self.assertEqual(termios.tcgetattr(s),original)
        port=Serial(os.ttyname(s)); os.close(m)
        with self.assertRaises(OSError): port.read()
        port.close(); os.close(s)
    def test_fixture_and_export(self):
        root=Path(__file__).resolve().parents[1]
        with tempfile.TemporaryDirectory() as temp:
            exe=Path(temp)/'fixture'
            subprocess.run(['gcc','-std=c11','-Wall','-Wextra','-Werror','-I'+str(root/'firmware'),str(root/'tests/fixture.c'),str(root/'firmware/engine.c'),str(root/'firmware/wire.c'),'-o',str(exe)],check=True)
            data=subprocess.check_output([exe]); source=Path(temp)/'trace'
            source.write_bytes(data); session=load_replay(source)
            self.assertEqual(session.rows[-1]['good'],1)
            self.assertEqual(session.rows[-1]['bad'],1)
            destination=Path(temp)/'result.csv'; session.export(destination,'synthetic test')
            self.assertIn('replay',destination.read_text())
            with self.assertRaises(FileExistsError): session.export(destination,'test')
            source.write_bytes(data+b'KM1,')
            with self.assertRaises(ValueError): load_replay(source)
            fifo=Path(temp)/'fifo'; os.mkfifo(fifo)
            with self.assertRaises(ValueError): load_replay(fifo)

if __name__=='__main__': unittest.main()
