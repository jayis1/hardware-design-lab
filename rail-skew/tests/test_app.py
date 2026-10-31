# Author: jayis1. SPDX-License-Identifier: MIT
import binascii, os, pathlib, pty, subprocess, sys, tempfile, termios, unittest
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/'app'))
from model import FIELDS, Session, validate
from transport import Decoder, Serial, load_replay
ROOT=pathlib.Path(__file__).resolve().parents[1]
def fixture():
    return subprocess.check_output([str(ROOT/'firmware/build/test-engine'),'fixture'])
def frame(row):
    body=('RS1,'+','.join(str(row[k]) for k in FIELDS)).encode()
    return body+b'*'+f'{binascii.crc_hqx(body,65535):04X}'.encode()+b'\n'
class Contract(unittest.TestCase):
    def test_encoder_and_categories(self):
        rows=Decoder().feed(fixture(),True)
        self.assertEqual(len(rows),7)
        self.assertEqual((rows[-1]['opening'],rows[-1]['closing']),(1,1))
        s=Session('replay')
        for row in rows: s.accept(row)
        self.assertGreater(rows[-1]['bin2'],0)
        self.assertGreater(rows[-1]['bin3'],0)
    def test_crc_impossible_truncation_resync(self):
        data=fixture(); rows=Decoder().feed(data,True)
        for change in ({'starts':1},{'last_peak':3},{'known':1},{'levels':2}):
            with self.assertRaises(ValueError): Decoder().feed(frame(dict(rows[0],**change)),True)
        d=Decoder(); self.assertEqual(d.feed(data[:20]+data),rows)
        self.assertGreater(d.errors,0)
        d=Decoder(); d.feed(data+b'RS1,')
        with self.assertRaises(ValueError): d.finish()
    def test_sparse_growth_and_frozen_last(self):
        rows=Decoder().feed(fixture(),True)
        for change in ({'last_ms':1},{'last_peak':2}):
            s=Session('replay'); s.accept(rows[-1])
            with self.assertRaises(ValueError): s.accept(dict(rows[-1],time=701,**change))
        s=Session('replay'); s.accept(rows[2])
        bad=dict(rows[2],time=301,bin2=rows[2]['bin2']+2)
        with self.assertRaises(ValueError): s.accept(bad)
    def test_freshness_and_export(self):
        rows=Decoder().feed(fixture(),True)
        s=Session('live'); s.connected=True
        s.accept(rows[0],10); self.assertFalse(s.fresh(10))
        s.accept(rows[1],10.1); self.assertTrue(s.fresh(10.1))
        s.accept(rows[1],10.2); self.assertFalse(s.fresh(11.2))
        s.connected=False; self.assertFalse(s.fresh(10.2))
        with tempfile.TemporaryDirectory() as tmp:
            p=pathlib.Path(tmp)/'e.csv'; s.export(p,10.2)
            self.assertIn('jayis1',p.read_text()); self.assertIn('live',p.read_text())
            self.assertEqual(p.stat().st_mode & 0o777,0o600)
            with self.assertRaises(FileExistsError): s.export(p,10.2)
    def test_pause_rollback_and_contradictory_duplicate(self):
        rows=Decoder().feed(fixture(),True)
        s=Session('live'); s.connected=True; s.accept(rows[0],0)
        with self.assertRaises(ValueError): s.accept(rows[1],2)
        self.assertTrue(s.warning)
        s=Session('replay'); s.accept(rows[1])
        with self.assertRaises(ValueError): s.accept(rows[0])
        s=Session('replay'); s.accept(rows[1])
        with self.assertRaises(ValueError): s.accept(dict(rows[1],raw=3))
    def test_gradual_backlog_and_terminal_freeze(self):
        row=Decoder().feed(fixture(),True)[0]
        s=Session('live'); s.connected=True; s.accept(row,0)
        # Frequent packets advancing only 1 ms cannot sustain live status.
        with self.assertRaisesRegex(ValueError,'progress mismatch'):
            for i in range(1,200): s.accept(dict(row,time=row['time']+i),i*.01)
        stopped=dict(row,state=3,anchor=0,flags=8)
        s=Session('replay'); s.accept(stopped)
        with self.assertRaisesRegex(ValueError,'halted evidence'):
            s.accept(dict(stopped,time=101,levels=1))
    def test_serial_restore_hangup_and_replay(self):
        master,slave=pty.openpty(); old=termios.tcgetattr(slave)
        serial=Serial(os.ttyname(slave)); os.write(master,fixture())
        import time
        time.sleep(.02)
        self.assertEqual(len(Decoder().feed(serial.read(),True)),7)
        serial.close(); self.assertEqual(old,termios.tcgetattr(slave))
        serial=Serial(os.ttyname(slave)); os.close(master)
        with self.assertRaises(OSError): serial.read()
        serial.close(); os.close(slave)
        with tempfile.TemporaryDirectory() as tmp:
            p=pathlib.Path(tmp)/'replay'; p.write_bytes(fixture())
            self.assertEqual(len(load_replay(p).rows),7)
            p.write_bytes(fixture()+b'R')
            with self.assertRaises(ValueError): load_replay(p)
            p.write_bytes(b'')
            with self.assertRaises(ValueError): load_replay(p)
            p.unlink(); os.mkfifo(p)
            with self.assertRaises(ValueError): load_replay(p)
if __name__=='__main__': unittest.main()
