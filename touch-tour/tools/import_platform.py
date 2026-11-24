#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""One-time overwrite-refusing import; subsequent local changes are separate."""
from pathlib import Path
import subprocess
ROOT = Path(__file__).resolve().parents[1]
REV = '6aa21c2b75a58425b7f0abe1a00320dfc43e98f9'
FILES = ['firmware/board.h', 'firmware/registers.h', 'firmware/drivers/board.c',
         'firmware/drivers/clock.c', 'firmware/drivers/serial.c', 'firmware/startup.c',
         'firmware/linker.ld', 'firmware/main.c', 'firmware/Makefile', 'firmware/wire.h',
         'tests/test_clock.c', 'tests/test_serial.c', 'app/transport.py',
         'tools/generate_hardware.py', 'tools/check_hardware.py',
         'tools/check_release.py', 'LICENSE', '.gitignore']
for name in FILES:
    if (ROOT / name).exists():
        raise SystemExit('Refusing overwrite: ' + name)
for name in FILES:
    text = subprocess.check_output(['git', 'show', f'{REV}:parity-sled/{name}'],
                                   cwd=ROOT.parent, text=True)
    for old, new in [('parity-sled', 'touch-tour'), ('Parity Sled', 'Touch Tour'),
                     ('PARITY_', 'TOUR_'), ('PY1', 'TT1'), ('demo.py1', 'demo.tt1'),
                     ('DATA0', 'PAD_A'), ('DATA1', 'PAD_B'), ('DATA2', 'PAD_C'),
                     ('PARITY.', 'PAD_D.'), ("'PARITY'", "'PAD_D'")]:
        text = text.replace(old, new)
    path = ROOT / name
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text)
print('Imported platform', REV)
