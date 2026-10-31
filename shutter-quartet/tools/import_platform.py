#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Import the reviewed jayis1 carrier platform at rail-skew 386daf9.
Only board support, transport and electrical topology are reused, not the
measurement engine. Refuse overwrite and exclude build products.
"""
from pathlib import Path
root = Path(__file__).resolve().parents[1]
source = root.parent / 'rail-skew'
names = ['firmware/main.c', 'firmware/board.h', 'firmware/registers.h',
         'firmware/startup.c', 'firmware/linker.ld', 'firmware/wire.h',
         'firmware/drivers/board.c', 'firmware/drivers/clock.c',
         'firmware/drivers/serial.c', 'firmware/Makefile',
         'tests/test_clock.c', 'tests/test_serial.c', '.gitignore', 'LICENSE',
         'tools/generate_hardware.py', 'tools/check_hardware.py', 'app/transport.py']
for name in names:
    target = root / name
    if target.exists():
        raise SystemExit('Refusing overwrite: ' + str(target))
    text = (source / name).read_text()
    for old, new in [('rail-skew', 'shutter-quartet'), ('Rail Skew', 'Shutter Quartet'),
                     ('RAIL_', 'SHUTTER_'), ('paired rail progression recorder', 'four-point optical exposure recorder'),
                     ('left first, left second, right first, right second', 'light A, light B, light C, light D'),
                     ('dry contacts only', 'unpowered NPN phototransistors only'),
                     ('Low-active dry contacts', 'Low-active phototransistor collectors'),
                     ('VOLTAGE-FREE CONTACTS ONLY', 'UNPOWERED NPN COLLECTORS ONLY'),
                     ("['LEFT_1', 'LEFT_2', 'RIGHT_1', 'RIGHT_2']", "['LIGHT_A', 'LIGHT_B', 'LIGHT_C', 'LIGHT_D']"),
                     ('RS1', 'SQ1')]:
        text = text.replace(old, new)
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(text)
print('Imported shared platform; author jayis1')
