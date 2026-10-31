#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""One-time reuse of jayis1's platform from shutter-quartet at fcfd4d1.
No measurement engine is imported; destination files must not exist.
"""
from pathlib import Path
root = Path(__file__).resolve().parents[1]
source = root.parent / 'shutter-quartet'
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
    for old, new in [('shutter-quartet', 'coupon-transit'), ('Shutter Quartet', 'Coupon Transit'),
                     ('SHUTTER_', 'TRANSIT_'), ('four-point optical exposure recorder', 'paired holder transit recorder'),
                     ('light A, light B, light C, light D', 'A source, A destination, B source, B destination'),
                     ('unpowered NPN phototransistors only', 'voltage-free dry contacts only'),
                     ('Low-active phototransistor collectors', 'Low-active dry contacts'),
                     ('UNPOWERED NPN COLLECTORS ONLY', 'VOLTAGE-FREE CONTACTS ONLY'),
                     ("['LIGHT_A', 'LIGHT_B', 'LIGHT_C', 'LIGHT_D']", "['A_SRC', 'A_DST', 'B_SRC', 'B_DST']"),
                     ('SQ1', 'CT1')]:
        text = text.replace(old, new)
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(text)
print('Imported platform; author jayis1')
