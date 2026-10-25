# Author: jayis1. SPDX-License-Identifier: MIT
import importlib.util
from pathlib import Path
import unittest
import tempfile
import os
import pty
import tkinter as tk
import time
spec = importlib.util.spec_from_file_location('rinse_app', Path(__file__).resolve().parents[1]/'app/app.py')
assert spec is not None and spec.loader is not None
app = importlib.util.module_from_spec(spec)
spec.loader.exec_module(app)
FIXTURE = Path(__file__).resolve().parents[1]/'firmware/build/fixture.txt'

class ProtocolTests(unittest.TestCase):
    def frame(self, row):
        body = ('RL1,' + ','.join(str(row[k]) for k in app.FIELDS)).encode()
        return body + f'*{app.crc(body):04X}\n'.encode()

    def test_impossible_exposure(self):
        row = app.parse(FIXTURE.read_bytes().splitlines(keepends=True)[0])
        for k in ('total', 'epoch_ms', 'longest'):
            row['a_'+k] = 900000
        with self.assertRaises(ValueError):
            app.parse(self.frame(row))

    def test_same_bath_rollback_and_time_budget(self):
        row = app.parse(FIXTURE.read_bytes().splitlines(keepends=True)[-1])
        for change in ('rollback', 'impossible_jump'):
            model = app.Model()
            model.feed(self.frame(row), 0)
            later = row.copy()
            later['seq'] += 1
            later['ms'] += 100
            if change == 'rollback':
                later['a_epoch_ms'] -= 1
            else:
                later['a_total'] += 101
            model.feed(self.frame(later), 1)
            self.assertTrue(model.blocked, change)

    def test_real_encoder(self):
        model = app.Model()
        model.feed(FIXTURE.read_bytes(), 1)
        self.assertEqual(model.errors, 0)
        self.assertEqual(model.latest['epoch'], 2)
        self.assertEqual(model.latest['a_baths'], 2)
        self.assertEqual(model.latest['b_baths'], 1)
        self.assertEqual(model.latest['c_baths'], 1)
        self.assertEqual(len(model.history), 10)
    def test_damage_resync_and_bound(self):
        model = app.Model()
        data = FIXTURE.read_bytes().splitlines(keepends=True)
        model.feed(b'RL1,123'+data[0], 0)
        self.assertEqual(model.latest['seq'], 0)
        model.feed(b'x'*10000, 0)
        self.assertLessEqual(len(model.pending), 3)
        with self.assertRaises(ValueError):
            app.parse(data[1].replace(b'RL1', b'RL2'))
    def test_rollback_duplicate_stale(self):
        model = app.Model()
        data = FIXTURE.read_bytes().splitlines(keepends=True)
        model.feed(data[1], 0)
        model.feed(data[1], 2)
        self.assertTrue(model.stale(2))
        model.feed(data[0], 3)
        self.assertTrue(model.blocked)
    def test_export(self):
        model = app.Model()
        model.source = 'replay'
        model.feed(FIXTURE.read_bytes(), 0)
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory)/'out.csv'
            model.export(path, False, 3)
            self.assertIn('jayis1', path.read_text())
            self.assertIn('replay', path.read_text())
            self.assertEqual(path.stat().st_mode & 0o777, 0o600)
            with self.assertRaises(FileExistsError):
                model.export(path, False, 3)

class GuiTests(unittest.TestCase):
    def test_pty_and_replay(self):
        root = tk.Tk()
        ui = app.App(root)
        master, slave = pty.openpty()
        try:
            ui.path.set(os.ttyname(slave))
            ui.connect()
            os.write(master, FIXTURE.read_bytes())
            deadline = time.monotonic() + 2
            while ui.model.latest is None and time.monotonic() < deadline:
                root.update()
                time.sleep(.01)
            assert ui.model.latest is not None
            self.assertEqual(ui.model.latest['epoch'], 2)
            self.assertEqual(len(ui.table.get_children()), 10)
            self.assertIn('A:', ui.summary.get())
            self.assertEqual(len(ui.tabs.tabs()), 3)
            os.close(master)
            master = None
            deadline = time.monotonic() + 1
            while ui.serial and time.monotonic() < deadline:
                root.update()
                time.sleep(.01)
            self.assertIsNone(ui.serial)
            self.assertEqual(ui.model.source, 'live')
            ui.replay(FIXTURE)
            self.assertEqual(ui.model.source, 'replay')
            assert ui.model.latest is not None
            self.assertEqual(ui.model.latest['a_baths'], 2)
        finally:
            ui.close()
            os.close(slave)
            if master is not None:
                os.close(master)

if __name__ == '__main__':
    unittest.main()
