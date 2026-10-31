# Author: jayis1. SPDX-License-Identifier: MIT
import binascii
import os
from pathlib import Path
import pty
import subprocess
import sys
import tempfile
import termios
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'app'))
from model import FIELDS, Session, validate
from transport import Decoder, Serial, load_replay

def frame(row):
    body = ('RR1,' + ','.join(str(row[k]) for k in FIELDS)).encode()
    return body + b'*' + f'{binascii.crc_hqx(body,65535):04X}\n'.encode()

def idle(now=100):
    row = dict.fromkeys(FIELDS, 0)
    row.update(now=now, qualified=1)
    return row

class ModelTests(unittest.TestCase):
    def test_c_fixture(self):
        root = Path(__file__).resolve().parents[1]
        with tempfile.TemporaryDirectory() as directory:
            binary = Path(directory) / 'fixture'
            subprocess.run(['gcc', '-std=c11', '-Wall', '-Wextra', '-Werror',
                '-I'+str(root/'firmware'), str(root/'tests/fixture.c'),
                str(root/'firmware/engine.c'), str(root/'firmware/wire.c'),
                '-o', str(binary)], check=True)
            data = subprocess.check_output([str(binary)])
            decoder = Decoder()
            session = Session('replay')
            for offset in range(0,len(data),7):
                for row in decoder.feed(data[offset:offset+7], strict=True):
                    session.accept(row)
            decoder.finish()
            self.assertEqual(session.rows[-1]['state'],5)
            self.assertEqual(session.rows[-1]['high'],1)
            self.assertIn('1',session.result())
            target = Path(directory)/'evidence.csv'
            session.export(target,'synthetic fixture, not measured')
            self.assertIn('jayis1',target.read_text())
            self.assertEqual(target.stat().st_mode & 0o777,0o600)
            with self.assertRaises(FileExistsError): session.export(target,'test')
            last = dict(session.rows[-1]); last['now']+=100; last['end_ms']+=1
            with self.assertRaises(ValueError): session.accept(last)
    def test_temporal_contradictions(self):
        before=idle(1000)
        before.update(state=2,age=100,downmask=8,events=1,clear_ms=250)
        after=dict(before)
        after.update(now=1100,age=200,downmask=12,events=2,down2=50)
        session=Session('replay'); session.accept(before)
        with self.assertRaises(ValueError): session.accept(after)
        ready=idle(1000); ready.update(state=1,clear_ms=250)
        done=idle(1001)
        done.update(state=5,age=100,end_ms=100,downmask=15,upmask=1,
                    events=6,down2=10,down1=20,down0=30,up0=40,clear_ms=250)
        session=Session('replay'); session.accept(ready)
        with self.assertRaises(ValueError): session.accept(done)
        done.update(age=5000,end_ms=5000)
        with self.assertRaises(ValueError): Decoder().feed(frame(done),True)
        short=dict(before); short.update(downmask=12,events=2,down2=1)
        with self.assertRaises(ValueError): validate(short)
    def test_framing(self):
        good = frame(idle())
        self.assertEqual(len(Decoder().feed(good,True)),1)
        decoder = Decoder()
        self.assertEqual(len(decoder.feed(good[:15]+good)),1)
        self.assertEqual(decoder.errors,1)
        with self.assertRaises(ValueError): Decoder().feed(good[:-3]+b'00\n',True)
        decoder = Decoder(); decoder.feed(good[:-1])
        with self.assertRaises(ValueError): decoder.finish()
    def test_invalid(self):
        for changes in [dict(upmask=5),dict(state=5),dict(state=6),dict(gaps=1),
                        dict(downmask=8,events=0),dict(raw=16),dict(down0=3),dict(now=2**32)]:
            row=idle(); row.update(changes)
            with self.assertRaises(ValueError): validate(row)
    def test_progress_and_disconnect(self):
        session=Session('live'); session.connected=True
        session.accept(idle(100),10)
        self.assertFalse(session.fresh(10))
        session.accept(idle(200),10.1)
        self.assertTrue(session.fresh(10.1))
        session.accept(idle(200),10.5)
        self.assertFalse(session.fresh(12))
        session.connected=False
        self.assertFalse(session.fresh(10.2))
    def test_backlog_and_reset(self):
        session=Session('live'); session.connected=True
        session.accept(idle(100),10)
        with self.assertRaises(ValueError): session.accept(idle(200),12)
        self.assertTrue(session.blocked)
        session=Session('replay'); session.accept(idle(100))
        with self.assertRaises(ValueError): session.accept(idle(99))
        session=Session('replay'); session.accept(idle(100))
        changed=idle(100); changed['raw']=1
        with self.assertRaises(ValueError): session.accept(changed)
    def test_replay_limits(self):
        with tempfile.TemporaryDirectory() as directory:
            path=Path(directory)/'capture'
            path.write_bytes(frame(idle()))
            self.assertEqual(len(load_replay(path).rows),1)
            path.write_bytes(frame(idle())+b'RR')
            with self.assertRaises(ValueError): load_replay(path)
            path.write_bytes(b'')
            with self.assertRaises(ValueError): load_replay(path)
            path.unlink(); os.mkfifo(path)
            with self.assertRaises(ValueError): load_replay(path)
    def test_serial_restores_and_hangup(self):
        master,slave=pty.openpty()
        try:
            old=termios.tcgetattr(slave)
            serial=Serial(os.ttyname(slave))
            os.write(master,b'RR1,')
            import time
            time.sleep(.02)
            self.assertEqual(serial.read(),b'RR1,')
            serial.close()
            self.assertEqual(termios.tcgetattr(slave),old)
            serial=Serial(os.ttyname(slave)); os.close(master); master=None
            with self.assertRaises(OSError): serial.read()
            serial.close()
        finally:
            if master is not None: os.close(master)
            os.close(slave)

if __name__=='__main__': unittest.main()
