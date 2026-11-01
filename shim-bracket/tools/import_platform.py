#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Import shared jayis1 F401 platform from duplex-witness, b58793b.
Does not import measurement policy, UI or README. Refuses overwrite.
"""
from pathlib import Path
root=Path(__file__).resolve().parents[1]
source=root.parent/'duplex-witness'
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
    for old,new in [('duplex-witness','shim-bracket'),('Duplex Witness','Shim Bracket'),
                    ('DUPLEX_','SHIM_'),('two-face stamp witness','indexed gauge declaration carrier'),
                    ('face A, face B, press, captive latch','selector bit 0, selector bit 1, FIT, REFUSE'),
                    ("['FACE_A', 'FACE_B', 'PRESS', 'LATCH']","['SELECT0', 'SELECT1', 'FIT', 'REFUSE']"),
                    ('DW1','SB1')]: text=text.replace(old,new)
    target.parent.mkdir(parents=True,exist_ok=True)
    target.write_text(text)
print('Imported shared platform; author jayis1')
