#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Initial provenance import only; refuses overwriting existing files."""
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
    for old, new in [('touch-tour', 'quorum-comb'), ('Touch Tour', 'Quorum Comb'),
                     ('TOUR_', 'QUORUM_'), ('TT1', 'QC1'), ('demo.tt1', 'demo.qc1'),
                     ('PAD_A', 'REED_A'), ('PAD_B', 'REED_B'),
                     ('PAD_C', 'REED_C'), ('PAD_D', 'REFERENCE')]:
        text = text.replace(old, new)
    path = ROOT / name
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text)
print('Imported jayis1 platform', REV)
