# Author: jayis1. SPDX-License-Identifier: MIT
import os
import pty
import termios
import time
import tkinter as tk
import unittest
from unittest.mock import patch
from test_app import a, root

class GuiTests(unittest.TestCase):
    def test_live_replay_and_disconnect(self):
        master,slave=pty.openpty()
        window=tk.Tk()
        app=a.Application(window)
        try:
            app.port.set(os.ttyname(slave)); app.connect()
            frames=(root/'firmware/build/fixture.txt').read_bytes()
            os.write(master,frames)
            for _ in range(5): window.update(); time.sleep(.12)
            self.assertEqual(app.model.latest['accepted'],2)
            self.assertEqual(len(app.table.get_children()),11)
            self.assertIn('LIVE',app.status.get())
            app.tabs.select(1); window.update()
            app.tabs.select(2); window.update()
            app.disconnect()
            self.assertEqual(app.model.origin,'live')
            self.assertIn('STALE',app.status.get())
            app.load_replay(root/'firmware/build/fixture.txt')
            self.assertEqual(app.model.origin,'replay')
            self.assertIn('NOT LIVE',app.status.get())
            app.port.set(os.ttyname(slave)); app.connect()
            self.assertEqual(app.summary.get(),'No records')
            self.assertEqual(len(app.table.get_children()),0)
            os.close(master); master=None
            for _ in range(3): window.update(); time.sleep(.12)
            self.assertFalse(app.model.connected)
        finally:
            app.close()
            if master is not None: os.close(master)
            os.close(slave)

    def test_serial_restores_on_constructor_failure(self):
        master,slave=pty.openpty()
        try:
            saved=termios.tcgetattr(slave)
            with patch.object(a.termios,'tcflush',side_effect=termios.error('injected')):
                self.assertRaises(termios.error,a.Serial,os.ttyname(slave))
            self.assertEqual(saved,termios.tcgetattr(slave))
        finally:
            os.close(master); os.close(slave)

if __name__=='__main__': unittest.main()
