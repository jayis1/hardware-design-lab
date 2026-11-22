#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Explicit one-time reuse of the passive acquisition platform, no overwrites."""
from pathlib import Path
import subprocess
ROOT = Path(__file__).resolve().parents[1]
REV = '09d7e03'
FILES = ['firmware/board.h', 'firmware/registers.h', 'firmware/drivers/board.c',
         'firmware/drivers/clock.c', 'firmware/drivers/serial.c', 'firmware/startup.c',
         'firmware/linker.ld', 'firmware/main.c', 'firmware/Makefile',
         'tests/test_clock.c', 'tests/test_serial.c', 'app/transport.py',
         'tools/generate_hardware.py', 'tools/check_hardware.py', 'LICENSE', '.gitignore']
for name in FILES:
    if (ROOT / name).exists():
        raise SystemExit('Refusing overwrite: ' + name)
for name in FILES:
    text = subprocess.check_output(['git', 'show', f'{REV}:ghost-grid/{name}'],
                                   cwd=ROOT.parent, text=True)
    for old, new in [('ghost-grid', 'foot-rock'), ('Ghost Grid', 'Foot Rock'),
                     ('GHOST_BOARD_H', 'FOOT_BOARD_H'),
                     ('independent contact', 'foot contact'),
                     ('KEY_00', 'FOOT_NW'), ('KEY_01', 'FOOT_NE'),
                     ('KEY_10', 'FOOT_SW'), ('KEY_11', 'FOOT_SE'),
                     ('GG1', 'FR1'), ('demo.gg1', 'demo.fr1')]:
        text = text.replace(old, new)
    path = ROOT / name
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text)
print('Imported jayis1 platform from', REV)
