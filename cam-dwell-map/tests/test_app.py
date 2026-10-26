# Author: jayis1. SPDX-License-Identifier: MIT
import binascii
import os
from pathlib import Path
import sys
import tempfile
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'app'))
from cam import parse, Stream, Session, read_replay, Serial
ROOT = Path(__file__).resolve().parents[1]
FIXTURE = (ROOT / 'firmware/build/fixture.txt').read_bytes().splitlines()[-1] + b'\n'
def frame(values):
    body = ','.join(values).encode()
    return body + b'*%04X\n' % binascii.crc_hqx(body, 65535)
def altered(**fields):
    parts = FIXTURE.split(b'*')[0].decode().split(',')
    for k, v in fields.items(): parts[int(k[1:])] = str(v)
    return frame(parts)
class Tests(unittest.TestCase):
    def test_real_encoder(self):
        r = parse(FIXTURE)
        self.assertEqual(r['closed'], 200)
        self.assertEqual(sum(r['bins']), 200)
    def test_validation(self):
        for data in [FIXTURE[:-3], FIXTURE.replace(b'CDM1', b'CDM2'),
                     altered(f11=201), altered(f14='B'+'0'*39),
                     altered(f6=0), altered(f12=0), altered(f2=2)]:
            with self.assertRaises(ValueError): parse(data)
    def test_stream_resync_and_tail(self):
        s = Stream()
        self.assertEqual(s.feed(FIXTURE[:12]), [])
        self.assertEqual(len(s.feed(FIXTURE)), 1)
        self.assertEqual(s.errors, 1)
        s.feed(b'CDM1,123')
        with self.assertRaises(ValueError): s.finish()
        self.assertEqual(len(s.buffer), 0)
    def test_monotonic_session(self):
        s = Session('replay')
        s.accept(parse(FIXTURE), 1)
        s.accept(parse(FIXTURE), 2)
        self.assertEqual(s.last_received, 1)
        with self.assertRaises(ValueError): s.accept(parse(altered(f7=1)), 2)
        self.assertTrue(s.blocked)
    def test_live_pause(self):
        s = Session('live')
        s.accept(parse(FIXTURE), 1)
        t = parse(FIXTURE)['time']
        with self.assertRaises(ValueError): s.accept(parse(altered(f1=t+100)), 3)
    def test_backlog_and_impossible_progress(self):
        s = Session('live'); s.accept(parse(FIXTURE), 1)
        t = s.last['time']
        s.accept(parse(altered(f1=t+1)), 1.5)
        with self.assertRaises(ValueError): s.accept(parse(altered(f1=t+2)), 2)
        s = Session('replay'); s.accept(parse(FIXTURE), 1)
        with self.assertRaises(ValueError): s.accept(parse(altered(f1=t+1, f6=101)), 2)
        with self.assertRaises(ValueError): parse(altered(f12=399))
        s = Session('replay'); s.accept(parse(altered(f2=2, f9=6)), 1)
        with self.assertRaises(ValueError): s.accept(parse(altered(f1=t+1000, f2=2, f9=6, f6=2)), 2)
    def test_replay_and_export(self):
        with tempfile.TemporaryDirectory() as d:
            source = Path(d) / 'capture.txt'
            source.write_bytes(b'# Author: jayis1; synthetic\n'+FIXTURE)
            records = read_replay(source)
            self.assertEqual(len(records), 1)
            s = Session('replay'); s.accept(records[0], 1); s.connected = False
            output = Path(d) / 'out.csv'; s.export(output, 5, 2)
            text = output.read_text()
            self.assertIn('replay', text); self.assertIn('jayis1', text)
            self.assertIn('False', text); self.assertIn('parser_errors,2', text)
            with self.assertRaises(FileExistsError): s.export(output, 5, 2)
            source.write_bytes(FIXTURE+b'CDM1,')
            with self.assertRaises(ValueError): read_replay(source)
            source.write_bytes(b'')
            with self.assertRaises(ValueError): read_replay(source)
            fifo = Path(d)/'fifo'; os.mkfifo(fifo)
            with self.assertRaises(ValueError): read_replay(fifo)
    def test_pty(self):
        master, slave = os.openpty()
        port = Serial(os.ttyname(slave))
        try:
            os.write(master, FIXTURE)
            self.assertEqual(port.read(), FIXTURE)
            os.close(master); master = None
            with self.assertRaises(OSError): port.read()
        finally:
            port.close(); os.close(slave)
            if master is not None: os.close(master)
if __name__ == '__main__': unittest.main()
