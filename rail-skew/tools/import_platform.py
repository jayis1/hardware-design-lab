#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Reuse jayis1 platform from bellows-phase at 0192ec5; refuse overwrite.
The new rail engine and model are not copied. Shared carrier circuitry is not
claimed as a new electrical invention. No generated build products are copied.
"""
from pathlib import Path
root=Path(__file__).resolve().parents[1]
source=root.parent/'bellows-phase'
names=['firmware/main.c','firmware/board.h','firmware/registers.h',
       'firmware/startup.c','firmware/linker.ld','firmware/wire.h',
       'firmware/drivers/board.c','firmware/drivers/clock.c',
       'firmware/drivers/serial.c','firmware/Makefile',
       'tests/test_clock.c','tests/test_serial.c','.gitignore','LICENSE',
       'tools/generate_hardware.py','tools/check_hardware.py','app/transport.py']
for name in names:
    target=root/name
    if target.exists(): raise SystemExit('Refusing overwrite: '+str(target))
    text=(source/name).read_text()
    for old,new in [('bellows-phase','rail-skew'),('Bellows Phase','Rail Skew'),
                    ('BELLOWS_','RAIL_'),('manual bellows valve recorder','paired rail progression recorder'),
                    ('compressed, expanded, inlet open, outlet open','left first, left second, right first, right second'),
                    ("['COMPRESSED', 'EXPANDED', 'INLET', 'OUTLET']","['LEFT_1', 'LEFT_2', 'RIGHT_1', 'RIGHT_2']"),
                    ('BP1','RS1')]: text=text.replace(old,new)
    target.parent.mkdir(parents=True,exist_ok=True)
    target.write_text(text)
print('Imported shared platform; author jayis1')
