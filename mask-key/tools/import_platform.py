#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Import pinned jayis1 infrastructure only; refuse existing destinations."""
from pathlib import Path
import subprocess
ROOT = Path(__file__).resolve().parents[1]
REV = '30bd817a0068e09cd4aafbde2447d768be8d2e0b'
FILES = ['firmware/board.h', 'firmware/registers.h', 'firmware/drivers/board.c',
         'firmware/drivers/clock.c', 'firmware/drivers/serial.c', 'firmware/startup.c',
         'firmware/linker.ld', 'firmware/main.c', 'firmware/Makefile', 'firmware/wire.h',
         'firmware/engine.c', 'firmware/engine.h', 'firmware/wire.c',
         'tests/test_clock.c', 'tests/test_serial.c', 'app/transport.py',
         'app/model.py', 'app/main.py', 'tools/generate_hardware.py',
         'tools/check_hardware.py', 'LICENSE', '.gitignore']
for name in FILES:
    if (ROOT/name).exists():
        raise SystemExit('Refusing overwrite: '+name)
for name in FILES:
    text = subprocess.check_output(['git','show',f'{REV}:truth-sieve/{name}'],cwd=ROOT.parent,text=True)
    for old,new in [('truth-sieve','mask-key'),('Truth Sieve','Mask Key'),
                    ('TRUTH_','MASK_'),('TS1','MK1'),('demo.ts1','demo.mk1'),
                    ('INPUT_A','CONTROL_A'),('INPUT_B','CONTROL_B'),
                    ('OUTPUT_X','CONTROL_C'),('OUTPUT_Y','RESPONSE_Y')]:
        text=text.replace(old,new)
    path=ROOT/name
    path.parent.mkdir(parents=True,exist_ok=True)
    path.write_text(text)
print('Imported pinned infrastructure. Device-specific implementation is maintained separately.')
