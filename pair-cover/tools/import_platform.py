#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Import pinned jayis1 infrastructure; refuse existing targets."""
from pathlib import Path
import subprocess
ROOT = Path(__file__).resolve().parents[1]
REV = '63c5c0ae46a4676c6bcd2ece309f0206e62c789a'
FILES = ['firmware/board.h', 'firmware/registers.h', 'firmware/drivers/board.c',
         'firmware/drivers/clock.c', 'firmware/drivers/serial.c', 'firmware/startup.c',
         'firmware/linker.ld', 'firmware/main.c', 'firmware/Makefile', 'firmware/wire.h',
         'tests/test_clock.c', 'tests/test_serial.c', 'app/transport.py',
         'tools/generate_hardware.py', 'tools/check_hardware.py', 'LICENSE', '.gitignore']
for name in FILES:
    if (ROOT / name).exists():
        raise SystemExit('Refusing overwrite: ' + name)
for name in FILES:
    text = subprocess.check_output(['git', 'show', f'{REV}:sweep-memory/{name}'], cwd=ROOT.parent, text=True)
    for old, new in [('sweep-memory', 'pair-cover'), ('Sweep Memory', 'Pair Cover'),
                     ('SWEEP_', 'PAIR_'), ('SM1', 'PC1'), ('demo.sm1', 'demo.pc1'),
                     ('PHASE_A', 'CONTROL_A'), ('PHASE_B', 'CONTROL_B'),
                     ('PROBE_X', 'CONTROL_C'), ('PROBE_Y', 'CONTROL_D')]:
        text = text.replace(old, new)
    path = ROOT / name
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text)
print('Imported jayis1 infrastructure', REV)
