#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""One-time, overwrite-refusing reuse of jayis1's audited passive platform."""
from pathlib import Path
import subprocess
ROOT = Path(__file__).resolve().parents[1]
REV = '97a09650408803eb8b24ab908ba9ded367c65041'
FILES = ['firmware/board.h', 'firmware/registers.h', 'firmware/drivers/board.c',
         'firmware/drivers/clock.c', 'firmware/drivers/serial.c', 'firmware/startup.c',
         'firmware/linker.ld', 'firmware/main.c', 'firmware/Makefile',
         'tests/test_clock.c', 'tests/test_serial.c', 'app/transport.py',
         'tools/generate_hardware.py', 'tools/check_hardware.py', 'LICENSE', '.gitignore']
for name in FILES:
    path = ROOT / name
    if path.exists():
        raise SystemExit(f'Refusing overwrite: {path}')
for name in FILES:
    text = subprocess.check_output(['git', 'show', f'{REV}:sleeve-order/{name}'],
                                   cwd=ROOT.parent, text=True)
    for old, new in [('sleeve-order', 'ghost-grid'), ('Sleeve Order', 'Ghost Grid'),
                     ('SLEEVE_BOARD_H', 'GHOST_BOARD_H'),
                     ('passive telescoping endpoint', 'passive independent contact'),
                     ('telescoping endpoint', 'independent contact'),
                     ('OUT_HOME', 'KEY_00'), ('OUT_FAR', 'KEY_01'),
                     ('IN_HOME', 'KEY_10'), ('IN_FAR', 'KEY_11'),
                     ('SV1', 'GG1'), ('demo.sv1', 'demo.gg1')]:
        text = text.replace(old, new)
    path = ROOT / name
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text)
print('Imported platform sources from', REV)
