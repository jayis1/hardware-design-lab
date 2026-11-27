#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""One-time provenance-preserving platform import; refuses existing files."""
from pathlib import Path
import subprocess
ROOT=Path(__file__).resolve().parents[1]
REV='3762a0deb5f424a6ba0acd7ab664991b42dcffb3'
FILES=['firmware/'+s for s in ['board.h','registers.h','drivers/board.c','drivers/clock.c','drivers/serial.c','startup.c','linker.ld','main.c','Makefile','wire.h','wire.c','engine.h','engine.c']]
FILES += ['tests/test_clock.c','tests/test_serial.c','tests/fixture.c','app/transport.py','app/model.py','app/main.py','tools/generate_hardware.py','tools/check_hardware.py','LICENSE','.gitignore']
for name in FILES:
    if (ROOT/name).exists(): raise SystemExit('Refusing overwrite: '+name)
for name in FILES:
    text=subprocess.check_output(['git','show',f'{REV}:closure-basis/{name}'],cwd=ROOT.parent,text=True)
    for old,new in [('closure-basis','mixing-gap'),('Closure Basis','Mixing Gap'),('CLOSURE_','MIXING_'),('CB1','MG1'),('demo.cb1','demo.mg1'),('supported','complete'),('rules','missing_total')]:
        text=text.replace(old,new)
    path=ROOT/name
    path.parent.mkdir(parents=True,exist_ok=True)
    path.write_text(text)
print('Imported pinned jayis1 platform. Apply device-specific analysis separately.')
