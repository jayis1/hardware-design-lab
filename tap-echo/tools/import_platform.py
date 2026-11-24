#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""One-time import of jayis1's reviewed passive carrier and MCU support."""
from pathlib import Path
import subprocess
ROOT = Path(__file__).resolve().parents[1]
REV = 'b47eaff4728f3325fa57c5e382106d2017561c01'
FILES = ['firmware/board.h', 'firmware/registers.h', 'firmware/drivers/board.c',
         'firmware/drivers/clock.c', 'firmware/drivers/serial.c', 'firmware/startup.c',
         'firmware/linker.ld', 'firmware/main.c', 'firmware/Makefile', 'firmware/wire.h',
         'tests/test_clock.c', 'tests/test_serial.c', 'app/transport.py',
         'tools/generate_hardware.py', 'tools/check_hardware.py', 'LICENSE', '.gitignore']
for name in FILES:
    if (ROOT / name).exists():
        raise SystemExit('Refusing overwrite: ' + name)
for name in FILES:
    text = subprocess.check_output(['git', 'show', f'{REV}:touch-tour/{name}'],
                                   cwd=ROOT.parent, text=True)
    for old, new in [('touch-tour', 'tap-echo'), ('Touch Tour', 'Tap Echo'),
                     ('TOUR_', 'ECHO_'), ('TT1', 'TE1'), ('demo.tt1', 'demo.te1'),
                     ('PAD_C', 'RECORD'), ('PAD_D', 'GATE')]:
        text = text.replace(old, new)
    path = ROOT / name
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text)
print('Imported platform', REV)
