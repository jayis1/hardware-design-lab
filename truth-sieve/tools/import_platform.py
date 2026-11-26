#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Reproduce pinned shared infrastructure. Refuse to overwrite any file."""
from pathlib import Path
import subprocess
ROOT = Path(__file__).resolve().parents[1]
REV = 'c9f54aef64ac5e16648fccaca0de07e211581eba'
FILES = ['firmware/board.h', 'firmware/registers.h', 'firmware/drivers/board.c',
         'firmware/drivers/clock.c', 'firmware/drivers/serial.c', 'firmware/startup.c',
         'firmware/linker.ld', 'firmware/main.c', 'firmware/Makefile', 'firmware/wire.h',
         'firmware/engine.c', 'firmware/engine.h', 'firmware/wire.c',
         'tests/test_clock.c', 'tests/test_serial.c', 'app/transport.py',
         'app/model.py', 'app/main.py', 'tools/generate_hardware.py',
         'tools/check_hardware.py', 'LICENSE', '.gitignore']
for name in FILES:
    if (ROOT/name).exists(): raise SystemExit('Refusing overwrite: '+name)
for name in FILES:
    text = subprocess.check_output(['git','show',f'{REV}:pair-cover/{name}'],cwd=ROOT.parent,text=True)
    for old,new in [('pair-cover','truth-sieve'),('Pair Cover','Truth Sieve'),
                    ('PAIR_','TRUTH_'),('PC1','TS1'),('demo.pc1','demo.ts1'),
                    ('CONTROL_A','INPUT_A'),('CONTROL_B','INPUT_B'),
                    ('CONTROL_C','OUTPUT_X'),('CONTROL_D','OUTPUT_Y')]:
        text=text.replace(old,new)
    path=ROOT/name
    path.parent.mkdir(parents=True,exist_ok=True)
    path.write_text(text)
print('Imported pinned jayis1 platform; device-specific patches are separately maintained.')
