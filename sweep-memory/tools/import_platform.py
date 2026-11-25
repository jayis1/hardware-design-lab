#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Import the credited, pinned passive carrier platform without overwriting."""
from pathlib import Path
import subprocess
ROOT = Path(__file__).resolve().parents[1]
REV = '396308d62ab2f6f041beff9168b56cf5a6eb8a7c'
FILES = ['firmware/board.h', 'firmware/registers.h', 'firmware/drivers/board.c',
         'firmware/drivers/clock.c', 'firmware/drivers/serial.c', 'firmware/startup.c',
         'firmware/linker.ld', 'firmware/main.c', 'firmware/Makefile', 'firmware/wire.h',
         'tests/test_clock.c', 'tests/test_serial.c', 'app/transport.py',
         'tools/generate_hardware.py', 'tools/check_hardware.py', 'LICENSE', '.gitignore']
for name in FILES:
    if (ROOT / name).exists():
        raise SystemExit('Refusing overwrite: ' + name)
for name in FILES:
    text = subprocess.check_output(['git', 'show', f'{REV}:insert-code/{name}'], cwd=ROOT.parent, text=True)
    for old, new in [('insert-code', 'sweep-memory'), ('Insert Code', 'Sweep Memory'),
                     ('INSERT_', 'SWEEP_'), ('IC1', 'SM1'), ('demo.ic1', 'demo.sm1'),
                     ('CODE_A', 'PHASE_A'), ('CODE_B', 'PHASE_B'),
                     ('CODE_C', 'PROBE_X'), ('CODE_D', 'PROBE_Y')]:
        text = text.replace(old, new)
    path = ROOT / name
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text)
print('Imported jayis1 platform', REV)
