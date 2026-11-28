#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""One-time pinned platform import; never run over completed device files."""
from pathlib import Path
import subprocess
ROOT = Path(__file__).resolve().parents[1]
REV = '181a193a02a4996b943970ee9c643f40ece0d9ca'
FILES = ['firmware/'+s for s in ['board.h','registers.h','drivers/board.c','drivers/clock.c','drivers/serial.c','startup.c','linker.ld','main.c','Makefile','wire.h','wire.c','engine.h','engine.c']]
FILES += ['tests/test_engine.c','tests/test_clock.c','tests/test_serial.c','tests/fixture.c','app/transport.py','app/model.py','app/main.py','tools/generate_hardware.py','tools/check_hardware.py','LICENSE','.gitignore']
for name in FILES:
    if (ROOT/name).exists():
        raise SystemExit('Refusing overwrite: '+name)
for name in FILES:
    text = subprocess.check_output(['git','show',f'{REV}:step-islands/{name}'],cwd=ROOT.parent,text=True)
    for old,new in [('step-islands','code-margin'),('Step Islands','Code Margin'),('ISLANDS_','MARGIN_'),('SI1','CM1'),('demo.si1','demo.cm1')]:
        text = text.replace(old,new)
    path=ROOT/name
    path.parent.mkdir(parents=True,exist_ok=True)
    path.write_text(text)
print('Imported pinned platform; apply device-specific changes separately.')
