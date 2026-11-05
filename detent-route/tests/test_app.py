# Author: jayis1. SPDX-License-Identifier: MIT
import binascii, importlib, os, pathlib, pty, sys, tempfile, termios, time, unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'app'))
from model import FIELDS, Session, validate
from transport import Decoder, Serial, load_replay

def fixture():
    return (ROOT/'firmware/build/demo.dr1').read_bytes()

def rows():
    return Decoder().feed(fixture(),strict=True)

def encode(row):
    body=('DR1,'+','.join(str(row[k]) for k in FIELDS)).encode()
    return body+('*%04X\n'%binascii.crc_hqx(body,65535)).encode()

class ModelTests(unittest.TestCase):
    def test_actual_encoder(self):
        data=rows(); self.assertEqual(len(data),18)
        r=data[-1]
        self.assertEqual((r['cw'],r['ccw'],r['returns'],r['jumps'],r['aborted']),(4,1,1,1,1))
        session=Session('replay')
        for r in data: session.accept(r)
        self.assertFalse(session.live())
    def test_corruption_and_resync(self):
        d=Decoder(); first=fixture().splitlines(keepends=True)[0]
        self.assertEqual(len(d.feed(first[:12]+first)),1)
        self.assertEqual(d.errors,1)
        bad=bytearray(first);bad[5]^=1
        self.assertEqual(d.feed(bad),[])
        with self.assertRaises(ValueError): Decoder().feed(bad,strict=True)
        d.feed(first[:10])
        with self.assertRaises(ValueError): d.finish()
    def test_crc_valid_impossible(self):
        r=rows()[-1].copy();r['cw_laps']=2
        with self.assertRaises(ValueError): Decoder().feed(encode(r),strict=True)
        r=rows()[0].copy();r['last_ms']=5
        with self.assertRaises(ValueError): validate(r)
    def test_history_and_duplicate(self):
        data=rows(); s=Session('replay');s.accept(data[0]);s.accept(data[0])
        self.assertEqual(len(s.rows),1)
        bad=data[0].copy();bad['raw']=2
        with self.assertRaises(ValueError): s.accept(bad)
        self.assertTrue(s.blocked)
        s=Session('replay');s.accept(data[2]);bad=data[3].copy();bad['last_ms']+=1
        with self.assertRaises(ValueError): s.accept(bad)
    def test_sparse_multiple_completion_result(self):
        old=rows()[0]
        new=old.copy()
        new.update(sequence=old['sequence']+1, now=old['now']+24,
                   starts=2, cw=2, last_valid=1, last_from=0, last_to=2, last_ms=9999)
        s=Session('replay');s.accept(old)
        with self.assertRaises(ValueError):s.accept(new)
        new.update(last_from=1,last_to=2)
        s=Session('replay');s.accept(old)
        with self.assertRaises(ValueError):s.accept(new)
        new.update(last_ms=6)
        s=Session('replay');s.accept(old);s.accept(new)
    def test_live_stale_and_backlog(self):
        r=rows()[0];s=Session('live');s.connected=True
        s.accept(r,10);self.assertFalse(s.live(10))
        n=r.copy();n['sequence']+=1;n['now']+=100
        s.accept(n,10.1);self.assertTrue(s.live(10.2));self.assertFalse(s.live(13))
        n=n.copy();n['sequence']+=1;n['now']+=100
        with self.assertRaises(ValueError): s.accept(n,13)
    def test_replay_and_export(self):
        with tempfile.TemporaryDirectory() as d:
            path=pathlib.Path(d)
            (path/'input').write_bytes(fixture())
            s=load_replay(path/'input');s.export(path/'out.csv')
            self.assertIn('jayis1',(path/'out.csv').read_text())
            self.assertEqual((path/'out.csv').stat().st_mode&0o777,0o600)
            with self.assertRaises(FileExistsError):s.export(path/'out.csv')
            (path/'empty').write_bytes(b'')
            with self.assertRaises(ValueError):load_replay(path/'empty')
            os.mkfifo(path/'fifo')
            with self.assertRaises(ValueError):load_replay(path/'fifo')
            (path/'input').write_bytes(fixture()+b'DR1,')
            with self.assertRaises(ValueError):load_replay(path/'input')
            Session('none').export(path/'empty.csv')
            self.assertIn('metadata',(path/'empty.csv').read_text())
    def test_serial_restore(self):
        master,slave=pty.openpty();saved=termios.tcgetattr(slave)
        port=Serial(os.ttyname(slave));os.write(master,b'hello');value=b''
        for _ in range(20):
            value=port.read()
            if value: break
            time.sleep(.01)
        self.assertEqual(value,b'hello');port.close()
        self.assertEqual(termios.tcgetattr(slave),saved)
        os.close(master);os.close(slave)
        with self.assertRaises((OSError,termios.error)):Serial('/dev/null')

class GuiTests(unittest.TestCase):
    def test_real_event_loop(self):
        import tkinter as tk
        from workbench import Workbench
        root=tk.Tk();app=Workbench(root)
        master,slave=pty.openpty()
        try:
            app.connect(os.ttyname(slave))
            self.assertTrue(app.session.connected)
            with self.assertRaises(ValueError):app.connect(os.ttyname(slave))
            os.write(master,fixture())
            for _ in range(12): root.update();time.sleep(.02)
            self.assertEqual(len(app.table.get_children()),18)
            self.assertIn('CW 4',app.summary.get())
            for tab in app.tabs.tabs():app.tabs.select(tab);root.update()
            with tempfile.TemporaryDirectory() as d:
                app.session.export(pathlib.Path(d)/'ui.csv',app.decoder.errors)
            os.close(master);master=None
            for _ in range(8):root.update();time.sleep(.02)
            self.assertFalse(app.session.connected)
            self.assertEqual(len(app.session.rows),18)
            app.replay(ROOT/'firmware/build/demo.dr1')
            self.assertEqual(app.session.source,'replay')
            self.assertIn('NOT LIVE',app.status.get())
        finally:
            app.close()
            if master is not None:os.close(master)
            os.close(slave)

if __name__=='__main__':unittest.main()
