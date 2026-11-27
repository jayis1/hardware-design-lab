#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Import jayis1's pinned contact acquisition infrastructure, never overwrite."""
from pathlib import Path
import subprocess
ROOT = Path(__file__).resolve().parents[1]
REV = 'b8b5b3dc1ce9b80028fa46ff7bc62e206d49090b'
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
    text = subprocess.check_output(['git','show',f'{REV}:mask-key/{name}'],cwd=ROOT.parent,text=True)
    for old,new in [('mask-key','closure-basis'),('Mask Key','Closure Basis'),
                    ('MASK_','CLOSURE_'),('MK1','CB1'),('demo.mk1','demo.cb1'),
                    ('CONTROL_A','POSITION_A'),('CONTROL_B','POSITION_B'),
                    ('CONTROL_C','POSITION_C'),('RESPONSE_Y','POSITION_D')]:
        text=text.replace(old,new)
    path=ROOT/name
    path.parent.mkdir(parents=True,exist_ok=True)
    path.write_text(text)
print('Imported pinned infrastructure; device analysis is maintained separately.')
