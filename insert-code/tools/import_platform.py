#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Import explicitly credited board support; never overwrite local work."""
from pathlib import Path
import subprocess
ROOT = Path(__file__).resolve().parents[1]
REV = 'abf9a413a3201e072a8660884098b16a033f0d9d'
FILES = ['firmware/board.h', 'firmware/registers.h', 'firmware/drivers/board.c',
         'firmware/drivers/clock.c', 'firmware/drivers/serial.c', 'firmware/startup.c',
         'firmware/linker.ld', 'firmware/main.c', 'firmware/Makefile', 'firmware/wire.h',
         'tests/test_clock.c', 'tests/test_serial.c', 'app/transport.py', 'app/main.py',
         'tools/generate_hardware.py', 'tools/check_hardware.py', 'LICENSE', '.gitignore']
for name in FILES:
    if (ROOT / name).exists():
        raise SystemExit('Refusing overwrite: ' + name)
for name in FILES:
    text = subprocess.check_output(['git', 'show', f'{REV}:quorum-comb/{name}'], cwd=ROOT.parent, text=True)
    for old, new in [('quorum-comb', 'insert-code'), ('Quorum Comb', 'Insert Code'),
                     ('QUORUM_', 'INSERT_'), ('QC1', 'IC1'), ('demo.qc1', 'demo.ic1'),
                     ('REED_A', 'CODE_A'), ('REED_B', 'CODE_B'),
                     ('REED_C', 'CODE_C'), ('REFERENCE', 'CODE_D')]:
        text = text.replace(old, new)
    path = ROOT / name
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text)
print('Imported jayis1 platform', REV)
