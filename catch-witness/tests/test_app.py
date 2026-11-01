# Author: jayis1. SPDX-License-Identifier: MIT
import binascii, csv, os, pathlib, pty, sys, tempfile, termios, time, unittest
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/'app'))
from model import Session, validate
from transport import Decoder, Serial, load_replay
from workbench import Workbench
ROOT=pathlib.Path(__file__).resolve().parents[1]
DATA=ROOT/'firmware/build/demo.cw1'

class Tests(unittest.TestCase):
    def test_real_encoder(self):
        session=load_replay(DATA)
        row=session.rows[-1]
        self.assertEqual([row[x] for x in ('starts','settled','unexplained','intentional','challenged','unseated')],[4,3,1,1,1,1])
    def test_malformed(self):
        data=DATA.read_bytes(); d=Decoder()
        self.assertTrue(d.feed(b'CW1,broken'+data)); self.assertGreater(d.errors,0)
        with self.assertRaises(ValueError):
            d=Decoder(); d.feed(data[:-1],True); d.finish()
        with self.assertRaises(ValueError): Decoder().feed(b'x'*65537)
        good=load_replay(DATA).rows[-1]
        bad=dict(good,starts=0)
        with self.assertRaises(ValueError): validate(bad)
        # CRC-valid impossible conservation is rejected by the real decoder.
        from model import FIELDS
        body=('CW1,'+','.join(str(bad[k]) for k in FIELDS)).encode()
        frame=body+b'*'+f'{binascii.crc_hqx(body,65535):04X}\n'.encode()
        with self.assertRaises(ValueError): Decoder().feed(frame,True)
    def test_session(self):
        rows=load_replay(DATA).rows; s=Session('replay')
        s.accept(rows[-1]); s.accept(rows[-1]); self.assertEqual(len(s.rows),1)
        with self.assertRaises(ValueError): s.accept(dict(rows[-1],now=1))
        self.assertTrue(s.blocked)
        s=Session('replay'); s.accept(rows[0])
        with self.assertRaises(ValueError): s.accept(dict(rows[0],levels=1))
    def test_transport(self):
        m,s=pty.openpty(); path=os.ttyname(s); before=termios.tcgetattr(s)
        port=Serial(path); os.write(m,DATA.read_bytes()); self.assertTrue(port.read())
        os.close(m)
        with self.assertRaises(OSError): port.read()
        port.close(); os.close(s)
        m,s=pty.openpty(); before=termios.tcgetattr(s); port=Serial(os.ttyname(s)); port.close()
        self.assertEqual(before,termios.tcgetattr(s)); os.close(m); os.close(s)
        with self.assertRaises((OSError,termios.error)): Serial('/dev/null')
    def test_gui(self):
        w=Workbench(); w.withdraw(); w.update()
        try:
            w.replay(str(DATA)); w.update()
            self.assertEqual(w.session.rows[-1]['intentional'],1)
            self.assertEqual(len(w.history.get_children()),len(w.session.rows))
            self.assertIn('replay',w.status.get().lower())
            for tab in w.tabs.tabs(): w.tabs.select(tab); w.update()
            with tempfile.TemporaryDirectory() as directory:
                target=pathlib.Path(directory)/'capture.csv'
                w.export(str(target))
                text=target.read_text(); self.assertIn('jayis1',text); self.assertIn('replay',text)
                with self.assertRaises(FileExistsError): w.export(str(target))
                fifo=pathlib.Path(directory)/'fifo'; os.mkfifo(fifo)
                with self.assertRaises(ValueError): load_replay(fifo)
            m,s=pty.openpty()
            try:
                w.connect(os.ttyname(s))
                with self.assertRaises(ValueError): w.connect(os.ttyname(s))
                # One actual production-encoder chunk through the live GUI path.
                os.write(m,DATA.read_bytes()); w.poll_once(); w.update()
                self.assertEqual(w.session.rows[-1]['starts'],4)
                self.assertIn('acquisition age unverified',w.status.get().lower())
                from model import FIELDS
                halted=dict(w.session.rows[-1],state=5,faults=4)
                halted['now']+=100
                body=('CW1,'+','.join(str(halted[k]) for k in FIELDS)).encode()
                os.write(m,body+b'*'+f'{binascii.crc_hqx(body,65535):04X}\n'.encode())
                w.poll_once(); self.assertIn('HALTED',w.status.get())
                self.assertIn('faults',w.summary.get())
                w.last_rx=time.monotonic()-2; w.poll_once()
                self.assertTrue(w.session.blocked)
                w.disconnect(); self.assertEqual(w.session.source,'serial')
            finally: os.close(m); os.close(s)
        finally: w.destroy()

if __name__=='__main__': unittest.main()
