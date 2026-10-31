#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Import reviewed jayis1 platform at ca7121a; refuse existing destinations."""
from pathlib import Path
root = Path(__file__).resolve().parents[1]
source = root.parent / 'sieve-shuttle'
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
    for old, new in [('sieve-shuttle', 'lay-count'), ('Sieve Shuttle', 'Lay Count'),
                     ('SIEVE_', 'LAY_'), ('manual sieve motion recorder', 'manual winding traverse recorder'),
                     ('west, east, north, south zones', 'left, right, spool index, tension indication'),
                     ("['WEST', 'EAST', 'NORTH', 'SOUTH']", "['LEFT', 'RIGHT', 'INDEX', 'TENSION']"),
                     ('SS1', 'LC1')]:
        text = text.replace(old, new)
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(text)
print('Imported reviewed platform and passive circuit; author jayis1')
