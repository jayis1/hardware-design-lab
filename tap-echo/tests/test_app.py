# Author: jayis1. SPDX-License-Identifier: MIT
import binascii
import json
import os
from pathlib import Path
import pty
import sys
import tempfile
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'app'))
from model import FIELDS, Session, validate
from transport import Decoder, Serial, load_replay

def frame(row):
    body = ('TE1,' + ','.join(str(row[k]) for k in FIELDS)).encode()
    return body + f'*{binascii.crc_hqx(body,65535):04X}\n'.encode()

class ModelTests(unittest.TestCase):
    def row(self):
        return dict.fromkeys(FIELDS, 0)
    def test_frames(self):
        row = self.row(); data = frame(row); d = Decoder()
        self.assertEqual(d.feed(data[:15]), [])
        self.assertEqual(d.feed(data[15:]), [row]); d.finish()
        with self.assertRaises(ValueError): Decoder().feed(data.replace(b'TE1,0', b'TE1,1'), strict=True)
    def test_validation(self):
        row = self.row(); row['count'] = 17
        with self.assertRaises(ValueError): validate(row)
        row = self.row(); row['event_pad0'] = 1
        with self.assertRaises(ValueError): validate(row)
        row = self.row(); row['matches'] = 1
        with self.assertRaises(ValueError): validate(row)
    def test_reset_and_export(self):
        s = Session('live'); row = self.row(); row['samples'] = 50; s.accept(row)
        row = self.row(); row['samples'] = 1; s.accept(row)
        self.assertEqual(s.epochs, 2)
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'session.json'; s.export(path)
            self.assertEqual(json.loads(path.read_text())['author'], 'jayis1')
            self.assertEqual(path.stat().st_mode & 0o777, 0o600)
            with self.assertRaises(FileExistsError): s.export(path)
    def test_real_posix_transport(self):
        master, slave = pty.openpty(); link = Serial(os.ttyname(slave))
        try:
            row = self.row(); os.write(master, frame(row))
            import select
            self.assertTrue(select.select([link.fd], [], [], 1)[0])
            self.assertEqual(Decoder().feed(link.read()), [row])
        finally:
            link.close(); os.close(master); os.close(slave)
    def test_fixture(self):
        path = Path(__file__).resolve().parents[1] / 'firmware/build/demo.te1'
        s = load_replay(path)
        self.assertEqual(s.rows[-1]['result'], 2)
        self.assertEqual(s.rows[-1]['matches'], 1)

if __name__ == '__main__': unittest.main()
