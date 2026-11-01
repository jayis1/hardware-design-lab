#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Import jayis1 passive F401 carrier from shim-bracket at 91e5062.
Only shared acquisition/transport/electrical infrastructure; no experiment policy.
Refuse overwrites. KiCad library geometry retains its upstream license.
"""
from pathlib import Path
import re
root=Path(__file__).resolve().parents[1]
source=root.parent/'shim-bracket'
names=['firmware/main.c','firmware/board.h','firmware/registers.h',
       'firmware/startup.c','firmware/linker.ld','firmware/wire.h',
       'firmware/drivers/board.c','firmware/drivers/clock.c','firmware/drivers/serial.c',
       'firmware/Makefile','tests/test_clock.c','tests/test_serial.c',
       '.gitignore','LICENSE','tools/generate_hardware.py','tools/check_hardware.py',
       'app/transport.py']
for name in names:
    target=root/name
    if target.exists(): raise SystemExit('Refusing overwrite: '+str(target))
    text=(source/name).read_text()
    for old,new in [('shim-bracket','texture-cycle'),('Shim Bracket','Texture Cycle'),
                    ('SHIM_','TEXTURE_'),('indexed gauge declaration carrier','captive swatch preference carrier'),
                    ('SB1','TC1')]:
        text=text.replace(old,new)
    text=re.sub(r'\bFIT\b','REFERENCE',text)
    text=re.sub(r'\bREFUSE\b','PREFER',text)
    target.parent.mkdir(parents=True,exist_ok=True)
    target.write_text(text)
print('Imported shared platform; author jayis1')
