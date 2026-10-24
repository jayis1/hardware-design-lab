# Author: jayis1. SPDX-License-Identifier: MIT
import importlib.util
import pathlib
import subprocess
import sys
import tempfile
import unittest
ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'app'))
from witness import Decoder, Session, crc16

def frame(values):
    payload = 'SW1,' + ','.join(map(str, values))
    return f'{payload}*{crc16(payload.encode()):04X}\n'.encode()
VALUES = [0, 1000, 1, 4, 12, 0, 400, 200, 200, 1, 0, 0, 0]
class ProtocolTests(unittest.TestCase):
    def test_real_firmware(self):
        raw = subprocess.check_output([str(ROOT / 'firmware/build/test-engine'), '--fixture'])
        rows = Decoder().feed(raw)
        self.assertEqual(len(rows), 1)
        self.assertEqual(rows[0]['drain_ms'], 200)
        self.assertEqual(rows[0]['phase'], 4)
    def test_fragment_resync(self):
        d = Decoder()
        self.assertEqual(d.feed(b'SW1,partial'), [])
        f = frame(VALUES)
        self.assertEqual(d.feed(f[:10]), [])
        self.assertEqual(len(d.feed(f[10:])), 1)
        self.assertGreater(d.errors, 0)
    def test_rejects(self):
        d = Decoder()
        self.assertEqual(d.feed(b'x'*5000 + b'\n'), [])
        self.assertEqual(d.feed(frame(VALUES).replace(b'200', b'201', 1)), [])
        bad = VALUES.copy(); bad[3] = 99
        self.assertEqual(d.feed(frame(bad)), [])
        self.assertLessEqual(len(d.buffer), 256)
    def test_session_order_freshness(self):
        s = Session()
        row = Decoder().feed(frame(VALUES))[0]
        self.assertTrue(s.accept(row, 10))
        self.assertFalse(s.accept(row, 11))
        self.assertFalse(s.fresh(12))
        row = row.copy(); row['seq'] = 1; row['ms'] = 999
        self.assertFalse(s.accept(row, 12))
        self.assertIn('restart', s.warning)
        self.assertEqual(len(s.rows), 1)
    def test_export_no_overwrite(self):
        s = Session('replay')
        s.accept(Decoder().feed(frame(VALUES))[0], 10)
        with tempfile.TemporaryDirectory() as tmp:
            path = pathlib.Path(tmp) / 'evidence.csv'
            s.export(path, 'replay', 12)
            self.assertIn('jayis1', path.read_text())
            self.assertIn('replay', path.read_text())
            with self.assertRaises(FileExistsError):
                s.export(path, 'live', 12)
    def test_export_preserves_integrity_and_provenance(self):
        s = Session('replay')
        s.accept(Decoder().feed(frame(VALUES))[0], 10)
        s.blocked = True
        s.warning = 'Possible device restart: reconnect required'
        with tempfile.TemporaryDirectory() as tmp:
            path = pathlib.Path(tmp) / 'evidence.csv'
            s.export(path, 'disconnected', 12, parse_errors=3)
            import csv
            with path.open() as stream:
                row = next(csv.DictReader(stream))
            self.assertEqual(row['source'], 'replay')
            self.assertEqual(row['connection'], 'disconnected')
            self.assertEqual(row['parse_errors'], '3')
            self.assertEqual(row['blocked'], 'True')
            self.assertIn('restart', row['warning'])
            self.assertEqual(row['fresh'], 'False')
if __name__ == '__main__':
    unittest.main()
