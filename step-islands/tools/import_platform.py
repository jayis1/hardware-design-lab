#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Pinned reuse of jayis1's acquisition, transport and passive hardware platform."""
from pathlib import Path
import subprocess
ROOT = Path(__file__).resolve().parents[1]
REV = '64d950ff8168329354bdedbb41f35b2ef12007fd'
FILES = ['firmware/'+s for s in ['board.h','registers.h','drivers/board.c','drivers/clock.c','drivers/serial.c','startup.c','linker.ld','main.c','Makefile','wire.h','wire.c','engine.h','engine.c']]
FILES += ['tests/test_engine.c','tests/test_clock.c','tests/test_serial.c','tests/fixture.c','app/transport.py','app/model.py','app/main.py','tools/generate_hardware.py','tools/check_hardware.py','LICENSE','.gitignore']
for name in FILES:
    if (ROOT/name).exists():
        raise SystemExit('Refusing overwrite: '+name)
for name in FILES:
    text = subprocess.check_output(['git','show',f'{REV}:mixing-gap/{name}'],cwd=ROOT.parent,text=True)
    for old,new in [('mixing-gap','step-islands'),('Mixing Gap','Step Islands'),('MIXING_','ISLANDS_'),('MG1','SI1'),('demo.mg1','demo.si1')]:
        text = text.replace(old,new)
    path=ROOT/name
    path.parent.mkdir(parents=True,exist_ok=True)
    path.write_text(text)
print('Imported pinned platform; device-specific analysis must be applied separately.')
