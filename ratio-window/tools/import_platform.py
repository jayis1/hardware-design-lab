#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Reproduce explicitly attributed platform reuse; never overwrite files."""
from pathlib import Path
import subprocess
ROOT = Path(__file__).resolve().parents[1]
REV = '99853fddaaf648c8ee5f573a069e02d46dbbee5f'
FILES = ['firmware/board.h', 'firmware/registers.h', 'firmware/drivers/board.c',
         'firmware/drivers/clock.c', 'firmware/drivers/serial.c', 'firmware/startup.c',
         'firmware/linker.ld', 'firmware/main.c', 'firmware/Makefile',
         'tests/test_clock.c', 'tests/test_serial.c', 'app/transport.py',
         'tools/generate_hardware.py', 'tools/check_hardware.py', 'LICENSE', '.gitignore']
for name in FILES:
    if (ROOT / name).exists():
        raise SystemExit('Refusing overwrite: ' + name)
for name in FILES:
    text = subprocess.check_output(['git', 'show', f'{REV}:foot-rock/{name}'],
                                   cwd=ROOT.parent, text=True)
    for old, new in [('foot-rock', 'ratio-window'), ('Foot Rock', 'Ratio Window'),
                     ('FOOT_BOARD_H', 'RATIO_BOARD_H'), ('foot contact', 'encoder contact'),
                     ('FOOT_NW', 'INPUT_A'), ('FOOT_NE', 'INPUT_B'),
                     ('FOOT_SW', 'OUTPUT_A'), ('FOOT_SE', 'OUTPUT_B'),
                     ('FR1', 'RW1'), ('demo.fr1', 'demo.rw1')]:
        text = text.replace(old, new)
    path = ROOT / name
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text)
print('Imported jayis1 platform from', REV)
